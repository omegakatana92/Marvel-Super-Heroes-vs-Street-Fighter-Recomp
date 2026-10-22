// SPDX-License-Identifier: GPL-3.0-only
//
// Marvel Super Heroes vs. Street Fighter: cheats and free play. Optional features (the launcher's Features
// page and the in-game menu switch them); they work on the game's memory after every frame. With every
// feature off the game runs exactly as the board does.
#include "mshvsf.h"

#include "state.h"

namespace cps2recomp {

namespace {
    using namespace mshvsf;

    // What the round began with: the full health of each player (learned from the first frames of a round)
    // and the clock it began at. 0: no round has been seen yet.
    uint8_t healthAtStart[2] = {0, 0};
    uint8_t clockAtStart = 0;

    void ForgetRound() {
        healthAtStart[0] = healthAtStart[1] = 0;
        clockAtStart = 0;
    }

    void Frame(Machine &) {
        // (two credits: a second player can always join. The title waits for one, so the attract show gives
        // way to it, as on a board with free play set.)
        if (FeatureOn("free_play") && Ram8(kCredits) < kCreditsTwo) {
            SetRam8(kCredits, kCreditsTwo);
        }
        if (!Fighting()) {
            ForgetRound();
            return;
        }
        // (learn the round's full health while the bars are still up)
        for (int n = 0; n < 2; n++) {
            const uint8_t health = Ram8(Player(n) + kHealth);
            if (health > healthAtStart[n] && health <= kHealthFull) {
                healthAtStart[n] = health;
            }
        }
        if (FeatureOn("infinite_time")) {
            // The clock stands still: keep it at the second it read when this round began (BCD).
            if (clockAtStart == 0) {
                const uint8_t clock = Ram8(kClock);
                if (clock != 0 && clock <= kClockFull) {
                    clockAtStart = clock;
                }
            }
            if (clockAtStart != 0) {
                SetRam8(kClock, clockAtStart);
            }
        }
        static const char *const kNoDamage[2] = {"no_damage", "no_damage_2"};
        for (int n = 0; n < 2; n++) {
            if (FeatureOn(kNoDamage[n]) && healthAtStart[n] != 0) {
                SetRam8(Player(n) + kHealth, healthAtStart[n]);
                SetRam8(Player(n) + kHealthShown, healthAtStart[n]);
            }
        }
        // One-hit knockouts: the opponent (player 2, the computer's side in a one-player game) is left with a
        // single point of health, so any hit ends the round.
        if (FeatureOn("one_hit")) {
            SetRam8(Player(1) + kHealth, 1);
            SetRam8(Player(1) + kHealthShown, 1);
        }
        // The LEVEL bars: every gauge is kept, so a super is always ready.
        if (FeatureOn("full_gauge")) {
            for (int n = 0; n < 2; n++) {
                SetRam8(Player(n) + kGauges, kGaugesFull);
            }
        }
    }
} // namespace

void CheatsFrame(Machine &machine) {
    Frame(machine);
}

void CheatsState(Archive &ar) {
    ar.Pod(healthAtStart[0]);
    ar.Pod(healthAtStart[1]);
    ar.Pod(clockAtStart);
}

void RegisterCheats() {
    Game &game = TheGame();
    game.features.push_back({"free_play", "Game", "Free play", "Start works without a coin.", kFeatureEnhanced});
    game.features.push_back({"no_damage", "Cheats", "No damage", "Player 1's health does not go down."});
    game.features.push_back(
        {"no_damage_2", "Cheats", "No damage, player 2", "Player 2's health does not go down (the opponent's, in a one-player game)."});
    game.features.push_back({"infinite_time", "Cheats", "Infinite time", "The round's clock stands still."});
    game.features.push_back(
        {"one_hit", "Cheats", "One-hit knockouts", "Player 2 (the opponent, in a one-player game) goes down with the first hit."});
    game.features.push_back({"full_gauge", "Cheats", "Full LEVEL bars",
                             "Every gauge of both players' LEVEL (super) bars is kept, so a super is always ready."});
}

} // namespace cps2recomp
