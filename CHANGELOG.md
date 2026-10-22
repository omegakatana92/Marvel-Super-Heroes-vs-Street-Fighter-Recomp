# Marvel Super Heroes vs. Street Fighter Recomp — changelog

## 0.2.0-beta.1

The game's own memory is mapped, and the first features built on it are in: the cheats, free play and
the training aids. They are optional; with every one off the game runs exactly as the board does.

**Found on the running board** (how each was found is in `games/mshvsf/PROGRESS.md`): the credits, the
round's clock (BCD, `0x99` at most), the match's state and the stage's camera; each player's record with
the health and the health the bar is drawn from, the LEVEL (super) gauges, the character's number, the
facing, the position, what a player is doing (standing, walking, crouching, jumping, attacking, reeling)
and the boxes the game tests, with the box tables in the ROM.

**Features**

- Free play.
- No damage, for each player; one-hit knockouts; infinite time; full LEVEL bars.
- Input display, frame data, a frame meter, a frame data log (`frame_data.csv`), a combo readout, health
  refills, and the hit boxes drawn over the picture (hurt, attack, a throw's range, throwable).

**Still to come** (`ROADMAP.md`): the training mode's dummy, the advantage after a guarded hit, the
push box and the projectiles' boxes, the operator settings, the readable rewrites and widescreen.

## 0.1.0-beta.1

First build. The 68000 and Z80 programs of *Marvel Super Heroes vs. Street Fighter* (Europe 970625)
are translated to C++ ahead of time and run on cps2recomp's own machine, with no emulator. The
launcher, the in-game menu, save states, run-ahead, the display options and every control are
remappable.

This is a beta of the translation: the game's own features (free play, cheats and the training aids)
still wait on its memory being mapped, which is the next step in [ROADMAP.md](ROADMAP.md). The
translated code is checked against the interpreter by `make check`.
