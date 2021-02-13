#include <bgfx/bgfx.h>
#include <bgfx/platform.h>
#include <bx/math.h>

#include "rendererState.h"

bgfx::ShaderHandle createShaderFromArray(uint8_t const * const data, size_t const len) {
    bgfx::Memory const * mem = bgfx::copy(data, len);
    return bgfx::createShader(mem);
}

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

    {
        bgfx::renderFrame();

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

    ret.sceneProgram = [](){
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

    ret.shadowProgram = [&](){
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

    ret.shadowMapBuffer = [&](){
        std::vector<bgfx::TextureHandle> shadowmaps = {
            bgfx::createTexture2D(
                ret.SHADOW_MAP_SIZE,
                ret.SHADOW_MAP_SIZE,
                false,
                1,
                bgfx::TextureFormat::RGBA8,
                BGFX_TEXTURE_RT
            )
        };

        ret.shadowMap = shadowmaps.at(0);
        return bgfx::createFrameBuffer(shadowmaps.size(), shadowmaps.data(), true);
    }();

    bgfx::setViewRect(ret.RENDER_SCENE_ID, 0, 0, rendererState.WINDOW_WIDTH, rendererState.WINDOW_HEIGHT);
    bgfx::setViewRect(ret.RENDER_SHADOW_ID, 0, 0, ret.SHADOW_MAP_SIZE, ret.SHADOW_MAP_SIZE);
    bgfx::setViewFrameBuffer(ret.RENDER_SHADOW_ID, ret.shadowMapBuffer);

    ret.uniforms.u_shadowmap   = bgfx::createUniform("u_shadowmap", bgfx::UniformType::Sampler);
    ret.uniforms.u_lightmapMtx = bgfx::createUniform("u_lightmapMtx", bgfx::UniformType::Mat4);
    ret.uniforms.u_lightDirMtx = bgfx::createUniform("u_lightDirMtx", bgfx::UniformType::Mat4);
    ret.uniforms.u_modelMtx    = bgfx::createUniform("u_modelMtx", bgfx::UniformType::Mat4);
    // why does bgfx not have float/int uniforms? ugh.
    ret.uniforms.u_frame       = bgfx::createUniform("u_frame", bgfx::UniformType::Vec4);

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

        bgfx::setViewTransform(ret.RENDER_SHADOW_ID, lightView.data(), lightProjection.data());
        bx::mtxMul(ret.lightmapMtx.data(), lightView.data(), lightProjection.data());
        bgfx::setUniform(ret.uniforms.u_lightDirMtx, lightView.data());
    }
    return ret;
}

RendererState rendererState = RendererState::init();
