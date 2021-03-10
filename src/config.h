#pragma once

#include <SDL2/SDL.h>

struct Config {
    static Config init();

    struct {
        int64_t resolutionX;
        int64_t resolutionY;
        int64_t msaa;
        bool vsync;
        int64_t renderDistance;

        int64_t shadowMapResolution;
    } graphics;

    struct {
        SDL_Keycode forward;
        SDL_Keycode back;
        SDL_Keycode left;
        SDL_Keycode right;
        SDL_Keycode up;
        SDL_Keycode down;
    } keybindings;
};

extern Config config;
