#include "pointer.h"

#include "input.h"
#include "physics.h"
#include "modelInstance.h"
#include "enemy.h"
#include "config.h"

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

        }
    };
}

PhysicsComponent pointerPhysicsComponent() {
    return {
        .collidable = pointerCollidable()
    };
}

EntityId createPointer() {
    auto pointer = entitySystem.newEntity();
    entitySystem.addComponent(pointer, "Donut Pointer");
    entitySystem.addComponent(pointer, ModelInstance::fromModelPtr(LOAD_MODEL("donut.glb")));
    entitySystem.addComponent(pointer, InputComponent{
        .onInput = pointerOnInput,
    });
    entitySystem.addComponent(pointer, pointerPhysicsComponent());

    return pointer;
}