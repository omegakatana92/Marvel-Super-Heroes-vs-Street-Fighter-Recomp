// SPDX-License-Identifier: GPL-3.0-only
//
// Marvel Super Heroes vs. Street Fighter (Europe 970625): names for what is known of the game's memory.
// Every address here was confirmed on the running board with the toolkit's own tools (--dump-ram, --trace,
// --poke, --watch; docs/FEATURES.md), after the published MAME cheat collection for this exact ROM set was
// read for a first guess, as PROGRESS.md tells in full. No game data is written down here, only where the
// game keeps it.
#pragma once

#include "game.h"

#include <cstdint>

namespace mshvsf {

// --- the board and the match ---

// Coins paid for and not yet used, a byte. The title shows it as "CREDIT(S) n[coins/2]"; holding it at 2
// turns the attract show into "PRESS 1 OR 2 START BUTTON" for both players.
constexpr uint32_t kCredits = 0xFF4C75;
constexpr uint8_t kCreditsTwo = 2;

// The match in progress: a byte that is 8 while a round is being fought (0 outside one, at the character
// choice and in the attract show), and the round's clock beside it. The clock is BCD: 0x99 reads "99", and
// it counts down from there, one a second. 0: the clock has run out.
constexpr uint32_t kMatchStatus = 0xFF4800;
constexpr uint8_t kMatchFought = 0x08;
constexpr uint32_t kClock = 0xFF4808;
constexpr uint8_t kClockFull = 0x99;
constexpr uint8_t kClockEmpty = 0x00;

// The stage's number, a byte (the low four bits). The stage's own record follows a table of signed
// offsets from 0xFF8000 (the camera lives in it): where the picture's left edge and bottom are now at
// +0x0C and +0x10 (words). The picture's left edge is the camera's across place plus 0x40.
constexpr uint32_t kStage = 0xFF4913;
constexpr int kStages = 16;
constexpr uint32_t kStageRecords = 0xFF8000;
constexpr int32_t kStageOffsets[kStages] = {-0x34C0, -0x35C0, -0x35C0, -0x3540, -0x35C0, -0x35C0, -0x34C0, -0x35C0,
                                           -0x3540, -0x3540, -0x35C0, -0x35C0, -0x35C0, -0x35C0, -0x35C0, -0x35C0};
constexpr uint32_t kCameraX = 0x0C, kCameraY = 0x10;
constexpr int kPictureLeft = 0x40;

// --- a player's record ---

// Four records, 0x400 apart: players 1 and 2, and the partner each has chosen (the partner's record is
// filled while it is out). Only the first two take part in the fight most of the time.
constexpr uint32_t kPlayer1 = 0xFF3800, kPlayerSize = 0x400;
constexpr int kPlayerRecords = 4;

constexpr uint32_t kActive = 0x00;      // byte: not zero while the record is in use
constexpr uint32_t kState = 0x06;       // word: 0 while the player is in charge of itself; 4, 8, 0x0C while
                                        // a hit (or a throw, or a knock-down) is in charge of it
constexpr uint32_t kX = 0x0C;           // signed word: where the player stands, across the stage
constexpr uint32_t kY = 0x10;           // signed word: height, the floor at 0x1D8 on the stages played
constexpr uint16_t kGround = 0x1D8;
constexpr uint32_t kFlip = 0x4B;        // byte, bit 0: the record is drawn turned about. The game's own
                                        // pictures face left, so a player facing left has the bit clear
constexpr uint32_t kCharacter = 0x53;   // byte: the character's number (even, 0x02-0x32)
constexpr uint32_t kAction = 0xA0;      // byte: what the player is doing (below)
constexpr uint32_t kActionBefore = 0xA1; // byte: the step before's kAction

enum : uint8_t {
    kActionStand = 0x00,
    kActionWalk = 0x02,
    kActionJumpStart = 0x04,
    kActionJump = 0x06,
    kActionLand = 0x08,
    kActionCrouchStart = 0x0A,
    kActionCrouch = 0x0C,
    kActionCrouchEnd = 0x0E,
    kActionIdle = 0x24,   // (a second idle the game uses, in a round and between them)
    kActionAttack = 0x28, // a normal or special move being done
    kActionReel = 0x36,   // reeling from a hit
};

// The health: what is left (a byte, 0x90 at the round's start) and the same again drawn down a step at a
// time, which is what the bar is drawn from.
constexpr uint32_t kHealth = 0x251, kHealthShown = 0x25B;
constexpr uint8_t kHealthFull = 0x90;
// The LEVEL (super) gauges kept: 0 to 3. A level is spent by a super; the cheat collections call 3 the most.
constexpr uint32_t kGauges = 0x254;
constexpr uint8_t kGaugesFull = 3;

// The boxes the game tests are in tables in the ROM, eight bytes a box: across, its half width, up, its
// half height (signed words, except the half sizes). The record has the tables' address (a long) and, for
// each box, the number of the one in use now (0: none). Bit 15 of an attack box's number marks the
// player's throw range rather than an attack that can hit; bits 13-14 are flags and are not part of the
// number (the game reads the low 13 bits).
constexpr uint32_t kBoxTable = 0x6C;       // long: where this record's box table is in the ROM
constexpr uint32_t kAttackBox = 0x70;      // word
constexpr uint32_t kAttackBox2 = 0x72;     // word
constexpr uint32_t kHurtBoxes[4] = {0x74, 0x76, 0x78, 0x7A}; // words
constexpr uint32_t kThrowBox = 0x7C;       // word: being thrown
constexpr uint16_t kBoxNumberMask = 0x1FFF;
constexpr uint16_t kThrowRangeBit = 0x8000;

// The panel as the game is told it (the port's own kRight... bits; beforeFrame sets them)
inline uint16_t Held(int n, uint16_t one, uint16_t two) {
    return n == 0 ? one : two;
}

// --- what a player is doing ---

inline uint32_t Player(int n) {
    return kPlayer1 + static_cast<uint32_t>(n) * kPlayerSize;
}

// The player faces left when its record is not turned about
inline bool FacingLeft(int n) {
    return (cps2recomp::Ram8(Player(n) + kFlip) & 1) == 0;
}

// A hit (or a throw, or a knock-down) has charge of the player
inline bool Reeling(int n) {
    return cps2recomp::Ram16(Player(n) + kState) != 0;
}

// The player is free to start something: in charge of itself and on the ground, standing, walking or
// crouching (and not between the steps of a move)
inline bool CanAct(int n) {
    using namespace cps2recomp;
    if (Reeling(n)) {
        return false;
    }
    switch (Ram8(Player(n) + kAction)) {
    case kActionStand:
    case kActionWalk:
    case kActionCrouchStart:
    case kActionCrouch:
    case kActionCrouchEnd:
    case kActionIdle:
        return true;
    default:
        return false;
    }
}

// A round is being fought: the game says so, and the clock is still running or has just been stopped.
inline bool Fighting() {
    using namespace cps2recomp;
    if (Ram8(kMatchStatus) != kMatchFought) {
        return false;
    }
    const uint8_t clock = Ram8(kClock);
    return clock <= kClockFull;
}

// The stage's record (its camera) as the picture is drawn from it
inline uint32_t StageRecord() {
    using namespace cps2recomp;
    const int stage = Ram8(kStage) & 0x0F;
    return static_cast<uint32_t>(static_cast<int32_t>(kStageRecords) + kStageOffsets[stage]);
}
inline int ScreenLeft() {
    using namespace cps2recomp;
    return static_cast<int16_t>(Ram16(StageRecord() + kCameraX)) + kPictureLeft;
}
inline int ScreenTop() {
    using namespace cps2recomp;
    return static_cast<int16_t>(Ram16(StageRecord() + kCameraY));
}

// The characters, by the number the game gives them (the game's own roster; the odd numbers are not used)
constexpr int kCharacters = 0x34;
inline constexpr const char *kCharacterNames[kCharacters] = {
    nullptr, nullptr, "Cyclops",          nullptr, "Captain America", nullptr, "Hulk",          nullptr,
    "Wolverine", nullptr, "Shuma-Gorath", nullptr, "Omega Red",      nullptr, "Spider-Man",    nullptr,
    "Blackheart", nullptr, "Ryu",         nullptr, "Ken",            nullptr, "Chun-Li",       nullptr,
    "Dhalsim", nullptr, "Zangief",        nullptr, "M. Bison",       nullptr, "Akuma",         nullptr,
    "Sakura", nullptr, "Norimaro",        nullptr, "Dan",            nullptr, "Mech-Akuma",    nullptr,
    "Mech-Zangief", nullptr, "Sunburned Sakura", nullptr, "Shadow",   nullptr, "U.S. Agent",    nullptr,
    "Mephisto", nullptr, "Armor Spider-Man",
};

// A character's name as the game numbers it ("?" for one not known)
inline const char *CharacterName(int n) {
    using namespace cps2recomp;
    const uint8_t who = Ram8(Player(n) + kCharacter);
    return who < kCharacters && kCharacterNames[who] ? kCharacterNames[who] : "?";
}

} // namespace mshvsf
