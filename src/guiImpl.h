#pragma once

#include <SDL2/SDL.h>
#include <imgui.h>

void ImGui_Implbgfx_Init();
void ImGui_Implbgfx_NewFrame();
void ImGui_Implbgfx_Shutdown();
void ImGui_Implbgfx_RenderDrawLists(ImDrawData* draw_data);

void ImGui_ImplSDL2_Init();
void ImGui_ImplSDL2_NewFrame(float const frameTime);
void ImGui_ImplSDL2_Shutdown();
bool ImGui_ImplSDL2_ProcessEvent(SDL_Event const event);