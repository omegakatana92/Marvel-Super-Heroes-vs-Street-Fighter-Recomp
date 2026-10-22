// SPDX-License-Identifier: GPL-3.0-only
//
// Marvel Super Heroes vs. Street Fighter (Europe 970625): what the game's own sources add to the port
// (see PROGRESS.md). Nothing here is the game's data: only where the game keeps a thing, and any part
// of its program that the port has to do differently from the board.
#include "game.h"

namespace cps2recomp {

void RegisterCheats(); // cheats.cpp
void CheatsFrame(Machine &machine);
void CheatsState(Archive &ar);
void RegisterTraining(); // training.cpp
void TrainingBeforeFrame(Machine &machine);
void TrainingFrame(Machine &machine);
void TrainingState(Archive &ar);

void RegisterGame() {
    RegisterCheats();
    RegisterTraining();
    TheGame().beforeFrame = TrainingBeforeFrame;
    // (the training aids see what a frame did before the cheats put things back)
    TheGame().frame = [](Machine &machine) {
        TrainingFrame(machine);
        CheatsFrame(machine);
    };
    TheGame().state = [](Archive &ar) {
        TrainingState(ar);
        CheatsState(ar);
    };
}

} // namespace cps2recomp
