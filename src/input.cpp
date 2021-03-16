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
    Mat4 camInv;
    bx::mtxInverse(camInv.data(), rendererState.cameraMtx.data());

    auto destination = bx::mul({x * 2 - 1, -y * 2 + 1, 1}, camInv.data());
    auto direction = bx::normalize(destination);

    // take the direction of the ray to be a function of type m -> (x, y, z)
    auto getRayPos = [&](float multiplier){ return bx::add(rendererState.cameraPos, bx::mul(direction, multiplier)); };
    // and the heightmap of the world to be a function of type (x, y) -> h
    // see: World::sampleHeight
    // Compose them (with some technicalities)
    auto f = [&](float x) {
        auto rayPos = getRayPos(x);
        return world.sampleHeight(rayPos.x, rayPos.z) - rayPos.y;
    };
    // and apply newton's method
    // https://en.wikipedia.org/wiki/Newton%27s_method
    float const X_0 = 2;
    float const DELTAX = 0.95;
    float const MAX_INACCURACY = 0.05;
    int const MAX_ITERATIONS = 3;
    float nx = X_0;
    float fx;
    int i = 0;
    while(std::abs(fx = f(nx)) > MAX_INACCURACY && i < MAX_ITERATIONS) {
        float dfx = (f(nx + DELTAX) - fx) * (1 / DELTAX);
        nx = nx - (fx / dfx);
        i++;
    }

    return getRayPos(nx);
}
