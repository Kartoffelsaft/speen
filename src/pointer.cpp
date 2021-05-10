#include "pointer.h"

#include <optional>

#include "input.h"
#include "physics.h"
#include "modelInstance.h"
#include "enemy.h"
#include "config.h"
#include "health.h"
#include "gui.h"
#include "frame.h"

std::optional<HealthComponent> pointeeHealth;

void pointerOnInput(InputState const & inputs, EntityId const id) {
    auto& pointerPhysics = entitySystem.getComponentData<PhysicsComponent>(id);
    pointerPhysics.position = getScreenWorldPos(inputs.mousePosXNormal, inputs.mousePosYNormal);

    for(auto inp: inputs.inputsJustPressed) if(config.keybindings.place.contains(inp)) {
        createEnemy(pointerPhysics.position);
    }
}

Collidable pointerCollidable() {
    return {
        .collisionRange = 0.f,
        .layer = 0b0000'0000,
        .mask = 0b1111'1111,
        .onCollision = [](EntityId const id, EntityId const otherId) {
            if(entitySystem.entityHasComponent<HealthComponent>(otherId)) {
                pointeeHealth = entitySystem.getComponentData<HealthComponent>(otherId);
            }
        }
    };
}

PhysicsComponent pointerPhysicsComponent() {
    return {
        .collidable = pointerCollidable()
    };
}

OnFrameComponent pointerOnFrameComponent() {
    return {
        .onFrame = [](EntityId const _id, float _delta) {
            pointeeHealth = std::nullopt;
        },
    };
}

void pointerRunGui(EntityId const _) {
    if(pointeeHealth) {
        auto& style = ImGui::GetStyle();
        auto pos = ImGui::GetIO().MousePos;
        pos.x -= 50;
        pos.y += 20;
        float const hWidth = 100;
        float const hHeight = 35;
        float const filledToWidth = (hWidth - 2 * style.WindowPadding.x) * pointeeHealth->health / pointeeHealth->maxHealth + style.WindowPadding.x;
        ImGui::SetNextWindowPos(pos, ImGuiCond_Always);
        ImGui::SetNextWindowSizeConstraints(ImVec2(hWidth, hHeight), ImVec2(hWidth, hHeight));

        if(ImGui::Begin(
            "Health Bar", 
            nullptr, 
            ImGuiWindowFlags_NoDecoration 
          | ImGuiWindowFlags_NoSavedSettings
          | ImGuiWindowFlags_NoFocusOnAppearing
        )) {
            ImGui::GetForegroundDrawList()->AddRectFilled(
                ImVec2(pos.x + style.WindowPadding.x, pos.y + style.WindowPadding.y), 
                ImVec2(pos.x + filledToWidth, pos.y + hHeight - style.WindowPadding.y), 
                ImColor(ImVec4(1.f, 0.f, 0.f, 1.f))
            );

            ImGui::End();
        }
    }
}

EntityId createPointer() {
    auto pointer = entitySystem.newEntity();
    entitySystem.addComponent(pointer, "Donut Pointer");
    entitySystem.addComponent(pointer, ModelInstance::fromModelPtr(LOAD_MODEL("donut.glb")));
    entitySystem.addComponent(pointer, InputComponent{
        .onInput = pointerOnInput,
    });
    entitySystem.addComponent(pointer, pointerPhysicsComponent());
    entitySystem.addComponent(pointer, GuiComponent{
        .runGui = pointerRunGui,
    });
    entitySystem.addComponent(pointer, pointerOnFrameComponent());

    return pointer;
}