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
#include "player.h"
#include "gui.h"

int main() {
    bgfx::setDebug(BGFX_DEBUG_STATS);

    initGui();

    entitySystem.initComponent<ModelInstance>();
    entitySystem.initComponent<InputComponent>();
    entitySystem.initComponent<OnFrameComponent>();
    entitySystem.initComponent<PhysicsComponent>();
    entitySystem.initComponent<GuiComponent>();

    InputState inputState;

    {
        auto donut = entitySystem.newEntity();
        entitySystem.addComponent(donut, ModelInstance::fromModelPtr(LOAD_MODEL("donut.glb")));
        entitySystem.addComponent(donut, InputComponent{
            .onInput = [](InputState const & inputs, EntityId const id) {
                auto& donutOrientation = entitySystem.getComponentData<ModelInstance>(id).orientation;
                auto newPos = getScreenWorldPos(inputs.mousePosXNormal, inputs.mousePosYNormal);
                donutOrientation[12] = newPos.x;
                donutOrientation[13] = newPos.y;
                donutOrientation[14] = newPos.z;

                for(auto i: inputs.inputsJustPressed) if(config.keybindings.place.contains(i)) {
                    auto newPlacement = entitySystem.newEntity();
                    entitySystem.addComponent(newPlacement, ModelInstance::fromModelPtr(LOAD_MODEL("man.glb")));
                    entitySystem.addComponent(newPlacement, PhysicsComponent{
                        .position = Vec3{
                            donutOrientation[12],
                            donutOrientation[13],
                            donutOrientation[14],
                        },
                        .collidable = Collidable{
                            .collisionRange = 0.5,
                        }
                    });
                }
            }
        });
    }

    createWorldEntity();

    createPlayer();

    while(!rendererState.windowShouldClose) {
        inputState.updateInputs();
        for(auto const & entity: entitySystem.filterByComponent<InputComponent>()) {
            auto& inputHandler = entitySystem.getComponentData<InputComponent>(entity);
            inputHandler.onInput(inputState, entity);
        }

        for(auto const & entity: entitySystem.filterByComponent<OnFrameComponent>()) {
            if(!entitySystem.invalidEntities.contains(entity))
                entitySystem.getComponentData<OnFrameComponent>(entity).onFrame(entity, rendererState.lastFrameTimeElapsed);
        }

        for(auto const & entity: entitySystem.filterByComponent<PhysicsComponent>()) {
            if(!entitySystem.invalidEntities.contains(entity))
                entitySystem.getComponentData<PhysicsComponent>(entity).step(entity, rendererState.lastFrameTimeElapsed);
        }

        bgfx::setUniform(
            rendererState.uniforms.u_frame, 
            std::vector<float>{rendererState.frame / 1000.f, 0.0, 0.0, 0.0}.data()
        );

        bgfx::setViewClear(RENDER_SHADOW_ID, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0xffffffff);
        bgfx::setViewClear(RENDER_SCENE_ID,  BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x8899ffff);
        bgfx::setViewClear(RENDER_SCREEN_ID, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x8899ffff);

        {
            std::vector<bgfx::ViewId> viewOrder = {
                RENDER_SHADOW_ID,
                RENDER_SCENE_ID,
                RENDER_SCREEN_ID,
            };

            bgfx::setViewOrder(0, viewOrder.size(), viewOrder.data());
        }
       
        for(auto const & e: entitySystem.filterByComponent<ModelInstance>()) {
            entitySystem.getComponentData<ModelInstance>(e).draw();
        }

        rendererState.finishRender();
    }

    terminateGui();

    SDL_DestroyWindow(rendererState.window);

    bgfx::shutdown();

    SDL_Quit();

    return 0;
}
