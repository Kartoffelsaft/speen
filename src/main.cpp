#include <stdio.h>
#include <vector>
#include <array>
#include <string>
#include <chrono>
#include <thread>
#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>
#include <bgfx/bgfx.h>
#include <bgfx/platform.h>
#include <bx/math.h>
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <tiny_gltf.h>

SDL_Window* window;
uint64_t frame = 0;
int const WINDOW_WIDTH = 1280;
int const WINDOW_HEIGHT = 720;
char const * const WINDOW_NAME = "First bgfx";
bool windowShouldClose = false;

bgfx::ProgramHandle sceneProgram;
bgfx::ProgramHandle shadowProgram;

int const RENDER_SCENE_ID = 0;
int const RENDER_SHADOW_ID = 1;

int const SHADOW_MAP_SIZE = 256;
bgfx::TextureHandle shadowMap;
bgfx::FrameBufferHandle shadowMapBuffer;

struct Model {
    static Model loadFromGLBData(tinygltf::TinyGLTF& loader, uint8_t const * const data, size_t const size) {
        tinygltf::Model model;
        std::string err;
        std::string warn;
        loader.LoadBinaryFromMemory(
            &model,
            &err,
            &warn,
            data,
            size
        );

        if(err.size() > 0) {
            fprintf(stderr, "Error loading file: %s\n", err.c_str());
        }

        return loadFromGLTFModel(model);
    }
    
    static Model loadFromGLBFile(tinygltf::TinyGLTF& loader, std::string const & file) {
        tinygltf::Model model;
        std::string err;
        std::string warn;
        loader.LoadBinaryFromFile(
            &model,
            &err,
            &warn,
            file
        );

        if(err.size() > 0) {
            fprintf(stderr, "Error loading file: %s\n", err.c_str());
        }

        return loadFromGLTFModel(model);
    }

    static Model loadFromGLTFModel(tinygltf::Model const model) {
        decltype(primitives) retPrimitives;
        
        for(auto const mesh: model.meshes) for(auto const rawPrimitive: mesh.primitives) {
            bgfx::VertexLayout retLayout;
            retLayout.begin();

            size_t vertexElementSize = 0;
            size_t vertexCount;
            std::vector<uint32_t> indices;
            std::vector<float> positions;
            std::vector<float> colors;
            std::vector<float> normals;

            {
                auto const & accessor = model.accessors[rawPrimitive.indices];
                auto const & bufferView = model.bufferViews[accessor.bufferView];
                auto const & buffer = model.buffers[bufferView.buffer];

                indices.reserve(accessor.count * 1);

                void const * const bufferData = 
                    (void*)(buffer.data.data() + bufferView.byteOffset + accessor.byteOffset);
                for(int i = 0; i < accessor.count; i++) {
                    switch(accessor.componentType) {
                        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE: 
                            indices.push_back(((uint8_t*)bufferData)[i]);
                            break;
                        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: 
                            indices.push_back(((uint16_t*)bufferData)[i]);
                            break;
                        case TINYGLTF_COMPONENT_TYPE_INT: 
                            indices.push_back(((uint32_t*)bufferData)[i]);
                            break;
                        // case TINYGLTF_COMPONENT_TYPE_UNSIGNED_LONG: 
                        //     indices.push_back(((uint64_t*)bufferData)[i]);
                        //     break;
                    }
                }
            }

            auto positionOffset = vertexElementSize;
            if(rawPrimitive.attributes.contains("POSITION")) {
                vertexElementSize += 3 * sizeof(float);
                
                tinygltf::Accessor const & accessor = model.accessors[rawPrimitive.attributes.at("POSITION")];
                tinygltf::BufferView const & bufferView = model.bufferViews[accessor.bufferView];
                tinygltf::Buffer const & buffer = model.buffers[bufferView.buffer];

                retLayout.add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float);
                positions.reserve(accessor.count * 3);

                float const * const bufferData = 
                    (float*)(buffer.data.data() + bufferView.byteOffset + accessor.byteOffset);
                for(int i = 0; i < accessor.count * 3; i++) {
                    positions.push_back(bufferData[i]);
                }

                vertexCount = accessor.count;
            } else {
                fprintf(stderr, "A 3D model without vertecies is a little odd, don't you think?\n");
            }

            auto colorOffset = vertexElementSize;
            if(rawPrimitive.attributes.contains("COLOR_0")) {
                vertexElementSize += 4 * sizeof(float);

                tinygltf::Accessor const & accessor = model.accessors[rawPrimitive.attributes.at("COLOR_0")];
                tinygltf::BufferView const & bufferView = model.bufferViews[accessor.bufferView];
                tinygltf::Buffer const & buffer = model.buffers[bufferView.buffer];

                colors.reserve(accessor.count * 4);
                retLayout.add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Float);
                bool const padWithAlpha = accessor.type == TINYGLTF_TYPE_VEC3;

                void const * const bufferData = 
                    (void*)(buffer.data.data() + bufferView.byteOffset + accessor.byteOffset);
                // bgfx is big endian, (abgr), gltf is little endian, (rgba), yet I don't need to
                // flip it here? wtf?
                if(!padWithAlpha) { for(int i = 0; i < accessor.count; i++) {
                    switch(accessor.componentType) {
                        case TINYGLTF_COMPONENT_TYPE_FLOAT:
                            colors.push_back(((float*)bufferData)[i*4 + 0]);
                            colors.push_back(((float*)bufferData)[i*4 + 1]);
                            colors.push_back(((float*)bufferData)[i*4 + 2]);
                            colors.push_back(((float*)bufferData)[i*4 + 3]);
                            break;
                        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
                            colors.push_back(((uint8_t*)bufferData)[i*4 + 0] / (float)UCHAR_MAX);
                            colors.push_back(((uint8_t*)bufferData)[i*4 + 1] / (float)UCHAR_MAX);
                            colors.push_back(((uint8_t*)bufferData)[i*4 + 2] / (float)UCHAR_MAX);
                            colors.push_back(((uint8_t*)bufferData)[i*4 + 3] / (float)UCHAR_MAX);
                            break;
                        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
                            colors.push_back(((uint16_t*)bufferData)[i*4 + 0] / (float)USHRT_MAX);
                            colors.push_back(((uint16_t*)bufferData)[i*4 + 1] / (float)USHRT_MAX);
                            colors.push_back(((uint16_t*)bufferData)[i*4 + 2] / (float)USHRT_MAX);
                            colors.push_back(((uint16_t*)bufferData)[i*4 + 3] / (float)USHRT_MAX);
                            break;
                    }
                }} else { for(int i = 0; i < accessor.count; i++) {
                    switch(accessor.componentType) {
                        case TINYGLTF_COMPONENT_TYPE_FLOAT:
                            colors.push_back(((float*)bufferData)[i*3 + 0]);
                            colors.push_back(((float*)bufferData)[i*3 + 1]);
                            colors.push_back(((float*)bufferData)[i*3 + 2]);
                            break;
                        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
                            colors.push_back(((uint8_t*)bufferData)[i*3 + 0] / (float)UCHAR_MAX);
                            colors.push_back(((uint8_t*)bufferData)[i*3 + 1] / (float)UCHAR_MAX);
                            colors.push_back(((uint8_t*)bufferData)[i*3 + 2] / (float)UCHAR_MAX);
                            break;
                        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
                            colors.push_back(((uint16_t*)bufferData)[i*3 + 0] / (float)USHRT_MAX);
                            colors.push_back(((uint16_t*)bufferData)[i*3 + 1] / (float)USHRT_MAX);
                            colors.push_back(((uint16_t*)bufferData)[i*3 + 2] / (float)USHRT_MAX);
                            break;
                    }
                    colors.push_back(1.0);
                }}
            }

            auto normalOffset = vertexElementSize;
            if(rawPrimitive.attributes.contains("NORMAL")) {
                vertexElementSize += 3 * sizeof(float);

                auto const & accessor = model.accessors[rawPrimitive.attributes.at("NORMAL")];
                auto const & bufferView = model.bufferViews[accessor.bufferView];
                auto const & buffer = model.buffers[bufferView.buffer];

                normals.reserve(accessor.count * 3);
                retLayout.add(bgfx::Attrib::Normal, 3, bgfx::AttribType::Float);

                float const * const bufferData = 
                    (float*)(buffer.data.data() + bufferView.byteOffset + accessor.byteOffset);
                for(int i = 0; i < accessor.count * 3; i++) {
                    normals.push_back(bufferData[i]);
                }
            }

            retLayout.end();
            std::vector<uint8_t> retVertexData(vertexCount * vertexElementSize);
            if(positions.size() > 0) for(int i = 0; i < vertexCount; i++) {
                *((float*)(retVertexData.data() + i * vertexElementSize + positionOffset) + 0)
                    = positions[i*3 + 0];
                *((float*)(retVertexData.data() + i * vertexElementSize + positionOffset) + 1)
                    = positions[i*3 + 1];
                *((float*)(retVertexData.data() + i * vertexElementSize + positionOffset) + 2)
                    = positions[i*3 + 2];
            }
            if(colors.size() > 0) for(int i = 0; i < vertexCount; i++) {
                *((float*)(retVertexData.data() + i * vertexElementSize + colorOffset) + 0)
                    = colors[i*4 + 0];
                *((float*)(retVertexData.data() + i * vertexElementSize + colorOffset) + 1)
                    = colors[i*4 + 1];
                *((float*)(retVertexData.data() + i * vertexElementSize + colorOffset) + 2)
                    = colors[i*4 + 2];
                *((float*)(retVertexData.data() + i * vertexElementSize + colorOffset) + 3)
                    = colors[i*4 + 3];
            }
            if(normals.size() > 0) for(int i = 0; i < vertexCount; i++) {
                *((float*)(retVertexData.data() + i * vertexElementSize + normalOffset) + 0)
                    = normals[i*3 + 0];
                *((float*)(retVertexData.data() + i * vertexElementSize + normalOffset) + 1)
                    = normals[i*3 + 1];
                *((float*)(retVertexData.data() + i * vertexElementSize + normalOffset) + 2)
                    = normals[i*3 + 2];
            }

            retPrimitives.push_back({
                .vertexData = retVertexData,
                .indexData = indices,
                .layout = retLayout,
            });
        }

        return {
            .primitives = retPrimitives,
        };
    }

    struct Primitive {
        std::vector<uint8_t> const vertexData;
        std::vector<uint32_t> const indexData;
        bgfx::VertexLayout const layout;
    };

    std::vector<Primitive> primitives;
};

// To use:
// #define MODEL_TO_LOAD [Your file name]
// #define TINYGLTF_LOADER [Your tinygltf::TinyGLTF]
// yourMesh = #include FILE_LOADER_HEADER
//
// I would make this a function-like macro, but macros can't contain includes
#ifdef EMBED_MODEL_FILES
#define FILE_LOADER_HEADER "./embed.h"
#else
#define FILE_LOADER_HEADER "./noembed.h"
#endif

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

    bgfx::reset(WINDOW_WIDTH, WINDOW_HEIGHT);

    bgfx::setDebug(BGFX_DEBUG_STATS);

    tinygltf::TinyGLTF modelLoader;

#define MODEL_TO_LOAD mokey.glb
#define TINYGLTF_LOADER modelLoader
    Model const mokey = 
#include FILE_LOADER_HEADER
    auto const & layout = mokey.primitives[0].layout;
    auto const & vertecies = mokey.primitives[0].vertexData;
    auto const & indices = mokey.primitives[0].indexData;

    auto mokeyVertexBuffer = bgfx::createVertexBuffer(
        bgfx::makeRef(vertecies.data(), vertecies.size()), 
        layout
    );
    auto mokeyIndexBuffer = bgfx::createIndexBuffer(
        bgfx::makeRef(indices.data(), indices.size() * sizeof(uint32_t)),
        BGFX_BUFFER_INDEX32
    );

    struct planeVertex {
        float x, y, z;
        uint32_t abgr;
    };
    std::vector<planeVertex> const planeVertecies = {
        { -30.f, -2.f, -30.f, 0xff00ffff },
        { -30.f, -2.f,  30.f, 0xffff00ff },
        {  30.f, -2.f, -30.f, 0xffffff00 },
        {  30.f, -2.f,  30.f, 0xffffffff },
    };

    std::vector<uint32_t> const planeIndicies {
        0, 1, 2,
        2, 1, 3
    };

    auto planeLayout = bgfx::VertexLayout();
    planeLayout
        .begin()
        .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
        .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
        .end();

    auto planeVertexBuffer = bgfx::createVertexBuffer(
        bgfx::makeRef(planeVertecies.data(), planeVertecies.size() * sizeof(planeVertex)), 
        planeLayout
    );
    auto planeIndexBuffer = bgfx::createIndexBuffer(
        bgfx::makeRef(planeIndicies.data(), planeIndicies.size() * sizeof(uint32_t)),
        BGFX_BUFFER_INDEX32
    );

    sceneProgram = [](){
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


    bgfx::setViewRect(RENDER_SCENE_ID, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);


    shadowProgram = [](){
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

    shadowMapBuffer = [](){
        std::vector<bgfx::TextureHandle> shadowmaps = {
            bgfx::createTexture2D(
                SHADOW_MAP_SIZE,
                SHADOW_MAP_SIZE,
                false,
                1,
                bgfx::TextureFormat::RGBA8,
                BGFX_TEXTURE_RT
            )
        };

        shadowMap = shadowmaps.at(0);
        return bgfx::createFrameBuffer(shadowmaps.size(), shadowmaps.data(), true);
    }();

    bgfx::setViewRect(RENDER_SHADOW_ID, 0, 0, SHADOW_MAP_SIZE, SHADOW_MAP_SIZE);
    bgfx::setViewFrameBuffer(RENDER_SHADOW_ID, shadowMapBuffer);

    auto u_shadowmap = bgfx::createUniform("u_shadowmap", bgfx::UniformType::Sampler);
    auto u_lightmapMtx = bgfx::createUniform("u_lightmapMtx", bgfx::UniformType::Mat4);
    auto u_lightDirMtx = bgfx::createUniform("u_lightDirMtx", bgfx::UniformType::Mat4);
    auto u_modelMtx = bgfx::createUniform("u_modelMtx", bgfx::UniformType::Mat4);
    // why does bgfx not have float/int uniforms? ugh.
    auto u_frame = bgfx::createUniform("u_frame", bgfx::UniformType::Vec4);

    std::array<float, 16> lightmapMtx;
    {
        bx::Vec3 lightSource = {-4.f, 6.f, 3.f};
        bx::Vec3 lightDest = {0.f, 0.f, 0.f};

        std::array<float, 16> lightView;
        bx::mtxLookAt(lightView.data(), lightSource, lightDest);

        std::array<float, 16> lightProjection;
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

        bgfx::setViewTransform(RENDER_SHADOW_ID, lightView.data(), lightProjection.data());
        bx::mtxMul(lightmapMtx.data(), lightView.data(), lightProjection.data());
        bgfx::setUniform(u_lightDirMtx, lightView.data());
    }

    {
        bx::Vec3 at = {0.f, 0.f, 0.f};
        bx::Vec3 eye = {5.f, 4.f, 3.f};

        std::array<float, 16> view;
        bx::mtxLookAt(view.data(), eye, at);

        std::array<float, 16> projection;
        bx::mtxProj(
            projection.data(),
            60.f,
            (float)WINDOW_WIDTH/(float)WINDOW_HEIGHT,
            0.01f,
            1000.f,
            bgfx::getCaps()->homogeneousDepth
        );

        bgfx::setViewTransform(RENDER_SCENE_ID, view.data(), projection.data());
    }

    bgfx::touch(RENDER_SCENE_ID);
    while(!windowShouldClose) {
        SDL_Event e;
        while(SDL_PollEvent(&e)) {
            if(e.type == SDL_QUIT) {
                windowShouldClose = true;
            }
        }

        bgfx::setUniform(u_frame, std::vector<float>{frame / 1000.f, 0.0, 0.0, 0.0}.data());

        bgfx::setViewClear(RENDER_SHADOW_ID, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0xffffffff);
        bgfx::setViewClear(RENDER_SCENE_ID, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0xff00ffff);

        {
            std::vector<bgfx::ViewId> viewOrder = {
                RENDER_SHADOW_ID,
                RENDER_SCENE_ID,
            };

            bgfx::setViewOrder(0, viewOrder.size(), viewOrder.data());
        }

        {
            std::array<float, 16> trans;
            bx::mtxIdentity(trans.data());
/*
            // it's refusing to do the depth test and I'm pissed
            // since it's behine everything else anyway I'll go without drawing it;
            bgfx::setState(
                BGFX_STATE_WRITE_RGB
              | BGFX_STATE_WRITE_A
              | BGFX_STATE_WRITE_Z
              | BGFX_STATE_CULL_CCW
              | BGFX_STATE_DEPTH_TEST_LESS
            );

            bgfx::setTransform(trans.data());

            bgfx::setVertexBuffer(0, planeVertexBuffer);
            bgfx::setIndexBuffer(planeIndexBuffer);

            bgfx::submit(RENDER_SHADOW_ID, shadowProgram);
*/
            bgfx::setState(
                BGFX_STATE_WRITE_RGB
              | BGFX_STATE_WRITE_A
              | BGFX_STATE_WRITE_Z
              | BGFX_STATE_CULL_CCW
              | BGFX_STATE_DEPTH_TEST_LESS
            );

            bgfx::setTransform(trans.data());
            bgfx::setUniform(u_modelMtx, trans.data());

            bgfx::setVertexBuffer(0, planeVertexBuffer);
            bgfx::setIndexBuffer(planeIndexBuffer);

            bgfx::setTexture(0, u_shadowmap, shadowMap);
            bgfx::setUniform(u_lightmapMtx, lightmapMtx.data());

            bgfx::submit(RENDER_SCENE_ID, sceneProgram);
        }
        {
            std::array<float, 16> trans;
            bx::mtxRotateY(trans.data(), 0.0001f * frame);

            bgfx::setState(
                BGFX_STATE_WRITE_RGB
              | BGFX_STATE_WRITE_A
              | BGFX_STATE_WRITE_Z
              | BGFX_STATE_CULL_CCW
              | BGFX_STATE_DEPTH_TEST_LESS
            );
            
            bgfx::setTransform(trans.data());

            bgfx::setVertexBuffer(0, mokeyVertexBuffer);
            bgfx::setIndexBuffer(mokeyIndexBuffer);

            bgfx::submit(RENDER_SHADOW_ID, shadowProgram);

            bgfx::setState(
                BGFX_STATE_WRITE_RGB
              | BGFX_STATE_WRITE_A
              | BGFX_STATE_WRITE_Z
              | BGFX_STATE_CULL_CCW
              | BGFX_STATE_DEPTH_TEST_LESS
            );

            bgfx::setTransform(trans.data());
            bgfx::setUniform(u_modelMtx, trans.data());

            bgfx::setVertexBuffer(0, mokeyVertexBuffer);
            bgfx::setIndexBuffer(mokeyIndexBuffer);

            bgfx::setTexture(0, u_shadowmap, shadowMap);
            bgfx::setUniform(u_lightmapMtx, lightmapMtx.data());

            bgfx::submit(RENDER_SCENE_ID, sceneProgram);
        }

        bgfx::frame();
        frame++;
    }

    SDL_DestroyWindow(window);

    bgfx::shutdown();

    SDL_Quit();

    return 0;
}
