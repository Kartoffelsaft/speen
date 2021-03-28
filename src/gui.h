#pragma once

#include <SDL2/SDL.h>
#include <imgui.h>

#include "entitySystem.h"

struct GuiComponent {
    void (*runGui)(EntityId const id);
};

void initGui();

bool processEventGui(SDL_Event const event);
void drawGui(float const frameTime);

void terminateGui();