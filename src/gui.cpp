#include <imgui.h>

#include "gui.h"

void initGui() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
}

void drawGui() {
    ImGuiIO& io = ImGui::GetIO();
}

void terminateGui() {
    ImGui::DestroyContext();
}