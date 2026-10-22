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

## The memory so far

_(None yet: this port has just been translated, with `make explore` driving the game's code out of the
interpreter and into translated C++. Addresses found from here on are written in this table, and the
`src/` files name only those.)_

| Address | Size | What | How it was found |
| --- | --- | --- | --- |
| _(none yet)_ | | | |

## Features built on it (`src/`)

_(None yet. Any game-side fix — a hook for a piece of the program the port has to do differently from the
board — is added in `games/mshvsf/src/` and written up here.)_

The toolkit's own features work without any of this: the launcher, the in-game menu, save states,
run-ahead, the Display settings and full control remapping.

## Next

- Translate the rest of the game's code: `make explore` (and the ladder runs) drive the made-up play
  further, so the count of instructions with no translation keeps falling.
- Find the player records (health, the LEVEL bar, the round's clock) as the sibling ports do, and build
  the features the toolkit's `fight/` parts want on top of them.
