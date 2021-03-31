#include "player.h"

#include "entitySystem.h"
#include "input.h"
#include "physics.h"
#include "gui.h"
#include "config.h"
#include "chunk.h"
#include "rendererState.h"
#include "modelInstance.h"

#define PLAYER_MODEL "mokey.glb"

static EntityId playerId;

struct InventoryItem {
    int data;
};

static std::vector<InventoryItem> inventory;

void playerOnInput(InputState const & inputs, EntityId const id) {
    auto& obj = entitySystem.getComponentData<PhysicsComponent>(id);

    obj.velocity.x = 0.f;
    obj.velocity.y = 0.f;
    obj.velocity.z = 0.f;

    for(auto inp: inputs.inputsHeld) {
        if(config.keybindings.forward.contains(inp)) {
            obj.velocity.x = -8.f;
        } if(config.keybindings.back.contains(inp)) {
            obj.velocity.x = +8.f;
        } if(config.keybindings.left.contains(inp)) {
            obj.velocity.z = -8.f;
        } if(config.keybindings.right.contains(inp)) {
            obj.velocity.z = +8.f;
        } if(config.keybindings.up.contains(inp)) {
            obj.velocity.y = +8.f;
        } if(config.keybindings.down.contains(inp)) {
            obj.velocity.y = -8.f;
        }
    }

    int chunkX = ((int)obj.position.x) / 16;
    int chunkZ = ((int)obj.position.z) / 16;

    rendererState.setCameraOrientation(
        obj.position + Vec3{5.f, 7.f, 5.f},
        obj.position
    );
    rendererState.setLightOrientation(
        {(float)chunkX * 16 - 48, 19, (float)chunkZ * 16 + 10},
        {(float)chunkX * 16, 0, (float)chunkZ * 16},
        50,
        80
    );
    world.updateModel(chunkX, chunkZ, config.graphics.renderDistance);
}

void playerRunGuiInventory() {
    ImGui::Begin("Inventory");
        ImGui::BeginTable("Inventory Table", 1);

        ImGui::TableSetupColumn("Data");
        for(int i = 0; i < inventory.size(); i++) {
            ImGui::PushID(i);

            ImGui::TableNextRow();
            ImGui::TableNextColumn();

            ImGui::Text("%08d", inventory.at(i));

            if(ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
                ImGui::SetDragDropPayload("Inventory Index", &i, sizeof(i));
                ImGui::EndDragDropSource();
            }

            if(ImGui::BeginDragDropTarget()) {
                if(ImGuiPayload const * p = ImGui::AcceptDragDropPayload("Inventory Index")) {
                    auto const si = *(int const *)p->Data;

                    std::swap(inventory.at(si), inventory.at(i));
                }
                ImGui::EndDragDropTarget();
            }

            ImGui::PopID();
        }

        ImGui::EndTable();
    ImGui::End();
}

void playerOnCollision(EntityId const id, EntityId const otherId) {
    entitySystem.removeEntity(otherId);
    inventory.push_back(InventoryItem{.data = otherId,});
}

void playerRunGui(EntityId const id) {
    playerRunGuiInventory();
}

ModelInstance playerComponentModel() {
    return ModelInstance::fromModelPtr(LOAD_MODEL(PLAYER_MODEL));
}

PhysicsComponent playerComponentPhysics() { 
    auto ret = PhysicsComponent{};
    ret.grounded = true;
    ret.collidable.emplace(Collidable{
        .collisionRange = 1.f,
        .onCollision = playerOnCollision,
    });
    return ret;
}

InputComponent playerComponentInput() {
    return InputComponent{
        .onInput = playerOnInput,
    };
}

GuiComponent playerComponentGui() {
    return GuiComponent{
        .runGui = playerRunGui,
    };
}

EntityId createPlayer() { 
    playerId = entitySystem.newEntity();

    entitySystem.addComponent(playerId, playerComponentModel());
    entitySystem.addComponent(playerId, playerComponentPhysics());
    entitySystem.addComponent(playerId, playerComponentInput());
    entitySystem.addComponent(playerId, playerComponentGui());

    return playerId;
}