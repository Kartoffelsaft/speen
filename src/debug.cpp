#include "debug.h"

#include <bgfx/bgfx.h>

#include "gui.h"
#include "input.h"
#include "config.h"

bool debugMenuEnabled = false;

void entityListGui() {
    if(!ImGui::CollapsingHeader("Entity List")) return;

    if(ImGui::BeginTable("Entity List", 2)) {
        for(auto id: entitySystem.entities) {
            ImGui::TableNextRow();
            
            ImGui::TableNextColumn();
            ImGui::Text("%d", id);

            ImGui::TableNextColumn();
            if(entitySystem.entityHasComponent<char const *>(id)) {
                ImGui::Text(entitySystem.getComponentData<char const *>(id));
            } else {
                ImGui::Text("Unnamed Entity");
            }
        }

        ImGui::EndTable();
    }
}

void entityReaderGui() {
    if(!ImGui::CollapsingHeader("Entity Reader")) return;

    static int id = 0;
    ImGui::InputInt("Id", &id);
    if(!entitySystem.entities.contains(id)) ImGui::Text("WARNING: Entity not found");

    if(ImGui::BeginTable("Entity Info", 2)) {
        for(auto const & [name, location]: entitySystem.entityInfo(id)) {
            ImGui::TableNextRow();

            ImGui::TableNextColumn();
            ImGui::Text("%s", name);

            ImGui::TableNextColumn();
            ImGui::Text("%p", location);
        }

        ImGui::EndTable();
    }
}

void runDebugGui(EntityId const _) {
    if(debugMenuEnabled && ImGui::Begin("Debug Menu")) {
        entityListGui();
        entityReaderGui();

        ImGui::End();
    }
}

void debugOnInput(InputState const & inputs, EntityId const id) {
    for(auto const & inp: inputs.inputsJustPressed) {
        if(config.keybindings.toggleDebugMenu.contains(inp)) debugMenuEnabled ^= 1;
    }

    bgfx::setDebug(debugMenuEnabled? BGFX_DEBUG_STATS : 0);
}

EntityId createDebugEntity() {
    auto id = entitySystem.newEntity();
    entitySystem.addComponent(id, "Debug Entity");
    entitySystem.addComponent(id, GuiComponent{
        .runGui = runDebugGui,
    });
    entitySystem.addComponent(id, InputComponent{
        .onInput = debugOnInput,
    });
}