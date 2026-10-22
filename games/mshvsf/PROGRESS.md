# Marvel Super Heroes vs. Street Fighter: progress

What is known of *Marvel Super Heroes vs. Street Fighter* (Europe 970625)'s memory, and what has been
built on it. Nothing here is a dump of the game's data, and no game data is written down: only where the
game keeps a thing, found by experiment on the running board with the toolkit's own tools
(`docs/FEATURES.md`).

## The ROM set

The set is MAME's **`mshvsf`** (Europe 970625), the parent of `mshvsfu`, `mshvsfu1`, `mshvsfj`,
`mshvsfj1`, `mshvsfj2`, `mshvsfh`, `mshvsfa`, `mshvsfa1` and `mshvsfb`. It was told apart from the
clones, and its files listed, from the set's own contents against MAME's ROM definition for `mshvsf`
(Capcom, 1997, board `cps2_2p6b`, 2 players, 6 buttons):

- the program is eight 512 KiB ROMs — `mvse.03f`, `mvse.04f`, `mvs.05a` … `mvs.10b` — 4 MiB in all, so
  `code_end = 400000`;
- the graphics are eight 4 MiB ROMs in two rows of four — `mvs.13m`, `mvs.15m`, `mvs.17m`, `mvs.19m`
  then `mvs.14m`, `mvs.16m`, `mvs.18m`, `mvs.20m` — 32 MiB, side by side, and the graphics region is
  populated from 0 (no leading hole, unlike Street Fighter Alpha);
- the sound program is `mvs.01`, `mvs.02`; the QSound samples are `mvs.11m`, `mvs.12m` (8 MiB);
- the key is `mshvsf.key`.

Every file is checked by CRC-32 (`games/mshvsf/game.cfg`), so only this revision is accepted.

## The translation

`make explore` drove the game headless in rounds until the count of instructions with no translation
stopped falling. The last round reported:

```
recomp: 1550 entries -> 146435 instructions (579008 bytes) in 36044 blocks, 8676 routines, 92 files;
        64 left to the interpreter (28 of them not instructions: data a guess took for code)
recompz80: 337 entries -> 4489 instructions (8098 bytes) in 843 blocks, 62 routines;
        0 left to the interpreter
```

`make check` then showed the translated code matching the interpreter by state hash over 3000 frames,
and two runs of the same input producing the same hash. A 1200-frame headless run of the finished
build left 33 instructions in the interpreter (about 0.000% of the run); they are on paths the explore
runs did not reach, and the count falls as those are played.

## How the game's own memory is found

The tools used, in the order they are reached for (all in `docs/FEATURES.md`):

- `--menu FRAME:KEYS` drives the in-game menu with no window: a coin goes in without a hand.
- `--movie FILE` replays a hand-written input movie, and `--poke ADDR=BYTE` holds a byte at a guess; the
  two together are how the game is driven to a known place and a thought tried.
- `--dump-ram FRAME:FILE` writes the 64 KiB of work RAM after a frame; `--trace LOW-HIGH:FILE` appends a
  stretch of work RAM (or all of it) after every frame; `--shot-every N:PFX` writes pictures of the board.
- `cps2recomp/tools/ramfind.py` lists the addresses that held a known value in each of several dumps;
  `cps2recomp/tools/tracechanges.py` prints a trace where a value changes.
- `--watch ADDRESS` (with `--cpu interp`) reports every write to an address and the instruction that made
  it, read against `work/mshvsf/program.lst`.

The two published collections for this same set were read for a first guess, and that is said where it
is so: the MAME cheat collection's `mshvsf.xml`, and the Marvel hit-box script that the MAME-rr/FBNeo
training forks carry (`marvel-hitboxes.lua`). **Every address below was then checked on the running
board** with the tools above: held at a value with `--poke` and read off the picture, or seen to move
with a known action in a `--trace`. No value was taken on trust from either.

## The memory so far

| Address | Size | What | How it was found |
| --- | --- | --- | --- |
| `FF4C75` | byte | credits paid for and not yet used | Four title-screen dumps at 0, 1, 2 and 4 coins; the byte that read 2, 1 and 0 as the credits did. `--poke FF4C75=05` then made the title read `CREDITS 5[1/2]` |
| `FF4800` | byte | the match: 8 while a round is being fought, 0 at the character choice and in the attract show | `--trace` through a whole 2-player match: 8 from the round's start to its end, and 0 at the choice. The published hit-box script calls it the match status |
| `FF4808` | byte, BCD | the round's clock: `0x99` at the start, one less a second | `--trace` of the whole RAM beside `--shot-every`: the byte fell `0x96, 0x95, 0x94 …` at the frames the picture read 96, 95, 94. Holding `0x99` with `--poke` kept the picture at 99 for a whole round |
| `FF4913` | byte | the stage's number (low four bits) | Read beside the stage's record and the camera; it changed with the stage chosen, and the record it selects put both players where the picture showed them |
| `FF8000 + stage offset` | record | the stage's camera: `+0x0C` and `+0x10` (words) are where the picture's left edge and top are; the left edge is that plus `0x40` | The published hit-box script's table of offsets; the record's two words were then checked against a match: `x = 263` put the picture's left edge at 327 and put player 1 at 210 pixels, which is where the picture has him. The offsets of the two stages played (4 and 7) were checked; the others are the script's and have not been seen yet |
| `FF3800` | record, `0x400` apart | four player records: players 1 and 2, then the partner each has chosen | The published hit-box script's base, checked by changing what the game is told a player holds and watching `+0x0C` (across) and `+0x10` (height) move as the player did, and by reading each record's character number at the character choice |
| `+0x251`, `+0x25B` | bytes | a player's health, and the same drawn down a step at a time (what the bar is drawn from); `0x90` at a round's start | The MAME cheat collection's "Infinite Energy" addresses; held at `0x90` with `--poke` and the bar stayed full while the computer attacked for 800 frames. A `--trace` of a hit showed both bytes fall, the second trailing the first |
| `+0x254` | byte | the LEVEL (super) gauges kept, 0 to 3 | The MAME cheat collection's "Always Max Power" address; `--poke FF3A54=03` (and the other record) made both LEVEL bars read 3 and stay full |
| `+0x53` | byte | the character's number (even, `0x02`-`0x32`) | The MAME cheat collection's character-select addresses; read at the character choice, where the records held the numbers of the four characters chosen |
| `+0x4B` | byte | how the record is turned about: bit 0 set when it is mirrored (the game's pictures face left, so a player facing left has it clear) | A `--trace` of two players walking through each other: the bit turned over exactly as they passed |
| `+0x06` | word | who is in charge of the player: 0 while it is itself; 4, 8 and `0x0C` while a hit, a throw or a knock-down is | A `--trace` of a round in which player 1 stood still: the word went 0 → `0x0C` on the frame a hit landed, and back to 0 as the player could act again. The published hit-box script tests the same word |
| `+0x0A0` | byte | what the player is doing: `0x00` standing, `0x02` walking, `0x04`/`0x06` a jump, `0x08` landing, `0x0A`/`0x0C`/`0x0E` crouching, `0x22` getting up, `0x24` guarding a hit (the blockstun pose), `0x28` a move being done, `0x2E` knocked down and lying, `0x36` reeling | A two-player match with player 2 standing still, driven through walk, crouch, jump, six buttons, a hit, a sweep and a knocked-down get-up, with a `--trace` and a picture every ten frames; each value matched the action in the picture |
| `+0x060` | word | the move being done: `0x1000`, `0x1002`, `0x1004` are light, medium and heavy punches; `0x1200`, `0x1202`, `0x1204` the three kicks; a special has a code of its own | Six single presses, one a run, with a `--trace`: the word was constant through each move and different for every button |
| `+0x6C` | long | where this record's box table is in the ROM | The published hit-box script; the table's entries were read from the program and drawn over the players, and a box's middle and size landed on the body part it belongs to |
| `+0x70`, `+0x72` | words | the two attack boxes out; 0 none. Bit 15 set: the player's throw range, not an attack that can hit. Bits 13-14 are flags (the game reads the low 13 bits) | Same: the words were 0 at rest, became `0x21`, `0x3F` and so on over the frames of a punch, and the box they name is where the fist was |
| `+0x74`, `+0x76`, `+0x78`, `+0x7A` | words | the four boxes a player can be hit in | Same: their table entries sit on the body as the game draws it |
| `+0x7C` | word | the box a player can be thrown from | Same |
| `+0xA4` | byte | the pushing box's number; its boxes are in a table of their own at `0x137EE2` (a long a number, then four bytes a character) | The published hit-box script; the box read from the table sat exactly where the two players' feet pushed against each other, and was drawn as a 54-pixel box around each body |
| `+0x064` | word | the move's step: `0x8000` while it can be part of the move, `0x4000` in its recovery | A `--trace` of one punch frame by frame: the word held `0x8000` through the attack box and turned to `0x4000` as it went away, before the action went back to standing |
| `+0x0A1`, `+0x0A7` | bytes | the action before, and a move's count (1, 2, 3 … as moves are done) | The same trace: `+0xA1` is the previous step's `+0xA0`; `+0xA7` ticks once a move |

The clock deserves a note: it is BCD, so `0x99` is the picture's "99" and `0x90` is "90". It steps
down about every 94 frames, which is this board's own count, not one a second of the port's time.

### What a guard, a knock-down and a get-up are

Found by holding away on the second player's controls (and down and away for a low) while player 1
attacked, in a two-player match where neither side was the computer:

- **A guard** is the stick held away from the attacker when the attack is tested; the game takes no
  health off, and the player is put in the blockstun pose, action `0x24`, for its duration (14 frames
  for a standing heavy punch). A **low** attack must be guarded with down and away: a standing guard
  lets it through.
- **A knock-down** is action `0x2E` (lying on the floor); **getting up** is `0x22`, then the player is
  free again. A sweep (down and a heavy kick) puts the other player down this way, whatever the guard.
  The state word `+0x06` is 0 throughout, so the action is what tells a knock-down from a hit's stun.

The toolkit's frame counting is told all of this: a guard is `Reeling` with `Contact(victim, guarded)`,
so the advantage after a guarded hit is worked out and written to `frame_data.csv`; a knock-down is
`Held`, so it is not counted as a move.

## Features built on it (`src/`)

| Feature | Category | What it does | Where |
| --- | --- | --- | --- |
| Free play | Game | Holds the credits at two, so Start works without a coin and the attract show gives way to the title | `cheats.cpp` (`FF4C75`) |
| No damage, no damage (player 2) | Cheats | Keeps a player's health where the round began (both stores), so hits do not take it | `cheats.cpp` (`+0x251`, `+0x25B`) |
| Infinite time | Cheats | Holds the clock at the second the round began | `cheats.cpp` (`FF4808`) |
| One-hit knockouts | Cheats | Leaves player 2 one point of health, so any hit ends the round | `cheats.cpp` |
| Full LEVEL bars | Cheats | Holds every gauge of both players full, so a super is always ready | `cheats.cpp` (`+0x254`) |
| Input display | Training | What each player holds and for how many frames, as a keypad number and the six buttons, turned so that "towards" is always 6 | `training.cpp` (`+0x4B`, `+0x0C`) |
| Frame data | Training | Startup, active and recovery of each move, from when an attack box is out; the numbers stay up after the move | `training.cpp` (`+0x0A0`, `+0x70`, `+0x72`) |
| Frame meter | Training | A row of cells a step, coloured by what each player was doing | `training.cpp` (the toolkit's `fight::Frames`) |
| Frame data log | Training | Writes each move to `frame_data.csv` beside the settings file, with the game's own move number | `training.cpp` (`+0x60`) |
| Combo readout | Training | What each hit took off, the combo and the round's best, from the health dropping | `training.cpp` (`+0x251`) |
| Hit boxes | Training | Draws the boxes the game tests: where a player can be hit (blue), attacks (red), a throw's range (yellow), a throwable box (white) and the pushing box (green), placed as the camera shows them | `training.cpp` (`+0x6C` table, `+0x70`–`+0x7C`, `+0xA4` and `0x137EE2`) |
| Health refills | Training | Health comes back after a moment without being hit | `training.cpp` |
| Dummy: stands, crouches, jumps | Dummy | Player 2 does what is chosen, or nothing | `training.cpp`, the toolkit's `fight::Dummy` |
| Dummy: records and replays | Dummy | Player 1's controls work player 2 while recording (three slots, 20 seconds each), then player 2 does it again and again | `training.cpp` |
| Dummy: guards | Dummy | Holds away when player 1 starts an attack near enough (a low one crouching, a jump-in standing), always, after being hit, or at random; may press buttons while guarding; answers with a recording after guarding or being hit | `training.cpp` (`+0x0A0` guard pose, `+0x0C`/`+0x10` places) |

With every one of these off, the game runs exactly as the board does: they work on the game's memory
after every frame, and nothing is hooked or rewritten yet.

## What is not known yet

- **The damage rule.** The store that takes a hit off a player's health is at `0x018536` (found with
  `--watch FF3E51` under `--cpu interp`), so the routine around it is the place to start; it has not
  been read through, so the combo readout takes its damage from the health dropping, and how much of a
  hit comes back by itself is not shown.
- **Projectiles.** Fireballs and the like have records of their own; none was produced in the runs so
  far (the special-move animations were reached, but the published pointer list showed only the
  players), so their boxes are not drawn or counted.
- **The operator settings** (difficulty, round time, free play in the EEPROM) and the game's own test
  menu are not mapped.
- **The other stage records.** Only the stages played have been checked against the camera table.
- **The game's own guard-range test.** The dummy guards on a distance to player 1's attack box: it
  works, but the game's own test (which an attack must pass for a guard to be possible) is not found.
- **The other moves' numbers.** The six normals are told apart by `+0x60`; the specials' codes are
  seen (`0x2A` was one) but not named.

## Next

- Read the damage rule through (from `0x018536`) and show what of a hit comes back.
- Find a projectile and its records, for the boxes and the combo count.
- Map the operator settings and the test menu.
- Then the readable rewrites and widescreen (ROADMAP.md), which want the same memory.
