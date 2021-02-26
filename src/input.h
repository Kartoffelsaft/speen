#pragma once

#include <SDL2/SDL.h>

#include "entitySystem.h"

struct InputComponent {
    void (*onInput)(std::vector<SDL_Event> const &, EntityId const);
};
