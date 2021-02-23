#include <bgfx/bgfx.h>
#include <bgfx/platform.h>
#include <bx/math.h>

#include "rendererState.h"
#include "config.h"

bgfx::ShaderHandle createShaderFromArray(uint8_t const * const data, size_t const len) {
    bgfx::Memory const * mem = bgfx::copy(data, len);
    return bgfx::createShader(mem);
}

SDL_Window* initWindow() {
    if(SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "Failed to load SDL video: %s\n", SDL_GetError());
        exit(-1);
    }

    auto ret = SDL_CreateWindow(
        WINDOW_NAME, 
        SDL_WINDOWPOS_UNDEFINED, 
        SDL_WINDOWPOS_UNDEFINED, 
        config.graphics.resolutionX, 
        config.graphics.resolutionY, 
        SDL_WINDOW_SHOWN
    );

    if(ret == nullptr) {
        fprintf(stderr, "Could not create SDL window: %s\n", SDL_GetError());
        exit(-2);
    }
    
    return ret;
}

void initBgfx(SDL_Window * const window) {
    bgfx::renderFrame();

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

    bgfx::reset(config.graphics.resolutionX, config.graphics.resolutionY, BGFX_RESET_MSAA_X4);
}

RendererState RendererState::init() {
    RendererState ret;
    
    ret.window = initWindow();
    
    initBgfx(ret.window);

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

    ret.shadowProgram = [](){
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
                config.graphics.shadowmapResolution,
                config.graphics.shadowmapResolution,
                false,
                1,
                bgfx::TextureFormat::RGBA8,
                BGFX_TEXTURE_RT
            )
        };

        ret.shadowMap = shadowmaps.at(0);
        return bgfx::createFrameBuffer(shadowmaps.size(), shadowmaps.data(), true);
    }();

    bgfx::setViewRect(RENDER_SCENE_ID, 0, 0, config.graphics.resolutionX, config.graphics.resolutionY);
    bgfx::setViewRect(RENDER_SHADOW_ID, 0, 0, config.graphics.shadowmapResolution, config.graphics.shadowmapResolution);
    bgfx::setViewFrameBuffer(RENDER_SHADOW_ID, ret.shadowMapBuffer);

    ret.uniforms = {
        .u_shadowmap   = bgfx::createUniform("u_shadowmap", bgfx::UniformType::Sampler),
        .u_lightDirMtx = bgfx::createUniform("u_lightDirMtx", bgfx::UniformType::Mat4),
        .u_lightmapMtx = bgfx::createUniform("u_lightmapMtx", bgfx::UniformType::Mat4),
        .u_modelMtx    = bgfx::createUniform("u_modelMtx", bgfx::UniformType::Mat4),
        // why does bgfx not have float/int uniforms? ugh.
        .u_frame       = bgfx::createUniform("u_frame", bgfx::UniformType::Vec4)
    };
    
    ret.setLightOrientation({-5, 5, 5}, {0, 0, 0}, 30);

    return ret;
}

void RendererState::setLightOrientation(bx::Vec3 from, bx::Vec3 to, float size){
    Mat4 lightView;
    bx::mtxLookAt(lightView.data(), from, to);

    Mat4 lightProjection;
    bx::mtxOrtho(
        lightProjection.data(), 
        -size, 
        size, 
        -size, 
        size, 
        -size, 
        size, 
        0.f, 
        bgfx::getCaps()->homogeneousDepth
    );

    bgfx::setViewTransform(RENDER_SHADOW_ID, lightView.data(), lightProjection.data());
    bx::mtxMul(this->lightmapMtx.data(), lightView.data(), lightProjection.data());
    bgfx::setUniform(this->uniforms.u_lightDirMtx, lightView.data());
}

void RendererState::setCameraOrientation(bx::Vec3 from, bx::Vec3 to, float fov) {
    Mat4 view;
    bx::mtxLookAt(view.data(), from, to);

    Mat4 projection;
    bx::mtxProj(
        projection.data(),
        fov,
        (float)config.graphics.resolutionX/(float)config.graphics.resolutionY,
        0.01f,
        1000.f,
        bgfx::getCaps()->homogeneousDepth
    );

    bgfx::setViewTransform(RENDER_SCENE_ID, view.data(), projection.data());
}

