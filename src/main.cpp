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

bool windowShouldClose = false;

bgfx::ProgramHandle sceneProgram;
bgfx::ProgramHandle shadowProgram;

int const RENDER_SCENE_ID = 0;
int const RENDER_SHADOW_ID = 1;

int const SHADOW_MAP_SIZE = 256;
bgfx::TextureHandle shadowMap;
bgfx::FrameBufferHandle shadowMapBuffer;

bgfx::ShaderHandle createShaderFromArray(uint8_t const * const data, size_t const len) {
    bgfx::Memory const * mem = bgfx::copy(data, len);
    return bgfx::createShader(mem);
}

int main(int argc, char** argv) {
    bgfx::setDebug(BGFX_DEBUG_STATS);

    std::weak_ptr<Model const> const mokey = LOAD_MODEL("mokey.glb");
    auto const & mokeyVertexBuffer = mokey.lock()->primitives[0].vertexBuffer;
    auto const & mokeyIndexBuffer  = mokey.lock()->primitives[0].indexBuffer;

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

    sceneProgram = [](){
        auto vertShader = [](){
            #include "../shaderBuild/vert.h"
            return createShaderFromArray(vert, sizeof(vert));
        }();

        auto fragShader = [](){
            #include "../shaderBuild/frag.h"
            return createShaderFromArray(frag, sizeof(frag));
        }();

        return bgfx::createProgram(vertShader, fragShader, true);
    }();


    bgfx::setViewRect(RENDER_SCENE_ID, 0, 0, rendererState.WINDOW_WIDTH, rendererState.WINDOW_HEIGHT);


    shadowProgram = [](){
        auto vertShader = [](){
            #include "../shaderBuild/vertShadowmap.h"
            return createShaderFromArray(vertShadowmap, sizeof(vertShadowmap));
        }();

        auto fragShader = [](){
            #include "../shaderBuild/fragShadowmap.h"
            return createShaderFromArray(fragShadowmap, sizeof(fragShadowmap));
        }();

        return bgfx::createProgram(vertShader, fragShader, true);
    }();

    shadowMapBuffer = [](){
        std::vector<bgfx::TextureHandle> shadowmaps = {
            bgfx::createTexture2D(
                SHADOW_MAP_SIZE,
                SHADOW_MAP_SIZE,
                false,
                1,
                bgfx::TextureFormat::RGBA8,
                BGFX_TEXTURE_RT
            )
        };

        shadowMap = shadowmaps.at(0);
        return bgfx::createFrameBuffer(shadowmaps.size(), shadowmaps.data(), true);
    }();

    bgfx::setViewRect(RENDER_SHADOW_ID, 0, 0, SHADOW_MAP_SIZE, SHADOW_MAP_SIZE);
    bgfx::setViewFrameBuffer(RENDER_SHADOW_ID, shadowMapBuffer);

    auto u_shadowmap = bgfx::createUniform("u_shadowmap", bgfx::UniformType::Sampler);
    auto u_lightmapMtx = bgfx::createUniform("u_lightmapMtx", bgfx::UniformType::Mat4);
    auto u_lightDirMtx = bgfx::createUniform("u_lightDirMtx", bgfx::UniformType::Mat4);
    auto u_modelMtx = bgfx::createUniform("u_modelMtx", bgfx::UniformType::Mat4);
    // why does bgfx not have float/int uniforms? ugh.
    auto u_frame = bgfx::createUniform("u_frame", bgfx::UniformType::Vec4);

    Mat4 lightmapMtx;
    {
        bx::Vec3 lightSource = {-4.f, 6.f, 3.f};
        bx::Vec3 lightDest = {0.f, 0.f, 0.f};

        Mat4 lightView;
        bx::mtxLookAt(lightView.data(), lightSource, lightDest);

        Mat4 lightProjection;
        bx::mtxOrtho(
            lightProjection.data(), 
            -10.f, 
            10.f, 
            -10.f, 
            10.f, 
            -15.f, 
            15.f, 
            0.f, 
            bgfx::getCaps()->homogeneousDepth
        );

        bgfx::setViewTransform(RENDER_SHADOW_ID, lightView.data(), lightProjection.data());
        bx::mtxMul(lightmapMtx.data(), lightView.data(), lightProjection.data());
        bgfx::setUniform(u_lightDirMtx, lightView.data());
    }

    {
        bx::Vec3 at = {0.f, 0.f, 0.f};
        bx::Vec3 eye = {5.f, 4.f, 3.f};

        Mat4 view;
        bx::mtxLookAt(view.data(), eye, at);

        Mat4 projection;
        bx::mtxProj(
            projection.data(),
            60.f,
            (float)rendererState.WINDOW_WIDTH/(float)rendererState.WINDOW_HEIGHT,
            0.01f,
            1000.f,
            bgfx::getCaps()->homogeneousDepth
        );

        bgfx::setViewTransform(RENDER_SCENE_ID, view.data(), projection.data());
    }

    bgfx::touch(RENDER_SCENE_ID);
    while(!windowShouldClose) {
        SDL_Event e;
        while(SDL_PollEvent(&e)) {
            if(e.type == SDL_QUIT) {
                windowShouldClose = true;
            }
        }

        bgfx::setUniform(u_frame, std::vector<float>{rendererState.frame / 10000.f, 0.0, 0.0, 0.0}.data());

        bgfx::setViewClear(RENDER_SHADOW_ID, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0xffffffff);
        bgfx::setViewClear(RENDER_SCENE_ID, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0xff00ffff);

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
            bgfx::setUniform(u_modelMtx, trans.data());

            bgfx::setVertexBuffer(0, planeVertexBuffer);
            bgfx::setIndexBuffer(planeIndexBuffer);

            bgfx::setTexture(0, u_shadowmap, shadowMap);
            bgfx::setUniform(u_lightmapMtx, lightmapMtx.data());

            bgfx::submit(RENDER_SCENE_ID, sceneProgram);
        }
        {
            Mat4 trans;
            bx::mtxRotateY(trans.data(), 0.0001f * rendererState.frame);

            bgfx::setState(
                BGFX_STATE_WRITE_RGB
              | BGFX_STATE_WRITE_A
              | BGFX_STATE_WRITE_Z
              | BGFX_STATE_CULL_CCW
              | BGFX_STATE_DEPTH_TEST_LESS
            );
            
            bgfx::setTransform(trans.data());

            bgfx::setVertexBuffer(0, mokeyVertexBuffer);
            bgfx::setIndexBuffer(mokeyIndexBuffer);

            bgfx::submit(RENDER_SHADOW_ID, shadowProgram);

            bgfx::setState(
                BGFX_STATE_WRITE_RGB
              | BGFX_STATE_WRITE_A
              | BGFX_STATE_WRITE_Z
              | BGFX_STATE_CULL_CCW
              | BGFX_STATE_DEPTH_TEST_LESS
            );

            bgfx::setTransform(trans.data());
            bgfx::setUniform(u_modelMtx, trans.data());

            bgfx::setVertexBuffer(0, mokeyVertexBuffer);
            bgfx::setIndexBuffer(mokeyIndexBuffer);

            bgfx::setTexture(0, u_shadowmap, shadowMap);
            bgfx::setUniform(u_lightmapMtx, lightmapMtx.data());

            bgfx::submit(RENDER_SCENE_ID, sceneProgram);
        }

        bgfx::frame();
        rendererState.frame++;
    }

    SDL_DestroyWindow(rendererState.window);

    bgfx::shutdown();

    SDL_Quit();

    return 0;
}
