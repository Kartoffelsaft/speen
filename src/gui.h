#pragma once

#include <SDL2/SDL.h>

void initGui();

bool processEventGui(SDL_Event const event);
void drawGui();

void terminateGui();