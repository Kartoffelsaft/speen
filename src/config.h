#pragma once

#include <SDL2/SDL.h>
#include <unordered_set>

#include "input.h"

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
        std::unordered_set<InputDatum> forward;
        std::unordered_set<InputDatum> back;
        std::unordered_set<InputDatum> left;
        std::unordered_set<InputDatum> right;
        std::unordered_set<InputDatum> up;
        std::unordered_set<InputDatum> down;
        std::unordered_set<InputDatum> place;
        std::unordered_set<InputDatum> attack;

        std::unordered_set<InputDatum> toggleDebugMenu;
    } keybindings;

    struct {
        bool debug;
    } misc;
};

extern Config config;
