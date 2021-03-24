#pragma once

#include <SDL2/SDL.h>
#include <unordered_set>

#include "entitySystem.h"

struct InputState {
    std::unordered_set<SDL_Keycode> keysHeld;
    std::unordered_set<SDL_Keycode> keysJustPressed;
    std::unordered_set<Uint8> buttonsHeld;
    std::unordered_set<Uint8> buttonsJustPressed;

    int mousePosX;
    int mousePosY;

    float mousePosXNormal;
    float mousePosYNormal;

    void updateInputs();
};

struct InputComponent {
    void (*onInput)(InputState const &, EntityId const);
};

bx::Vec3 getScreenWorldPos(float x, float y);
