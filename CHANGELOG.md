# Marvel Super Heroes vs. Street Fighter Recomp — changelog

## 0.1.0-beta.1

First build. The 68000 and Z80 programs of *Marvel Super Heroes vs. Street Fighter* (Europe 970625)
are translated to C++ ahead of time and run on cps2recomp's own machine, with no emulator. The
launcher, the in-game menu, save states, run-ahead, the display options and every control are
remappable.

This is a beta of the translation: the game's own features (free play, cheats and the training aids)
still wait on its memory being mapped, which is the next step in [ROADMAP.md](ROADMAP.md). The
translated code is checked against the interpreter by `make check`.
