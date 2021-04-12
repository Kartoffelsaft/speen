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

enum Weapon {
    BulletWeapon,
    GrenadeWeapon,
};

char const * weaponAsString(Weapon const weapon) {
    switch (weapon) {
        case BulletWeapon : return "Gun";
        case GrenadeWeapon: return "Grenade Launcher";
    }

    // It's undefined behavior here. I'm just silencing a compiler warning
    return nullptr;
}

struct InventoryItem {
    std::variant<Weapon> item;
};

struct {
    Weapon weapon;
} equipment;

static std::vector<InventoryItem> inventory;

constexpr PhysicsComponent weaponProjectilePhysicsComponent(Weapon const weapon, Vec3 const & from, Vec3 const & to) {
    auto defaultPhysics = PhysicsComponent{
        .position = from,
        .velocity = (to - from).normalized() * 20.f,
        .accelleration = Vec3{0.0f, 0.0f, 0.0f},
        .type = PhysicsType::Floating,
        .collidable = Collidable{
            .collisionRange = 0.1f,
            .layer = 0b0000'1000,
            .mask = 0b0000'1001,
            .onCollision = [](EntityId const id, EntityId const otherId) {
                entitySystem.removeEntity(id);
                entitySystem.removeEntity(otherId);
                inventory.emplace_back(InventoryItem{
                    .item = Weapon::GrenadeWeapon,
                });
            }
        }
    };

    switch(weapon) {
        case Weapon::BulletWeapon:
            break;
        case Weapon::GrenadeWeapon:
            defaultPhysics.accelleration.y = -9.f;
            defaultPhysics.type = PhysicsType::Bouncy;
            break;
    }

    return defaultPhysics;
}

void shootAt(Vec3 const & at) {
    auto const & playerPhysics = entitySystem.getComponentData<PhysicsComponent>(playerId);
    auto from = playerPhysics.position + Vec3{0, 1, 0};
    auto to = at + Vec3{0, 0.3, 0};

    auto bulletId = entitySystem.newEntity();

    entitySystem.addComponent<ModelInstance>(bulletId, ModelInstance::fromModelPtr(LOAD_MODEL("bullet.glb")));

    Mat4 tmp;
    bx::mtxLookAt(tmp.data(), to, from);
    bx::mtxInverse(entitySystem.getComponentData<ModelInstance>(bulletId).orientation.data(), tmp.data());

    entitySystem.addComponent<PhysicsComponent>(bulletId, weaponProjectilePhysicsComponent(equipment.weapon, from, to));
}

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

    for(auto inp: inputs.inputsJustPressed) {
        if(config.keybindings.attack.contains(inp)) {
            shootAt(getScreenWorldPos(inputs.mousePosXNormal, inputs.mousePosYNormal));
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

char const * const DRAG_DROP_INVENTORY_INDEX = "Inventory Index";

void playerRunGuiInventory() {
    if(ImGui::Begin("Inventory")) {
        ImGui::BeginTable("Inventory Table", 1);

        ImGui::TableSetupColumn("Data");
        for(std::size_t i = 0; i < inventory.size(); i++) {
            ImGui::PushID(i);

            ImGui::TableNextRow();
            ImGui::TableNextColumn();

            ImGui::Text("%s", weaponAsString(std::get<0>(inventory.at(i).item)));

            if(ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
                ImGui::SetDragDropPayload(DRAG_DROP_INVENTORY_INDEX, &i, sizeof(i));
                ImGui::EndDragDropSource();
            }

            if(ImGui::BeginDragDropTarget()) {
                if(ImGuiPayload const * p = ImGui::AcceptDragDropPayload(DRAG_DROP_INVENTORY_INDEX)) {
                    auto const si = *(int const *)p->Data;

                    if(si < (int)i) {
                        std::rotate(inventory.begin() + si, inventory.begin() + si + 1, inventory.begin() + i + 1);
                    } else {
                        std::rotate(inventory.begin() + i, inventory.begin() + si, inventory.begin() + si + 1);
                    }
                }
                ImGui::EndDragDropTarget();
            }

            ImGui::PopID();
        }

        ImGui::EndTable();
    }
    ImGui::End();
}

void playerRunGuiEquipment() {
    if(ImGui::Begin("Equipment")) {
        ImGui::PushID("Weapon");

        ImGui::Button(
            equipment.weapon == Weapon::BulletWeapon?
            "Gun" :
            "Grenade Launcher",
            ImVec2(150, 50)
        );

        if(ImGui::BeginDragDropTarget()) {
            if(ImGuiPayload const * p = ImGui::AcceptDragDropPayload(DRAG_DROP_INVENTORY_INDEX)) {
                auto swapInv = InventoryItem{
                    .item = equipment.weapon,
                };
                auto i = *(int const *)p->Data;
                std::swap(swapInv, inventory.at(i));
                equipment.weapon = std::get<Weapon>(swapInv.item);
            }

            ImGui::EndDragDropTarget();
        }

        ImGui::PopID();
    }
    ImGui::End();
}

void playerOnCollision(EntityId const id, EntityId const otherId) {
    // Nothing happens for now
}

void playerRunGui(EntityId const id) {
    playerRunGuiInventory();
    playerRunGuiEquipment();
}

ModelInstance playerComponentModel() {
    return ModelInstance::fromModelPtr(LOAD_MODEL(PLAYER_MODEL));
}

PhysicsComponent playerComponentPhysics() { 
    auto ret = PhysicsComponent{};
    ret.type = PhysicsType::Grounded;
    ret.collidable.emplace(Collidable{
        .collisionRange = 1.f,
        .layer = 0b0000'0010,
        .mask  = 0b0000'0010,
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