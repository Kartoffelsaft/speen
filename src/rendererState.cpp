#include <bgfx/bgfx.h>
#include <bgfx/platform.h>
#include <bx/math.h> // NOLINT(modernize-deprecated-headers)

#include "rendererState.h"
#include "config.h"
#include "gui.h"

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
    i.type = bgfx::RendererType::OpenGL;
#elif SDL_VIDEO_DRIVER_WINDOWS
    i.type = bgfx::RendererType::OpenGL;
    i.platformData.nwh = wndwInfo.info.win.window;
#endif

    bgfx::init(i);

    bgfx::reset(
        config.graphics.resolutionX, 
        config.graphics.resolutionY, 
       (config.graphics.vsync? BGFX_RESET_VSYNC : 0)
      |(config.graphics.msaa == 2?  BGFX_RESET_MSAA_X2:
        config.graphics.msaa == 4?  BGFX_RESET_MSAA_X4:
        config.graphics.msaa == 8?  BGFX_RESET_MSAA_X8:
        config.graphics.msaa == 16? BGFX_RESET_MSAA_X16:
        0)
    );
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

    ret.screenProgram = [](){
        auto vertShader = [](){
            #include "../shaderBuild/vertScreen.h"
            return createShaderFromArray(vertScreen, sizeof(vertScreen));
        }();

        auto fragShader = [](){
            #include "../shaderBuild/fragScreen.h"
            return createShaderFromArray(fragScreen, sizeof(fragScreen));
        }();

        return bgfx::createProgram(vertShader, fragShader, true);
    }();

    ret.screenBuffer = [&](){
        std::vector<bgfx::TextureHandle> screenTextures = {
            bgfx::createTexture2D( // visuals
                config.graphics.resolutionX, 
                config.graphics.resolutionY, 
                false, 
                1, 
                bgfx::TextureFormat::RGBA8, 
                BGFX_TEXTURE_RT
            ),
            bgfx::createTexture2D( // extra info
                config.graphics.resolutionX, 
                config.graphics.resolutionY, 
                false, 
                1, 
                bgfx::TextureFormat::RGBA8, 
                BGFX_TEXTURE_RT
            ),
            bgfx::createTexture2D( // depth
                config.graphics.resolutionX, 
                config.graphics.resolutionY, 
                false, 
                1, 
                bgfx::isTextureValid(0, false, 1, bgfx::TextureFormat::D32F, BGFX_TEXTURE_RT)? 
                bgfx::TextureFormat::D32F : bgfx::TextureFormat::D24, 
                BGFX_TEXTURE_RT
            ),
        };
        std::vector<bgfx::Attachment> screenAttachments;
        screenAttachments.reserve(screenTextures.size());
        for(auto& tex: screenTextures) {
            bgfx::Attachment a;
            a.init(tex);
            screenAttachments.push_back(a);
        }

        ret.screenTexture = screenTextures.at(0);
        ret.screenData = screenTextures.at(1);
        ret.screenDepth = screenTextures.at(2);
        return bgfx::createFrameBuffer(screenAttachments.size(), screenAttachments.data(), true);
    }();

    ret.shadowMapBuffer = [&](){
        std::vector<bgfx::TextureHandle> shadowMaps = {
            bgfx::createTexture2D(
                config.graphics.shadowMapResolution,
                config.graphics.shadowMapResolution,
                false,
                1,
                bgfx::TextureFormat::RGBA8,
                BGFX_TEXTURE_RT
            )
        };

        ret.shadowMap = shadowMaps.at(0);
        return bgfx::createFrameBuffer(shadowMaps.size(), shadowMaps.data(), true);
    }();

    bx::mtxIdentity(ret.cameraMtx.data());
    ret.cameraPos = {0.f, 0.f, 0.f};

    bgfx::setViewRect(RENDER_SCENE_ID, 0, 0, config.graphics.resolutionX, config.graphics.resolutionY);
    bgfx::setViewFrameBuffer(RENDER_SCENE_ID, ret.screenBuffer);
    bgfx::setViewRect(RENDER_SHADOW_ID, 0, 0, config.graphics.shadowMapResolution, config.graphics.shadowMapResolution);
    bgfx::setViewFrameBuffer(RENDER_SHADOW_ID, ret.shadowMapBuffer);
    bgfx::setViewRect(RENDER_SCREEN_ID, 0, 0, config.graphics.resolutionX, config.graphics.resolutionY);

    ret.uniforms = {
        .u_shadowMap   = bgfx::createUniform("u_shadowMap", bgfx::UniformType::Sampler),
        .u_lightDirMtx = bgfx::createUniform("u_lightDirMtx", bgfx::UniformType::Mat4),
        .u_lightMapMtx = bgfx::createUniform("u_lightMapMtx", bgfx::UniformType::Mat4),
        .u_modelMtx    = bgfx::createUniform("u_modelMtx", bgfx::UniformType::Mat4),
        // why does bgfx not have float/int uniforms? ugh.
        .u_frame       = bgfx::createUniform("u_frame", bgfx::UniformType::Vec4),
        .u_texture     = bgfx::createUniform("u_texture", bgfx::UniformType::Sampler)
    };
    
    return ret;
}

void RendererState::drawTextureToScreen(bgfx::TextureHandle texture, float const z) {
    bgfx::TransientVertexBuffer screenSpaceBuffer;
    bgfx::TransientIndexBuffer indices;
    bgfx::VertexLayout layout;
    layout.begin()
        .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
        .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
        .end();
    bgfx::allocTransientVertexBuffer(&screenSpaceBuffer, 4, layout);
    auto ssbData = (float*)screenSpaceBuffer.data;
    bgfx::allocTransientIndexBuffer(&indices, 6);
    auto iData = (uint16_t*)indices.data;

    ssbData[0 ] = 0.0f;
    ssbData[1 ] = 0.0f;
    ssbData[2 ] = z   ;
    ssbData[3 ] = 0.0f;
    ssbData[4 ] = 1.0f;

    ssbData[5 ] = 0.0f;
    ssbData[6 ] = config.graphics.resolutionY;
    ssbData[7 ] = z   ;
    ssbData[8 ] = 0.0f;
    ssbData[9 ] = 0.0f;

    ssbData[10] = config.graphics.resolutionX;
    ssbData[11] = 0.0f;
    ssbData[12] = z   ;
    ssbData[13] = 1.0f;
    ssbData[14] = 1.0f;

    ssbData[15] = config.graphics.resolutionX;
    ssbData[16] = config.graphics.resolutionY;
    ssbData[17] = z   ;
    ssbData[18] = 1.0f;
    ssbData[19] = 0.0f;

    iData[0] = 0;
    iData[1] = 3;
    iData[2] = 1;
    iData[3] = 0;
    iData[4] = 2;
    iData[5] = 3;

    bgfx::setState(
        BGFX_STATE_WRITE_RGB 
      | BGFX_STATE_WRITE_A 
      | BGFX_STATE_BLEND_FUNC(
            BGFX_STATE_BLEND_SRC_ALPHA, BGFX_STATE_BLEND_INV_SRC_ALPHA
        )
    );

    bgfx::setVertexBuffer(0, &screenSpaceBuffer);
    bgfx::setIndexBuffer(&indices);
    bgfx::setTexture(0, uniforms.u_texture, texture);
    bgfx::submit(RENDER_SCREEN_ID, screenProgram);
}

void RendererState::finishRender() {
    bgfx::setScissor(0, 0, config.graphics.resolutionX, config.graphics.resolutionY);
    this->drawTextureToScreen(screenTexture, -1.0f);

    drawGui(lastFrameTimeElapsed);

    this->frame = bgfx::frame();

    auto frameEnd = std::chrono::high_resolution_clock::now();
    std::chrono::duration<float> timeElapsed = frameEnd - frameStart;
    lastFrameTimeElapsed = timeElapsed.count();
    frameStart = frameEnd;
}

void RendererState::setLightOrientation(bx::Vec3 from, bx::Vec3 to, float size, float depth){
    Mat4 lightView;
    bx::mtxLookAt(lightView.data(), from, to);

    Mat4 lightProjection;
    bx::mtxOrtho(
        lightProjection.data(), 
        -size, 
        size, 
        -size, 
        size, 
        -depth,
        depth,
        0.f, 
        bgfx::getCaps()->homogeneousDepth
    );

    bgfx::setViewTransform(RENDER_SHADOW_ID, lightView.data(), lightProjection.data());
    lightMapMtx = lightProjection * lightView;
    bgfx::setUniform(this->uniforms.u_lightDirMtx, lightView.data());
}

void RendererState::setCameraOrientation(bx::Vec3 from, bx::Vec3 to) {
    bx::mtxLookAt(cameraViewMtx.data(), from, to);

    bx::mtxProj(
        cameraProjectionMtx.data(),
        (float)config.graphics.fieldOfView,
        (float)config.graphics.resolutionX/(float)config.graphics.resolutionY,
        NEAR_CLIP,
        FAR_CLIP,
        bgfx::getCaps()->homogeneousDepth
    );

    bgfx::setViewTransform(RENDER_SCENE_ID, cameraViewMtx.data(), cameraProjectionMtx.data());
    cameraPos = from;
    cameraMtx = cameraProjectionMtx * cameraViewMtx;
}

