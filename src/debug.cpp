#include "debug.h"

#include "gui.h"
#include "input.h"
#include "config.h"

bool debugMenuEnabled = false;

void runDebugGui(EntityId const _) {
    if(debugMenuEnabled && ImGui::Begin("Debug Menu")) {
        if(ImGui::CollapsingHeader("Entity List") && ImGui::BeginTable("Entity List", 2)) {
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

        ImGui::End();
    }
}

void debugOnInput(InputState const & inputs, EntityId const id) {
    for(auto const & inp: inputs.inputsJustPressed) {
        if(config.keybindings.toggleDebugMenu.contains(inp)) debugMenuEnabled ^= 1;
    }
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