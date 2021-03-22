#include <bx/math.h>
#include <cmath>

#include "input.h"
#include "rendererState.h"
#include "config.h"
#include "chunk.h"

void InputState::updateInputs() {
    keysJustPressed.clear();
    SDL_Event e;
    while(SDL_PollEvent(&e)) {
        switch (e.type) {
            case SDL_QUIT:
                rendererState.windowShouldClose = true;
                break;
            case SDL_KEYDOWN:
                keysJustPressed.insert(e.key.keysym.sym);
                keysHeld.insert(e.key.keysym.sym);
                break;
            case SDL_KEYUP:
                keysHeld.erase(e.key.keysym.sym);
                break;
            case SDL_MOUSEMOTION:
                mousePosX = e.motion.x;
                mousePosY = e.motion.y;
                mousePosXNormal = e.motion.x/(float)config.graphics.resolutionX;
                mousePosYNormal = e.motion.y/(float)config.graphics.resolutionY;
                break;
        }
    }
}

bx::Vec3 getScreenWorldPos(float x, float y) {
    static float depth;
    static bgfx::TextureHandle depthTexture = BGFX_INVALID_HANDLE;
    if(!bgfx::isValid(depthTexture)) [[unlikely]] {
        depthTexture = bgfx::createTexture2D(1, 1, false, 1, bgfx::TextureFormat::D32F, BGFX_TEXTURE_BLIT_DST | BGFX_TEXTURE_READ_BACK);
    }

    auto px = (uint64_t)((x    ) * config.graphics.resolutionX);
    auto py = (uint64_t)((1 - y) * config.graphics.resolutionY);

    bgfx::setViewClear(RENDER_BLIT_MOUSE_ID, BGFX_CLEAR_DEPTH);
    // TODO: properly clamp for (literal) edge case
    bgfx::blit(RENDER_BLIT_MOUSE_ID, depthTexture, 0, 0, rendererState.screenDepth, px, py, 1, 1);
    bgfx::readTexture(depthTexture, &depth);

    Mat4 viewInv;
    bx::mtxInverse(viewInv.data(), rendererState.cameraViewMtx.data());

    Mat4 projInv;
    bx::mtxInverse(projInv.data(), rendererState.cameraProjectionMtx.data());

    auto xy = bx::mul({x * 2 - 1, -y * 2 + 1, 0}, projInv.data());
    // holy shit it took me like at least 10 hours to figure out this is the equasion I need
    // yet I still have no idea how it works
    // touch with peril
    auto z = 0.5 * (FAR_CLIP + NEAR_CLIP) * (2 * NEAR_CLIP) / (FAR_CLIP - depth * (FAR_CLIP - NEAR_CLIP));
    
    return bx::mul({xy.x * z, xy.y * z, z}, viewInv.data());
}
