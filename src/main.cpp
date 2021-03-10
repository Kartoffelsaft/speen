#include <vector>
#include <chrono>
#include <thread>
#include <bgfx/bgfx.h>
// For whatever reason the linter is convinced I'm trying to include C's math.h
#include <bx/math.h> // NOLINT(modernize-deprecated-headers)

#include "model.h"
#include "rendererState.h"
#include "mathUtils.h"
#include "modelInstance.h"
#include "entitySystem.h"
#include "input.h"
#include "config.h"
#include "chunk.h"
#include "frame.h"
#include "physics.h"

int main() {
    bgfx::setDebug(BGFX_DEBUG_STATS);

    entitySystem.initComponent<ModelInstance>();
    entitySystem.initComponent<InputComponent>();
    entitySystem.initComponent<OnFrameComponent>();
    entitySystem.initComponent<PhysicsComponent>();

    InputState inputState;

    {
        auto donut = entitySystem.newEntity();
        entitySystem.addComponent(donut, ModelInstance::fromModelPtr(LOAD_MODEL("donut.glb")));
        auto& donutOrientation = entitySystem.getComponentData<ModelInstance>(donut)->orientation;
        donutOrientation[12] = -5.f;
        donutOrientation[13] = 1.f;
        donutOrientation[14] = -5.f;
    }

    {
        auto terrain = entitySystem.newEntity();
        entitySystem.addComponent(terrain, ModelInstance::fromModelPtr(world.updateModel(0, 0, config.graphics.renderDistance)));
        auto& terrainOrientation = entitySystem.getComponentData<ModelInstance>(terrain)->orientation;
        terrainOrientation[12] = -8.f;
        terrainOrientation[13] = -1.f;
        terrainOrientation[14] = -8.f;
    }

    auto mokey = entitySystem.newEntity();
    entitySystem.addComponent(mokey, ModelInstance::fromModelPtr(LOAD_MODEL("mokey.glb")));
    entitySystem.addComponent(mokey, PhysicsComponent{
        .posX = 0,
        .posY = 0,
        .posZ = 0,
        .velX = 0,
        .velY = 0,
        .velZ = 0
    });
    entitySystem.addComponent(mokey, InputComponent{
        .onInput = [](InputState const & inputs, EntityId const id) {
            auto* obj = entitySystem.getComponentData<PhysicsComponent>(id);

            obj->velX = 0.f;
            obj->velY = 0.f;
            obj->velZ = 0.f;

            if(inputs.keysHeld.contains(config.keybindings.forward)) {
                obj->velX = -8.f;
            } if(inputs.keysHeld.contains(config.keybindings.back)) {
                obj->velX = +8.f;
            } if(inputs.keysHeld.contains(config.keybindings.left)) {
                obj->velZ = -8.f;
            } if(inputs.keysHeld.contains(config.keybindings.right)) {
                obj->velZ = +8.f;
            } if(inputs.keysHeld.contains(config.keybindings.up)) {
                obj->velY = +8.f;
            } if(inputs.keysHeld.contains(config.keybindings.down)) {
                obj->velY = -8.f;
            }

            int chunkX = ((int)obj->posX - 8) / 16;
            int chunkZ = ((int)obj->posZ - 8) / 16;

            rendererState.setCameraOrientation(
                {obj->posX + 5, obj->posY + 7, obj->posZ + 5},
                {obj->posX, obj->posY, obj->posZ},
                60
            );
            rendererState.setLightOrientation(
                {(float)chunkX * 16 - 40, 19, (float)chunkZ * 16 + 18},
                {(float)chunkX * 16, 0, (float)chunkZ * 16},
                50,
                80
            );
            world.updateModel(chunkX, chunkZ, config.graphics.renderDistance);
        }
    });

    auto frameStart = std::chrono::high_resolution_clock::now();
    float lastFrameTimeElapsed = 0.f;

    while(!rendererState.windowShouldClose) {
        inputState.updateInputs();
        for(auto const & entity: entitySystem.filterByComponent<InputComponent>()) {
            auto* const inputHandler = entitySystem.getComponentData<InputComponent>(entity);
            inputHandler->onInput(inputState, entity);
        }

        for(auto const & entity: entitySystem.filterByComponent<OnFrameComponent>()) {
            entitySystem.getComponentData<OnFrameComponent>(entity)->onFrame(entity, lastFrameTimeElapsed);
        }

        for(auto const & entity: entitySystem.filterByComponent<PhysicsComponent>()) {
            entitySystem.getComponentData<PhysicsComponent>(entity)->step(entity, lastFrameTimeElapsed);
        }

        bgfx::setUniform(
            rendererState.uniforms.u_frame, 
            std::vector<float>{rendererState.frame / 1000.f, 0.0, 0.0, 0.0}.data()
        );

        bgfx::setViewClear(RENDER_SHADOW_ID, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0xffffffff);
        bgfx::setViewClear(RENDER_SCENE_ID,  BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x8899ffff);

        {
            std::vector<bgfx::ViewId> viewOrder = {
                RENDER_SHADOW_ID,
                RENDER_SCENE_ID,
            };

            bgfx::setViewOrder(0, viewOrder.size(), viewOrder.data());
        }
       
        for(auto const & e: entitySystem.filterByComponent<ModelInstance>()) {
            entitySystem.getComponentData<ModelInstance>(e)->draw();
        }

        bgfx::frame();

        auto frameEnd = std::chrono::high_resolution_clock::now();
        std::chrono::duration<float> timeElapsed = frameEnd - frameStart;
        lastFrameTimeElapsed = timeElapsed.count();
        frameStart = frameEnd;

        rendererState.frame++;
    }

    SDL_DestroyWindow(rendererState.window);

    bgfx::shutdown();

    SDL_Quit();

    return 0;
}
