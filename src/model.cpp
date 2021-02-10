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

    if(err.size() > 0) {
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

    if(err.size() > 0) {
        fprintf(stderr, "Error loading file: %s\n", err.c_str());
    }

    return loadFromGLTFModel(model);
}

Model Model::loadFromGLTFModel(
    tinygltf::Model const model
) {
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
