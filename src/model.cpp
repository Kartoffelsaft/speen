#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "model.h"

Model Model::loadFromGLBData(
    tinygltf::TinyGLTF& loader, 
    uint8_t const * const data, 
    size_t const size
) {
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

    if(!err.empty()) {
        fprintf(stderr, "Error loading file: %s\n", err.c_str());
    }

    return loadFromGLTFModel(model);
}

Model Model::loadFromGLBFile(
    tinygltf::TinyGLTF& loader, std::string const & file
) {
    tinygltf::Model model;
    std::string err;
    std::string warn;
    loader.LoadBinaryFromFile(
        &model,
        &err,
        &warn,
        file
    );

    if(!err.empty()) {
        fprintf(stderr, "Error loading file: %s\n", err.c_str());
    }

    return loadFromGLTFModel(model);
}

Model Model::loadFromGLTFModel(
    tinygltf::Model const & model
) {
    decltype(primitives) retPrimitives;
    
    for(auto const & mesh: model.meshes) for(auto const & rawPrimitive: mesh.primitives) {
        // from here until the unpacking most of what you're looking at here is basically copying data from
        // the gltf file into various std::vectors, while using retLayout, vertexElementSize and vertexCount
        // to keep track of what has been written
        // If you want details, ctrl+f for '.contains("POSITION")', as it is a good example of how it's copied
        
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
            for(std::size_t i = 0; i < accessor.count; i++) {
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

        // This attribute should come right after whatever has been previously copied
        // so it's offset is set to however big that is
        auto positionOffset = vertexElementSize;
        if(rawPrimitive.attributes.contains("POSITION")) {
            // Since we know now that the position attribute is going to be a part of each vertex,
            // We'll have to specify that each vertex needs a Vec3's size worth of extra space to fit
            vertexElementSize += 3 * sizeof(float);
            
            // Three star programming in action!\s
            // Basically, the attribute gives an index into an accessor, which gives 
            // an index into a bufferView, which gives an index into a buffer
            // and each of those has it's own little nibble of data needed for later
            tinygltf::Accessor const & accessor = model.accessors[rawPrimitive.attributes.at("POSITION")];
            tinygltf::BufferView const & bufferView = model.bufferViews[accessor.bufferView];
            tinygltf::Buffer const & buffer = model.buffers[bufferView.buffer];

            // This is a similar thing to vertexElementSize += 3 * sizeof(float), but set up for 
            // bgfx to be able to understand the vertex data it receives
            retLayout.add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float);
            
            // Technically works without this line, but doing this helps because this will be one
            // big allocation instead of several small ones
            positions.reserve(accessor.count * 3);

            // Remember those nibbles of data? this is where they are used.
            // This is where the attribute data is stored
            float const * const bufferData = 
                (float*)(buffer.data.data() + bufferView.byteOffset + accessor.byteOffset);
            // Here's going through the data and copying it
            for(std::size_t i = 0; i < accessor.count * 3; i++) {
                positions.push_back(bufferData[i]);
            }

            // This only needs to be done once, because if there is n number of positions,
            // there will be n normals, n texCoords, etc.
            vertexCount = accessor.count;
        } else {
            fprintf(stderr, "A 3D model without vertices is a little odd, don't you think?\n");
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
            if(!padWithAlpha) { for(std::size_t i = 0; i < accessor.count; i++) {
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
            }} else { for(std::size_t i = 0; i < accessor.count; i++) {
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
            for(std::size_t i = 0; i < accessor.count * 3; i++) {
                normals.push_back(bufferData[i]);
            }
        }

        retLayout.end();
        
        // Most of the processing done here involves converting from a packed array to
        // an unpacked array. i.e.:
        // { a1, a2, a3, ..., b1, b2, b3, ... c1, c2, c3, ... } to
        // { a1, b1, c1, a2, b2, c2, a3, b3, c3, ... }
        // where a, b, and c are types and the numbers are indices
        
        std::vector<uint8_t> retVertexVector(vertexCount * vertexElementSize);
        if(!positions.empty()) for(std::size_t i = 0; i < vertexCount; i++) {
            *((float*)(retVertexVector.data() + i * vertexElementSize + positionOffset) + 0)
                = positions[i*3 + 0];
            *((float*)(retVertexVector.data() + i * vertexElementSize + positionOffset) + 1)
                = positions[i*3 + 1];
            *((float*)(retVertexVector.data() + i * vertexElementSize + positionOffset) + 2)
                = positions[i*3 + 2];
        }
        if(!colors.empty()) for(std::size_t i = 0; i < vertexCount; i++) {
            *((float*)(retVertexVector.data() + i * vertexElementSize + colorOffset) + 0)
                = colors[i*4 + 0];
            *((float*)(retVertexVector.data() + i * vertexElementSize + colorOffset) + 1)
                = colors[i*4 + 1];
            *((float*)(retVertexVector.data() + i * vertexElementSize + colorOffset) + 2)
                = colors[i*4 + 2];
            *((float*)(retVertexVector.data() + i * vertexElementSize + colorOffset) + 3)
                = colors[i*4 + 3];
        }
        if(!normals.empty()) for(std::size_t i = 0; i < vertexCount; i++) {
            *((float*)(retVertexVector.data() + i * vertexElementSize + normalOffset) + 0)
                = normals[i*3 + 0];
            *((float*)(retVertexVector.data() + i * vertexElementSize + normalOffset) + 1)
                = normals[i*3 + 1];
            *((float*)(retVertexVector.data() + i * vertexElementSize + normalOffset) + 2)
                = normals[i*3 + 2];
        }

        // I would use bgfx::makeRef but for whatever reason it breaks
        auto const retVertexBuffer = bgfx::createVertexBuffer(
            bgfx::copy(retVertexVector.data(), retVertexVector.size() * sizeof(uint8_t)),
            retLayout
        );
        auto const retIndexBuffer  = bgfx::createIndexBuffer(
            bgfx::copy(indices.data(), indices.size() * sizeof(uint32_t)),
            BGFX_BUFFER_INDEX32
        );

        retPrimitives.push_back({
            .vertexBuffer = retVertexBuffer,
            .indexBuffer = retIndexBuffer,
            .layout = retLayout,
        });
    }

    return {
        .primitives = retPrimitives,
    };
}

tinygltf::TinyGLTF gltfLoader; // NOLINT(cert-err58-cpp)

ModelLoader ModelLoader::init() {
#ifdef EMBED_MODEL_FILES
    return ModelLoader{
        .loadedModels = {
            {"./cookedModels/mokey.glb.pmdl", [](){
                uint8_t const data[] = {
#include "../cookedModels/mokey.glb.h"
                };
                return std::make_shared<Model const>(Model::loadFromGLBData(gltfLoader, data, sizeof(data)));
            }()},
            {"./cookedModels/donut.glb.pmdl", [](){
                uint8_t const data[] = {
#include "../cookedModels/donut.glb.h"
                };
                return std::make_shared<Model const>(Model::loadFromGLBData(gltfLoader, data, sizeof(data)));
            }()},
            {"./cookedModels/man.glb.pmdl", [](){
                uint8_t const data[] = {
#include "../cookedModels/man.glb.h"
                };
                return std::make_shared<Model const>(Model::loadFromGLBData(gltfLoader, data, sizeof(data)));
            }()},
            {"./cookedModels/bullet.glb.pmdl", [](){
                uint8_t const data[] = {
#include "../cookedModels/bullet.glb.h"
                };
                return std::make_shared<Model const>(Model::loadFromGLBData(gltfLoader, data, sizeof(data)));
            }()},
            {"./cookedModels/explosion.glb.pmdl", [](){
                uint8_t const data[] = {
#include "../cookedModels/explosion.glb.h"
                };
                return std::make_shared<Model const>(Model::loadFromGLBData(gltfLoader, data, sizeof(data)));
            }()}
            {"./cookedModels/tree.glb.pmdl", [](){
                uint8_t const data[] = {
#include "../cookedModels/tree.glb.h"
                };
                return std::make_shared<Model const>(Model::loadFromGLBData(gltfLoader, data, sizeof(data)));
            }()}
        }
    };
#else
    return ModelLoader();
#endif
}

std::weak_ptr<Model const> ModelLoader::getModel(
    std::string const & name
) {
    if(loadedModels.contains(name)) {
        return loadedModels.at(name);
    } else {
        loadedModels[name] = std::make_shared<Model const>(Model::loadFromGLBFile(gltfLoader, name));
        return loadedModels.at(name);
    }
}

void Model::Primitive::destroy() {
    bgfx::destroy(this->vertexBuffer);
    bgfx::destroy(this->indexBuffer);
}
