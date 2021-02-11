#include <SDL2/SDL_syswm.h>
#include <bgfx/bgfx.h>
#include <bgfx/platform.h>

#include "rendererState.h"

RendererState RendererState::init() {
    RendererState ret;

    if(SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "Failed to load SDL video: %s\n", SDL_GetError());
        exit(-1);
    }

    ret.window = SDL_CreateWindow(
        ret.WINDOW_NAME, 
        SDL_WINDOWPOS_UNDEFINED, 
        SDL_WINDOWPOS_UNDEFINED, 
        ret.WINDOW_WIDTH, 
        ret.WINDOW_HEIGHT, 
        SDL_WINDOW_SHOWN
    );

    if(ret.window == nullptr) {
        fprintf(stderr, "Could not create SDL window: %s\n", SDL_GetError());
        exit(-2);
    }

    bgfx::renderFrame();

    {
        bgfx::Init i;

        SDL_SysWMinfo wndwInfo;
        SDL_VERSION(&wndwInfo.version);
        if(!SDL_GetWindowWMInfo(ret.window, &wndwInfo)) exit(-3);

#if SDL_VIDEO_DRIVER_X11
        i.platformData.ndt = wndwInfo.info.x11.display;
        i.platformData.nwh = (void*)wndwInfo.info.x11.window;
#elif SDL_VIDEO_DRIVER_WINDOWS
        i.platformData.nwh = wndwInfo.info.win.window;
#endif
        i.type = bgfx::RendererType::OpenGL;

        bgfx::init(i);
    }

    bgfx::reset(ret.WINDOW_WIDTH, ret.WINDOW_HEIGHT);

    ret.modelLoader = ModelLoader::init();

    return ret;
}

RendererState rendererState = RendererState::init();
