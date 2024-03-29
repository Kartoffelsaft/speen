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
#include "health.h"
#include "debug.h"
#include "pointer.h"

// Fuck you SDL
#ifdef main
#undef main
#endif
int main() {
    initGui();

    entitySystem.initComponent<ModelInstance>();
    entitySystem.initComponent<InputComponent>();
    entitySystem.initComponent<OnFrameComponent>();
    entitySystem.initComponent<PhysicsComponent>();
    entitySystem.initComponent<DeathComponent>();
    entitySystem.initComponent<LifetimeComponent>();
    entitySystem.initComponent<HealthComponent>();
    entitySystem.initComponent<GuiComponent>();
    entitySystem.initComponent<char const *>();

    if(config.misc.debug) createDebugEntity();

    InputState inputState;

    createPointer();

    createWorldEntity();

    createPlayer();

    while(!rendererState.windowShouldClose) {
        inputState.updateInputs();
        for(auto& [id, inp]: entitySystem.filterByComponent<InputComponent>()) {
            inp.onInput(inputState, id);
        }

        for(auto& [id, comp]: entitySystem.filterByComponent<OnFrameComponent>()) {
            comp.onFrame(id, rendererState.lastFrameTimeElapsed);
        }

        for(auto& [id, comp]: entitySystem.filterByComponent<LifetimeComponent>()) {
            comp.age(id, rendererState.lastFrameTimeElapsed);
        }

        for(auto& [id, comp]: entitySystem.filterByComponent<PhysicsComponent>()) {
            comp.step(id, rendererState.lastFrameTimeElapsed);
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
       
        for(auto const & [_, model]: entitySystem.filterByComponent<ModelInstance>()) {
            if(model.mustRender || world.withinRenderDistance(model)) model.draw();
        }

        rendererState.finishRender();
        entitySystem.removeQueuedEntities();
    }

    terminateGui();

    SDL_DestroyWindow(rendererState.window);

    bgfx::shutdown();

    SDL_Quit();

    return 0;
}
