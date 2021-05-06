#include "debug.h"

#include "gui.h"

void runDebugGui(EntityId const _) {
    if(ImGui::Begin("Debug Menu")) {
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

EntityId createDebugEntity() {
    auto id = entitySystem.newEntity();
    entitySystem.addComponent(id, "Debug Entity");
    entitySystem.addComponent(id, GuiComponent{
        .runGui = runDebugGui,
    });
}