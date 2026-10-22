// SPDX-License-Identifier: GPL-3.0-only
//
// Marvel Super Heroes vs. Street Fighter (Europe 970625): what the game's own sources add to the port
// (see PROGRESS.md). Nothing here is the game's data: only where the game keeps a thing, and any part
// of its program that the port has to do differently from the board.
//
// The toolchain reads hook addresses out of these sources: it looks for the text "AddHook(0x". A port
// with no hooks of its own (yet) registers nothing: the game runs entirely from the translated code and
// the interpreter.
#include "game.h"

namespace cps2recomp {

void RegisterGame() {}

} // namespace cps2recomp
