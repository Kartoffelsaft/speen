#pragma once

#include <SDL2/SDL.h>
#include <bx/math.h> // NOLINT(modernize-deprecated-headers)
#include <unordered_set>
#include <variant>

#include "entitySystem.h"

using InputDatum = std::variant<SDL_Keycode, Uint8>;

struct InputState {
    std::unordered_set<InputDatum> inputsHeld;
    std::unordered_set<InputDatum> inputsJustPressed;

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
