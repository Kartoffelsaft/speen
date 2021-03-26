#pragma once

#include <bgfx/bgfx.h>
#include <bx/math.h> // NOLINT(modernize-deprecated-headers)
#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>

#include "model.h"
#include "mathUtils.h"

char const * const WINDOW_NAME = "First bgfx";

bgfx::ViewId const RENDER_SCENE_ID = 1;
bgfx::ViewId const RENDER_SHADOW_ID = 0;
bgfx::ViewId const RENDER_SCREEN_ID = 2;
bgfx::ViewId const RENDER_BLIT_MOUSE_ID = 3;

float const NEAR_CLIP = 1.3f;
float const FAR_CLIP = 250.f;

struct RendererState {
    static RendererState init();

    uint64_t frame = 0;

    SDL_Window* window;
    ModelLoader modelLoader;

    bool windowShouldClose = false;

    bgfx::ProgramHandle sceneProgram;
    bgfx::ProgramHandle shadowProgram;
    bgfx::ProgramHandle screenProgram;

    bgfx::TextureHandle screenTexture;
    bgfx::TextureHandle screenData;
    bgfx::TextureHandle screenDepth;
    bgfx::FrameBufferHandle screenBuffer;

    bgfx::TextureHandle shadowMap;
    bgfx::FrameBufferHandle shadowMapBuffer;

    Mat4 lightMapMtx;
    Mat4 cameraViewMtx;
    Mat4 cameraProjectionMtx;
    Mat4 cameraMtx;
    bx::Vec3 cameraPos;

    struct {
        bgfx::UniformHandle u_shadowMap;
        bgfx::UniformHandle u_lightDirMtx;
        bgfx::UniformHandle u_lightMapMtx;
        bgfx::UniformHandle u_modelMtx;
        bgfx::UniformHandle u_frame;
        bgfx::UniformHandle u_texture;
    } uniforms;

    void drawTextureToScreen(bgfx::TextureHandle texture, float const z);
    void finishRender();
    
    void setLightOrientation(bx::Vec3 from, bx::Vec3 to, float size, float depth);
    void setCameraOrientation(bx::Vec3 from, bx::Vec3 to);
};

extern RendererState rendererState;
