#include "input.h"
#include "rendererState.h"

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
        }
    }
}