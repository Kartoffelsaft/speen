#include <imgui.h>
#include <bgfx/bgfx.h>
#include <bx/math.h> 

#include "gui.h"
#include "guiImpl.h"
#include "entitySystem.h"

void initGui() {
    ImGui::CreateContext();

    ImGui_Implbgfx_Init();
    ImGui_ImplSDL2_Init();
}

bool processEventGui(SDL_Event const event) {
    return ImGui_ImplSDL2_ProcessEvent(event);
}

void drawGui(float const frameTime) {
    ImGui_Implbgfx_NewFrame();
    ImGui_ImplSDL2_NewFrame(frameTime);

    ImGui::NewFrame();

    for(auto const & id: entitySystem.filterByComponent<GuiComponent>()) {
        entitySystem.getComponentData<GuiComponent>(id).runGui(id);
    }

    ImGui::ShowDemoWindow();
    ImGui::Render();
    ImGui_Implbgfx_RenderDrawLists(ImGui::GetDrawData());
}

void terminateGui() {
    ImGui_Implbgfx_Shutdown();
    ImGui_ImplSDL2_Shutdown();

    ImGui::DestroyContext();
}