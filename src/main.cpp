#include <stdio.h>
#include <vector>
#include <array>
#include <string>
#include <chrono>
#include <thread>
#include <bgfx/bgfx.h>
#include <bgfx/platform.h>
#include <bx/math.h>

#include "model.h"
#include "rendererState.h"
#include "mathUtils.h"
#include "modelInstance.h"
#include "entitySystem.h"
#include "input.h"
#include "config.h"
#include "chunk.h"

int main(int argc, char** argv) {
    bgfx::setDebug(BGFX_DEBUG_STATS);

    entitySystem.initComponent<ModelInstance>();
    entitySystem.initComponent<InputComponent>();

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
        .onInput = [](std::vector<SDL_Event> const events, EntityId const id) {
            auto* model = entitySystem.getComponentData<ModelInstance>(id);
            for(auto const & event: events) {
                Mat4 delta;
                bx::mtxIdentity(delta.data());
                if(event.type == SDL_KEYDOWN) {
                    // of course it refuses for this to be a switch case
                    if(event.key.keysym.sym == config.keybindings.forward) {
                        delta[12] -= 0.1;
                    } else if(event.key.keysym.sym == config.keybindings.back) {
                        delta[12] += 0.1;
                    } else if(event.key.keysym.sym == config.keybindings.left) {
                        delta[14] -= 0.1;
                    } else if(event.key.keysym.sym == config.keybindings.right) {
                        delta[14] += 0.1;
                    } else if(event.key.keysym.sym == config.keybindings.up) {
                        delta[13] += 0.1;
                    } else if(event.key.keysym.sym == config.keybindings.down) {
                        delta[13] -= 0.1;
                    }
                    Mat4 tmp;
                    bx::mtxMul(tmp.data(), model->orientation.data(), delta.data());
                    model->orientation = tmp;
                }
            }

            rendererState.setCameraOrientation(
                {model->orientation[12] + 5, model->orientation[13] + 5, model->orientation[14] + 5},
                {model->orientation[12], model->orientation[13], model->orientation[14]},
                60
            );
        }
    });
    while(!rendererState.windowShouldClose) {
        std::vector<SDL_Event> events;
        do {
            events.emplace_back();
        } while(SDL_PollEvent(&events.back()));
        events.pop_back(); // last event doesn't have anything in it
        
        for(auto const & e: events) if(e.type == SDL_QUIT) {
            rendererState.windowShouldClose = true;
        }
        
        for(auto const & entity: entitySystem.filterByComponent<InputComponent>()) {
            auto* const inputHandler = entitySystem.getComponentData<InputComponent>(entity);
            inputHandler->onInput(events, entity);
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
