#pragma once

#include <bgfx/bgfx.h>
#include <bx/math.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>

#include "model.h"
#include "mathUtils.h"

char const * const WINDOW_NAME = "First bgfx";

bgfx::ViewId const RENDER_SCENE_ID = 1;
bgfx::ViewId const RENDER_SHADOW_ID = 0;

struct RendererState {
    static RendererState init();

    uint64_t frame = 0;

    SDL_Window* window;
    ModelLoader modelLoader;

    bool windowShouldClose = false;

    bgfx::ProgramHandle sceneProgram;
    bgfx::ProgramHandle shadowProgram;

    bgfx::TextureHandle shadowMap;
    bgfx::FrameBufferHandle shadowMapBuffer;

    Mat4 lightmapMtx;

    struct {
        bgfx::UniformHandle u_shadowmap;
        bgfx::UniformHandle u_lightDirMtx;
        bgfx::UniformHandle u_lightmapMtx;
        bgfx::UniformHandle u_modelMtx;
        bgfx::UniformHandle u_frame;
    } uniforms;
    
    void setLightOrientation(bx::Vec3 from, bx::Vec3 to, float size);
    void setCameraOrientation(bx::Vec3 from, bx::Vec3 to, float fov);
};

extern RendererState rendererState;
