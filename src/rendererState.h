#pragma once

#include <bgfx/bgfx.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>

#include "model.h"
#include "mathUtils.h"

struct RendererState {
    static RendererState init();

    uint64_t frame = 0;
    int const WINDOW_WIDTH = 1280;
    int const WINDOW_HEIGHT = 720;
    char const * const WINDOW_NAME = "First bgfx";

    SDL_Window* window;
    ModelLoader modelLoader;

    bool windowShouldClose = false;

    bgfx::ProgramHandle sceneProgram;
    bgfx::ProgramHandle shadowProgram;

    int const RENDER_SCENE_ID = 0;
    int const RENDER_SHADOW_ID = 1;

    int const SHADOW_MAP_SIZE = 1024;
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
};

extern RendererState rendererState;
