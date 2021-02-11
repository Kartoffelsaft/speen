#pragma once

#include <SDL2/SDL.h>

#include "model.h"

struct RendererState {
    static RendererState init();

    uint64_t frame = 0;
    int const WINDOW_WIDTH = 1280;
    int const WINDOW_HEIGHT = 720;
    char const * const WINDOW_NAME = "First bgfx";

    SDL_Window* window;
    ModelLoader modelLoader;
};

extern RendererState rendererState;
