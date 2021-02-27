#pragma once

#include <SDL2/SDL.h>
#include <unordered_set>

#include "entitySystem.h"

struct InputState {
    std::unordered_set<SDL_Keycode> keysHeld;
    std::unordered_set<SDL_Keycode> keysJustPressed;

    void updateInputs();
};

struct InputComponent {
    void (*onInput)(InputState const &, EntityId const);
};
