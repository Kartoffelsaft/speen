#pragma once

#include <SDL2/SDL.h>
#include <unordered_set>

struct Config {
    static Config init();

    struct {
        int64_t resolutionX;
        int64_t resolutionY;
        int64_t msaa;
        bool vsync;
        int64_t renderDistance;
        double fieldOfView;

        int64_t shadowMapResolution;
    } graphics;

    struct {
        std::unordered_set<SDL_Keycode> forward;
        std::unordered_set<SDL_Keycode> back;
        std::unordered_set<SDL_Keycode> left;
        std::unordered_set<SDL_Keycode> right;
        std::unordered_set<SDL_Keycode> up;
        std::unordered_set<SDL_Keycode> down;
    } keybindings;
};

extern Config config;
