#include <stdio.h>
#include <vector>
#include <array>
#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>
#include <bgfx/bgfx.h>
#include <bgfx/platform.h>
#include <bx/math.h>

SDL_Window* window;
uint64_t frame = 0;
int const WINDOW_WIDTH = 1280;
int const WINDOW_HEIGHT = 720;
char const * const WINDOW_NAME = "First bgfx";
bool windowShouldClose = false;

bgfx::ProgramHandle shaderProgram;

struct ColoredPosition {
    float x;
    float y;
    float z;

    uint32_t abgr;

    static bgfx::VertexLayout layout() {
        auto ret = bgfx::VertexLayout();
        ret .begin()
            .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
            .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
            .end();
        return ret;
    }
};

auto vertecies = std::vector<ColoredPosition>{
    {-1.f, -1.f, -1.f, 0xff000000},
    {-1.f, -1.f,  1.f, 0xff0000ff},
    {-1.f,  1.f, -1.f, 0xff00ff00},
    {-1.f,  1.f,  1.f, 0xff00ffff},
    { 1.f, -1.f, -1.f, 0xffff0000},
    { 1.f, -1.f,  1.f, 0xffff00ff},
    { 1.f,  1.f, -1.f, 0xffffff00},
    { 1.f,  1.f,  1.f, 0xffffffff},
};

auto indices = std::vector<uint32_t> {
    0, 1, 2,
    1, 3, 2,
    4, 6, 5,
    5, 6, 7,
    0, 2, 4,
    4, 2, 6,
    1, 5, 3,
    5, 7, 3,
    0, 4, 1,
    4, 5, 1,
    2, 3, 6,
    6, 3, 7,
};

bgfx::ShaderHandle createShaderFromArray(uint8_t const * const data, size_t const len) {
    bgfx::Memory const * mem = bgfx::copy(data, len);
    return bgfx::createShader(mem);
}

int main(int argc, char** argv) {
    if(SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "Failed to load SDL video: %s\n", SDL_GetError());
        exit(-1);
    }

    window = SDL_CreateWindow(
        WINDOW_NAME, 
        SDL_WINDOWPOS_UNDEFINED, 
        SDL_WINDOWPOS_UNDEFINED, 
        WINDOW_WIDTH, 
        WINDOW_HEIGHT, 
        SDL_WINDOW_SHOWN
    );

    if(window == nullptr) {
        fprintf(stderr, "Could not create SDL window: %s\n", SDL_GetError());
        exit(-2);
    }

    bgfx::renderFrame();

    {
        bgfx::Init i;

        SDL_SysWMinfo wndwInfo;
        SDL_VERSION(&wndwInfo.version);
        if(!SDL_GetWindowWMInfo(window, &wndwInfo)) exit(-3);

#if SDL_VIDEO_DRIVER_X11
        i.platformData.ndt = wndwInfo.info.x11.display;
        i.platformData.nwh = (void*)wndwInfo.info.x11.window;
#elif SDL_VIDEO_DRIVER_WINDOWS
        i.platformData.nwh = wndwInfo.info.win.window;
#endif
        i.type = bgfx::RendererType::OpenGL;

        bgfx::init(i);
    }
    bgfx::setDebug(BGFX_DEBUG_WIREFRAME);
    bgfx::setDebug(BGFX_DEBUG_STATS);

    auto layout = ColoredPosition::layout();
    auto vertexBuffer = bgfx::createVertexBuffer(
        bgfx::makeRef(vertecies.data(), vertecies.size() * sizeof(ColoredPosition)), 
        layout
    );
    auto indexBuffer = bgfx::createIndexBuffer(
        bgfx::makeRef(indices.data(), indices.size() * sizeof(uint32_t)),
        BGFX_BUFFER_INDEX32
    );

    shaderProgram = [](){
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

    bgfx::reset(WINDOW_WIDTH, WINDOW_HEIGHT);

    bgfx::setViewRect(0, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);

    bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0xff00ffff);

    bgfx::touch(0);
    while(!windowShouldClose) {
        SDL_Event e;
        while(SDL_PollEvent(&e)) {
            if(e.type == SDL_QUIT) {
                windowShouldClose = true;
            }
        }

        bx::Vec3 at = {0.f, 0.f, 0.f};
        bx::Vec3 eye = {7.f, 6.f, 5.f};

        std::array<float, 16> view;
        bx::mtxLookAt(view.data(), eye, at);

        std::array<float, 16> projection;
        bx::mtxProj(
            projection.data(),
            90.f,
            (float)WINDOW_WIDTH/(float)WINDOW_HEIGHT,
            0.01f,
            1000.f,
            bgfx::getCaps()->homogeneousDepth
        );

        bgfx::setViewTransform(0, view.data(), projection.data());

        {
            bgfx::setState(
                BGFX_STATE_WRITE_RGB
              | BGFX_STATE_WRITE_A
              | BGFX_STATE_WRITE_Z
              | BGFX_STATE_CULL_CCW
            );

            std::array<float, 16> trans;
            bx::mtxRotateY(trans.data(), 0.001f * frame);

            bgfx::setTransform(trans.data());

            bgfx::setVertexBuffer(0, vertexBuffer);
            bgfx::setIndexBuffer(indexBuffer);

            bgfx::submit(0, shaderProgram);
        }
        bgfx::frame();
        frame++;
    }

    SDL_DestroyWindow(window);

    bgfx::shutdown();

    SDL_Quit();

    return 0;
}
