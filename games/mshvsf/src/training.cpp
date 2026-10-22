// SPDX-License-Identifier: GPL-3.0-only
//
// Marvel Super Heroes vs. Street Fighter: the training aids, put together from the toolkit's fight/ parts
// (fight/fight.h). The input display needs only the panel and the way each player faces; the frame data,
// the combo readout and the hit boxes read the players' records and the boxes' tables in the ROM (mshvsf.h).
// The dummy (player 2 standing, crouching, jumping, guarding, recording and replaying) waits on the game's
// own idea of guarding being found (ROADMAP.md).
#include "mshvsf.h"

#include "fight.h"
#include "machine.h"
#include "state.h"

#include <algorithm>
#include <cstdlib>
#include <string>
#include <vector>

namespace cps2recomp {

namespace {
    using namespace mshvsf;

    constexpr int kRefillAfter = 90; // frames without a hit after which health comes back

    // A signed word of the ROM's data (a box table's entry)
    inline int Rom16(uint32_t address) {
        if (address + 2 > ProgramSize()) {
            return 0;
        }
        const uint8_t *p = Program(address);
        return static_cast<int16_t>((p[0] << 8) | p[1]);
    }
    // A long of the ROM's data (a table's pointer)
    inline uint32_t Rom32(uint32_t address) {
        if (address + 4 > ProgramSize()) {
            return 0;
        }
        const uint8_t *p = Program(address);
        return (static_cast<uint32_t>(p[0]) << 24) | (static_cast<uint32_t>(p[1]) << 16) |
               (static_cast<uint32_t>(p[2]) << 8) | p[3];
    }

    fight::InputHistory g_inputs;
    fight::Frames g_frames;
    fight::MoveLog g_log;
    fight::Combos g_combos;
    fight::Dummy g_dummy;
    bool g_fighting = false;
    uint8_t g_healthBefore[2] = {0, 0};
    bool g_guardBefore[2] = {false, false};
    uint16_t g_held[2] = {0, 0}; // what the game was told each player holds this frame
    bool g_logOpen[2] = {false, false}; // a move the frame log is following (attacks only)
    int g_logWait[2] = {0, 0};          // (how long after it the log keeps being stepped, to write it)
    std::vector<HudBox> g_nextBoxes;

    void Afresh() {
        g_inputs.Clear();
        g_frames.Reset();
        g_combos.Clear();
        g_healthBefore[0] = g_healthBefore[1] = 0;
        g_guardBefore[0] = g_guardBefore[1] = false;
        g_logOpen[0] = g_logOpen[1] = false;
        g_logWait[0] = g_logWait[1] = 0;
        g_nextBoxes.clear();
    }

    // A move's start for the log: who, what was held (as the input display shows it), in the air, and which
    // move the game gives it (its own number at +0x60)
    fight::MoveLog::Start MoveStart(int n) {
        fight::MoveLog::Start start;
        start.character = CharacterName(n);
        start.input = fight::Notation(g_held[n], FacingLeft(n));
        start.air = Ram8(Player(n) + kAction) == kActionJump;
        start.id = Ram16(Player(n) + kMove);
        return start;
    }

    // The boxes an object's record says are out, in the stage's coordinates: their middles (across and up
    // from the picture's top edge, which is what the record's height is) and their half sizes.
    void ObjectBoxes(uint32_t object, std::vector<fight::StageBox> &boxes) {
        if (!Ram8(object + kActive)) {
            return;
        }
        const uint32_t table = Ram32(object + kBoxTable);
        if (table < 0x1000 || table >= 0x400000 || (table & 1)) {
            return;
        }
        const uint8_t flip = Ram8(object + kFlip);
        const int x = static_cast<int16_t>(Ram16(object + kX));
        const int y = static_cast<int16_t>(Ram16(object + kY));
        const int screenTop = ScreenTop();
        auto placeAt = [&](uint32_t address, fight::BoxKind kind) {
            const int across = Rom16(address);
            const int halfWidth = Rom16(address + 2);
            const int up = Rom16(address + 4);
            const int halfHeight = Rom16(address + 6);
            if (halfWidth <= 0 || halfHeight <= 0) {
                return;
            }
            const int centreX = x + ((flip & 1) ? -across : across);
            const int centreY = y + ((flip & 2) ? -up : up);
            boxes.push_back({kind, centreX, screenTop - centreY, halfWidth, halfHeight, false});
        };
        auto place = [&](uint16_t number, fight::BoxKind kind) {
            if (number) {
                placeAt(table + (number & kBoxNumberMask) * 8u, kind);
            }
        };
        for (const uint32_t offset : kHurtBoxes) {
            place(Ram16(object + offset), fight::BoxKind::Hurt);
        }
        place(Ram16(object + kThrowBox), fight::BoxKind::Throw);
        // (the two attack boxes; bit 15 of the number is the throw's range, not an attack that can hit)
        for (const uint32_t offset : {kAttackBox, kAttackBox2}) {
            const uint16_t number = Ram16(object + offset);
            place(number, (number & kThrowRangeBit) ? fight::BoxKind::ThrowRange : fight::BoxKind::Attack);
        }
        // (the pushing box: its number is a byte and its table is shared by every character, a long a number
        // that points to the boxes of each character in turn)
        const uint8_t push = Ram8(object + kPushBox);
        const uint32_t pushList = Rom32(kPushBoxTable + static_cast<uint32_t>(push) * 2u);
        const uint32_t pushAt = pushList + (static_cast<uint32_t>(Ram8(object + kCharacter)) << 2);
        if (pushList >= 0x1000 && pushList < 0x400000 && pushAt + 8 <= ProgramSize()) {
            placeAt(pushAt, fight::BoxKind::Push);
        }
    }

    // The boxes as the picture shows them (the game's memory is one frame behind the picture the port draws)
    void CollectBoxes(Machine &machine) {
        std::vector<fight::StageBox> stage;
        for (int n = 0; n < 2; n++) {
            ObjectBoxes(Player(n), stage);
        }
        const int cameraX = ScreenLeft();
        g_nextBoxes.clear();
        for (const fight::StageBox &box : stage) {
            // (the toolkit places a box against the floor; this game's records measure height down from the
            // picture's top, so the box's "up" is the stage's top edge less its height, and the floor is 0)
            g_nextBoxes.push_back(fight::ToPicture({box.kind, box.x, box.y, box.halfWidth, box.halfHeight, box.thrown},
                                                   cameraX, 0, 0));
        }
    }

    // An attack box of `attacker` is near enough that the game could test it against `victim`: the box's middle
    // across the stage and its half width are read the way the game reads them. (The game's own guard-range
    // test is not found yet, so this is a distance: PROGRESS.md.)
    bool AttackNear(int attacker, int victim) {
        if (!Ram8(Player(attacker) + kActive)) {
            return false;
        }
        const uint32_t table = Ram32(Player(attacker) + kBoxTable);
        if (table < 0x1000 || table >= 0x400000 || (table & 1)) {
            return false;
        }
        const uint8_t flip = Ram8(Player(attacker) + kFlip);
        const int x = static_cast<int16_t>(Ram16(Player(attacker) + kX));
        const int y = static_cast<int16_t>(Ram16(Player(attacker) + kY));
        const int victimX = static_cast<int16_t>(Ram16(Player(victim) + kX));
        const int victimY = static_cast<int16_t>(Ram16(Player(victim) + kY)) - 48;
        for (const uint32_t offset : {kAttackBox, kAttackBox2}) {
            const uint16_t number = Ram16(Player(attacker) + offset);
            if (!number || (number & kThrowRangeBit)) {
                continue;
            }
            const uint32_t address = table + (number & kBoxNumberMask) * 8u;
            const int boxAcross = static_cast<int16_t>(Rom16(address));
            const int halfWidth = static_cast<int16_t>(Rom16(address + 2));
            const int up = static_cast<int16_t>(Rom16(address + 4));
            if (halfWidth <= 0) {
                continue;
            }
            const int centreX = x + ((flip & 1) ? -boxAcross : boxAcross);
            const int centreY = y + ((flip & 2) ? -up : up);
            if (std::abs(centreX - victimX) <= halfWidth + 34 && std::abs(centreY - victimY) <= 90) {
                return true;
            }
        }
        return false;
    }

    // The attacker is in the air (a jump-in must be guarded standing)
    bool AttackerAirborne(int n) {
        const uint8_t action = Ram8(Player(n) + kAction);
        return action == kActionJumpStart || action == kActionJump || static_cast<int16_t>(Ram16(Player(n) + kY)) < kGround - 8;
    }

    // Player 1 has started something (or is in the air towards player 2) close enough that the dummy should
    // be holding away already. The game decides whether it would have hit; a human holds back as the move
    // begins, not on the frame its box comes out, which is a frame or two too late to be read as guarding.
    bool AttackComing(int attacker, int victim) {
        const uint8_t action = Ram8(Player(attacker) + kAction);
        if (action != kActionAttack && !AttackerAirborne(attacker)) {
            return false;
        }
        const int x = static_cast<int16_t>(Ram16(Player(attacker) + kX));
        const int y = static_cast<int16_t>(Ram16(Player(attacker) + kY));
        const int victimX = static_cast<int16_t>(Ram16(Player(victim) + kX));
        const int victimY = static_cast<int16_t>(Ram16(Player(victim) + kY));
        return std::abs(x - victimX) <= 175 && y <= victimY + 40;
    }

    // What the dummy is told of the fight this frame
    fight::DummyView DummyViewNow(uint16_t attackerHeld) {
        fight::DummyView view;
        view.fighting = Fighting();
        view.doing = g_frames.Now(1);
        view.onRight = static_cast<int16_t>(Ram16(Player(1) + kX)) >= static_cast<int16_t>(Ram16(Player(0) + kX));
        view.attackInRange = AttackComing(0, 1) || AttackNear(0, 1);
        // (an attack held down is a low one: guard it crouching; anything else standing, and a jump-in standing)
        view.guardStanding = !(attackerHeld & kDown) || AttackerAirborne(0);
        view.sinceHit = g_combos.Since(1);
        view.knowsDown = true;
        view.down = KnockedDown(1);
        return view;
    }

    void Frame(Machine &machine) {
        const bool fighting = Fighting();
        if (!fighting || !g_fighting) {
            Afresh();
            g_fighting = fighting;
            if (!fighting) {
                SetHud({});
                SetBoxes({});
                return;
            }
            for (int n = 0; n < 2; n++) {
                g_healthBefore[n] = Ram8(Player(n) + kHealth);
            }
        }
        g_combos.Frame();
        const bool readout = FeatureOn("damage_readout"), refill = FeatureOn("health_refill");
        const bool inputs = FeatureOn("input_display");
        for (int n = 0; n < 2; n++) {
            const uint8_t health = Ram8(Player(n) + kHealth);
            // A hit has landed: the health went down. (The game does not take any off for a guarded hit, so a
            // guarded one reads as none; the game's own guarding is not known yet.)
            if (health < g_healthBefore[n] && g_healthBefore[n] <= kHealthFull) {
                const unsigned damage = static_cast<unsigned>(g_healthBefore[n] - health);
                const fight::Doing doing = g_frames.Now(n);
                g_frames.Contact(n, false);
                g_combos.Hit(n, damage, 0, doing == fight::Doing::HitStun || doing == fight::Doing::Held);
            }
            g_healthBefore[n] = health;
            if (refill && g_combos.Since(n) > kRefillAfter && health < kHealthFull) {
                SetRam8(Player(n) + kHealth, kHealthFull);
                SetRam8(Player(n) + kHealthShown, kHealthFull);
                g_healthBefore[n] = kHealthFull;
            }
        }
        std::vector<HudLine> lines;
        std::vector<HudBox> boxes;
        for (int n = 0; n < 2; n++) {
            if (readout) {
                g_combos.Hud(n, lines, 0.17f, true, {}, true);
            }
            g_inputs.Push(n, g_held[n], FacingLeft(n));
            if (inputs) {
                g_inputs.Hud(n, lines, 0.30f);
            }
            fight::StepView view;
            const bool attackAction = Ram8(Player(n) + kAction) == kActionAttack;
            const bool guarding = Guarding(n);
            const bool downed = KnockedDown(n);
            // (a guarded hit: the game takes no health off, and the player is in the blockstun pose. Its start
            // is what tells the frame counting a hit was guarded, for the advantage after it.)
            if (guarding && !g_guardBefore[n]) {
                g_frames.Contact(n, true);
            }
            g_guardBefore[n] = guarding;
            view.control = downed ? fight::StepView::Held
                          : (Reeling(n) || guarding) ? fight::StepView::Reeling
                                                     : fight::StepView::Own;
            view.canAct = CanAct(n);
            view.attacking = attackAction;
            view.jumping = (Ram8(Player(n) + kAction) == kActionJumpStart || Ram8(Player(n) + kAction) == kActionJump ||
                            Ram8(Player(n) + kAction) == kActionLand) ||
                           Ram16(Player(n) + kY) < kGround;
            const uint16_t attack = Ram16(Player(n) + kAttackBox), attack2 = Ram16(Player(n) + kAttackBox2);
            view.attackOut = ((attack && !(attack & kThrowRangeBit)) || (attack2 && !(attack2 & kThrowRangeBit)));
            g_frames.Step(n, view);
            // (the log wants moves only: a jump and a landing are not ones, and the game's action says which
            // steps of a move are its attack's. It is stepped a while after the move too, so that the move is
            // written once its advantage could have come in.)
            if (attackAction) {
                g_logOpen[n] = true;
                g_logWait[n] = 0;
            }
            if (FeatureOn("frame_log") && g_logOpen[n]) {
                g_log.Step(n, g_frames, MoveStart);
                if (!attackAction && ++g_logWait[n] > 70) {
                    g_logOpen[n] = false;
                }
            }
        }
        g_frames.Hud(FeatureOn("frame_data"), FeatureOn("frame_meter"), lines, boxes, 0.72f, 193, 384, 0);
        if (FeatureOn("hit_boxes")) {
            // (what the record says now is what the next picture shows)
            CollectBoxes(machine);
            boxes.insert(boxes.end(), g_nextBoxes.begin(), g_nextBoxes.end());
        } else {
            g_nextBoxes.clear();
        }
        SetBoxes(std::move(boxes));
        SetHud(std::move(lines));
    }
} // namespace

void TrainingBeforeFrame(Machine &machine) {
    // What the game is told the panel holds this frame: player 2's first, in case the dummy works it, and
    // what the dummy chose is what the input display and the frame log then show.
    g_dummy.BeforeFrame(machine.players[0], machine.players[1], DummyViewNow(machine.players[0]));
    g_held[0] = machine.players[0];
    g_held[1] = machine.players[1];
}

void TrainingFrame(Machine &machine) {
    Frame(machine);
}

void TrainingState(Archive &ar) {
    ar.Mark(0x4D534856); // "MSHV"
    ar.Pod(g_fighting);
    ar.Pod(g_healthBefore[0]);
    ar.Pod(g_healthBefore[1]);
    ar.Pod(g_guardBefore[0]);
    ar.Pod(g_guardBefore[1]);
    ar.Pod(g_held[0]);
    ar.Pod(g_held[1]);
    ar.Pod(g_combos);
    ar.Pod(g_inputs);
    g_dummy.Serialize(ar);
    g_frames.Serialize(ar);
}

void RegisterTraining() {
    Game &game = TheGame();
    g_frames.print = getenv("MSHV_FRAMES") != nullptr;
    game.features.push_back({"input_display", "Training", "Input display",
                             "Shows what each player presses and for how many frames: the stick as a keypad number (5 is the "
                             "middle, 6 towards, 2 down), then the buttons (LP MP HP LK MK HK)."});
    game.features.push_back({"frame_data", "Training", "Frame data",
                             "After a move: the step it can first hit on (startup), how many it can hit for (active) and how "
                             "many follow before the player can act (recovery)."});
    game.features.push_back({"frame_meter", "Training", "Frame meter",
                             "A row of cells for each player, one for every step: green before a move can hit, red while it can, "
                             "blue after; yellow reeling from a hit; grey in the air."});
    game.features.push_back({"frame_log", "Training", "Frame data log",
                             "Writes every move's frame data to frame_data.csv, beside the settings file: the character, what "
                             "was held as it began, startup, active and recovery."});
    game.features.push_back({"damage_readout", "Training", "Combo readout",
                             "Under the bar of the player hit: what the hit took off, and for a combo (hits landing before the "
                             "player is free again) the hits and the damage, and the round's best."});
    game.features.push_back({"hit_boxes", "Training", "Hit boxes",
                             "Draws the boxes the game tests: where a player can be hit (blue), attacks (red; yellow for a "
                             "throw's range, white for a box it can be thrown from)."});
    game.features.push_back({"health_refill", "Training", "Health refills",
                             "A player's health comes back when they have not been hit for a moment."});
    // The dummy: player 2 worked by the player's choices. Guarding uses a distance to player 1's attack box
    // (the game's own guard-range test is not found yet: PROGRESS.md); a jump-in is guarded standing.
    fight::DummyOptions dummy;
    dummy.guardActions.push_back({"Pushes away", [](int frame, uint16_t, uint16_t) -> uint16_t {
                                      static constexpr uint16_t kMash[] = {kPunch1, 0, kPunch2, 0, kPunch3, 0,
                                                                           kKick1, 0,  kKick2, 0,  kKick3, 0};
                                      return kMash[frame % 12];
                                  },
                                  true});
    dummy.guardActionsWords = "What player 2 does while guarding a hit: presses buttons to push the attacker away "
                              "(the game's own guard cancels are not known yet).";
    dummy.downAfter = 34; // (the length of a hit's stun seen on the board)
    fight::RegisterDummy(dummy);
}

} // namespace cps2recomp
