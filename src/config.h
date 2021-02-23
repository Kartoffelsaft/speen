#pragma once

#include <SDL2/SDL.h>

struct Config {
    static Config init();

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
