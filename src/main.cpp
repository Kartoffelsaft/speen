#include <stdio.h>
#include <vector>
#include <array>
#include <string>
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

bgfx::ProgramHandle shaderProgram;

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

    tinygltf::TinyGLTF modelLoader;

    Model const mokey = [&](){
        uint8_t const mokey[] = {
            #include "../cookedModels/mokey.glb.h"
        };
        return Model::loadFromGLBData(modelLoader, mokey, sizeof(mokey));
    }();

    auto const & layout = mokey.primitives[0].layout;
    auto const & vertecies = mokey.primitives[0].vertexData;
    auto const & indices = mokey.primitives[0].indexData;

    auto vertexBuffer = bgfx::createVertexBuffer(
        bgfx::makeRef(vertecies.data(), vertecies.size()), 
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
        bx::Vec3 eye = {5.f, 4.f, 3.f};

        std::array<float, 16> view;
        bx::mtxLookAt(view.data(), eye, at);

        std::array<float, 16> projection;
        bx::mtxProj(
            projection.data(),
            50.f,
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
              | BGFX_STATE_DEPTH_TEST_LESS
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
