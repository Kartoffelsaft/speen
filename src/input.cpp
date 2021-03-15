#include <bx/math.h>

#include "input.h"
#include "rendererState.h"
#include "config.h"

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
    Mat4 camInv;
    bx::mtxInverse(camInv.data(), rendererState.cameraMtx.data());

    auto destination = bx::mul({x * 2 - 1, -y * 2 + 1, 1}, camInv.data());
    auto direction = bx::normalize(destination);

    return bx::add(rendererState.cameraPos, bx::mul(direction, 8.f));
}
