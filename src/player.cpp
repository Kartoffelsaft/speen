#include "player.h"

#include "entitySystem.h"
#include "input.h"
#include "physics.h"
#include "config.h"
#include "chunk.h"
#include "rendererState.h"
#include "modelInstance.h"

#define PLAYER_MODEL "mokey.glb"

void playerOnInput(InputState const & inputs, EntityId const id) {
    auto* obj = entitySystem.getComponentData<PhysicsComponent>(id);

    obj->velX = 0.f;
    obj->velY = 0.f;
    obj->velZ = 0.f;

    for(auto inp: inputs.inputsHeld) {
        if(config.keybindings.forward.contains(inp)) {
            obj->velX = -8.f;
        } if(config.keybindings.back.contains(inp)) {
            obj->velX = +8.f;
        } if(config.keybindings.left.contains(inp)) {
            obj->velZ = -8.f;
        } if(config.keybindings.right.contains(inp)) {
            obj->velZ = +8.f;
        } if(config.keybindings.up.contains(inp)) {
            obj->velY = +8.f;
        } if(config.keybindings.down.contains(inp)) {
            obj->velY = -8.f;
        }
    }

    int chunkX = ((int)obj->posX) / 16;
    int chunkZ = ((int)obj->posZ) / 16;

    rendererState.setCameraOrientation(
        {obj->posX + 5, obj->posY + 7, obj->posZ + 5},
        {obj->posX, obj->posY, obj->posZ}
    );
    rendererState.setLightOrientation(
        {(float)chunkX * 16 - 48, 19, (float)chunkZ * 16 + 10},
        {(float)chunkX * 16, 0, (float)chunkZ * 16},
        50,
        80
    );
    world.updateModel(chunkX, chunkZ, config.graphics.renderDistance);
}

ModelInstance playerComponentModel() {
    return ModelInstance::fromModelPtr(LOAD_MODEL(PLAYER_MODEL));
}

PhysicsComponent playerComponentPhysics() { 
    auto ret = PhysicsComponent{};
    ret.grounded = true;
    return ret;
}

InputComponent playerComponentInput() {
    return InputComponent{
        .onInput = playerOnInput,
    };
}

EntityId createPlayer() { 
    auto player = entitySystem.newEntity();

    entitySystem.addComponent(player, playerComponentModel());
    entitySystem.addComponent(player, playerComponentPhysics());
    entitySystem.addComponent(player, playerComponentInput());

    return player;
}