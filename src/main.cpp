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

int main(int argc, char** argv) {
    bgfx::setDebug(BGFX_DEBUG_STATS);

    entitySystem.initComponent<ModelInstance>();
    entitySystem.initComponent<InputComponent>();

    auto mokey = entitySystem.newEntity();
    entitySystem.addComponent(mokey, ModelInstance::fromModelPtr(LOAD_MODEL("mokey.glb")));
    entitySystem.addComponent(mokey, InputComponent{
        .onInput = [](std::vector<SDL_Event> const events, EntityId const id) {
            auto* model = entitySystem.getComponentData<ModelInstance>(id);
            for(auto const & event: events) {
                Mat4 delta;
                bx::mtxIdentity(delta.data());
                if(event.type == SDL_KEYDOWN) {
                    switch(event.key.keysym.scancode) {
                        case SDL_SCANCODE_W:
                            bx::mtxRotateX(delta.data(), 0.05);
                            break;
                        case SDL_SCANCODE_S:
                            bx::mtxRotateX(delta.data(), -0.05);
                            break;
                        case SDL_SCANCODE_A:
                            bx::mtxRotateY(delta.data(), 0.05);
                            break;
                        case SDL_SCANCODE_D:
                            bx::mtxRotateY(delta.data(), -0.05);
                            break;
                        default:
                            break;
                    }
                    Mat4 tmp;
                    bx::mtxMul(tmp.data(), model->orientation.data(), delta.data());
                    model->orientation = tmp;
                }
            }
        }
    });

    struct planeVertex {
        float x, y, z;
        uint32_t abgr;
    };
    std::vector<planeVertex> const planeVertecies = {
        { -30.f, -2.f, -30.f, 0xff00ffff },
        { -30.f, -2.f,  30.f, 0xffff00ff },
        {  30.f, -2.f, -30.f, 0xffffff00 },
        {  30.f, -2.f,  30.f, 0xffffffff },
    };

    std::vector<uint32_t> const planeIndicies {
        0, 1, 2,
        2, 1, 3
    };

    auto planeLayout = bgfx::VertexLayout();
    planeLayout
        .begin()
        .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
        .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
        .end();

    auto planeVertexBuffer = bgfx::createVertexBuffer(
        bgfx::makeRef(planeVertecies.data(), planeVertecies.size() * sizeof(planeVertex)), 
        planeLayout
    );
    auto planeIndexBuffer = bgfx::createIndexBuffer(
        bgfx::makeRef(planeIndicies.data(), planeIndicies.size() * sizeof(uint32_t)),
        BGFX_BUFFER_INDEX32
    );

    {
        bx::Vec3 at = {0.f, 0.f, 0.f};
        bx::Vec3 eye = {5.f, 4.f, 3.f};

        Mat4 view;
        bx::mtxLookAt(view.data(), eye, at);

        Mat4 projection;
        bx::mtxProj(
            projection.data(),
            60.f,
            (float)WINDOW_WIDTH/(float)WINDOW_HEIGHT,
            0.01f,
            1000.f,
            bgfx::getCaps()->homogeneousDepth
        );

        bgfx::setViewTransform(RENDER_SCENE_ID, view.data(), projection.data());
    }

    while(!rendererState.windowShouldClose) {
//        SDL_Event e;
//        while(SDL_PollEvent(&e)) {
//            if(e.type == SDL_QUIT) {
//                rendererState.windowShouldClose = true;
//            }
//        }
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
            std::vector<float>{rendererState.frame / 10000.f, 0.0, 0.0, 0.0}.data()
        );

        bgfx::setViewClear(RENDER_SHADOW_ID, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0xffffffff);
        bgfx::setViewClear(RENDER_SCENE_ID,  BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0xff00ffff);

        {
            std::vector<bgfx::ViewId> viewOrder = {
                RENDER_SHADOW_ID,
                RENDER_SCENE_ID,
            };

            bgfx::setViewOrder(0, viewOrder.size(), viewOrder.data());
        }

        {
            Mat4 trans;
            bx::mtxIdentity(trans.data());
/*
            // it's refusing to do the depth test and I'm pissed
            // since it's behind everything else anyway I'll go without drawing it
            bgfx::setState(
                BGFX_STATE_WRITE_RGB
              | BGFX_STATE_WRITE_A
              | BGFX_STATE_WRITE_Z
              | BGFX_STATE_CULL_CCW
              | BGFX_STATE_DEPTH_TEST_LESS
            );

            bgfx::setTransform(trans.data());

            bgfx::setVertexBuffer(0, planeVertexBuffer);
            bgfx::setIndexBuffer(planeIndexBuffer);

            bgfx::submit(RENDER_SHADOW_ID, shadowProgram);
*/
            bgfx::setState(
                BGFX_STATE_WRITE_RGB
              | BGFX_STATE_WRITE_A
              | BGFX_STATE_WRITE_Z
              | BGFX_STATE_CULL_CCW
              | BGFX_STATE_DEPTH_TEST_LESS
            );

            bgfx::setTransform(trans.data());
            bgfx::setUniform(rendererState.uniforms.u_modelMtx, trans.data());

            bgfx::setVertexBuffer(0, planeVertexBuffer);
            bgfx::setIndexBuffer(planeIndexBuffer);

            bgfx::setTexture(0, rendererState.uniforms.u_shadowmap, rendererState.shadowMap);
            bgfx::setUniform(rendererState.uniforms.u_lightmapMtx, rendererState.lightmapMtx.data());

            bgfx::submit(RENDER_SCENE_ID, rendererState.sceneProgram);
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
