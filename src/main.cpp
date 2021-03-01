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

int main() {
    bgfx::setDebug(BGFX_DEBUG_STATS);

    entitySystem.initComponent<ModelInstance>();
    entitySystem.initComponent<InputComponent>();

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
        auto chunk = Chunk::generate();
        static auto terrainModel = std::make_shared<Model>(chunk.asModel());
        entitySystem.addComponent(terrain, ModelInstance::fromModelPtr(terrainModel));
        auto& terrainOrientation = entitySystem.getComponentData<ModelInstance>(terrain)->orientation;
        terrainOrientation[12] = -8.f;
        terrainOrientation[13] = -1.f;
        terrainOrientation[14] = -8.f;
    }

    auto mokey = entitySystem.newEntity();
    entitySystem.addComponent(mokey, ModelInstance::fromModelPtr(LOAD_MODEL("mokey.glb")));
    entitySystem.addComponent(mokey, InputComponent{
        .onInput = [](InputState const & inputs, EntityId const id) {
            auto* model = entitySystem.getComponentData<ModelInstance>(id);
            Mat4 delta;
            bx::mtxIdentity(delta.data());
            if(inputs.keysHeld.contains(config.keybindings.forward)) {
                delta[12] -= 0.1;
            } if(inputs.keysHeld.contains(config.keybindings.back)) {
                delta[12] += 0.1;
            } if(inputs.keysHeld.contains(config.keybindings.left)) {
                delta[14] -= 0.1;
            } if(inputs.keysHeld.contains(config.keybindings.right)) {
                delta[14] += 0.1;
            } if(inputs.keysHeld.contains(config.keybindings.up)) {
                delta[13] += 0.1;
            } if(inputs.keysHeld.contains(config.keybindings.down)) {
                delta[13] -= 0.1;
            }
            Mat4 tmp;
            bx::mtxMul(tmp.data(), model->orientation.data(), delta.data());
            model->orientation = tmp;

            float& posX = model->orientation[12];
            float& posY = model->orientation[13];
            float& posZ = model->orientation[14];

            int chunkX = (int)posX / 16;
            int chunkZ = (int)posZ / 16;

            rendererState.setCameraOrientation(
                {posX + 5, posY + 5, posZ + 5},
                {posX, posY, posZ},
                60
            );
            rendererState.setLightOrientation(
                {(float)chunkX * 16 - 10, 5, (float)chunkZ * 16 + 7},
                {(float)chunkX * 16, 0, (float)chunkZ * 16},
                40
            );
        }
    });
    while(!rendererState.windowShouldClose) {
        inputState.updateInputs();
        for(auto const & entity: entitySystem.filterByComponent<InputComponent>()) {
            auto* const inputHandler = entitySystem.getComponentData<InputComponent>(entity);
            inputHandler->onInput(inputState, entity);
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
        rendererState.frame++;
    }

    SDL_DestroyWindow(rendererState.window);

    bgfx::shutdown();

    SDL_Quit();

    return 0;
}
