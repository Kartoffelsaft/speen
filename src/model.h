#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <bgfx/bgfx.h>
#include <tiny_gltf.h>

struct Model {
    static Model loadFromGLBData(
        tinygltf::TinyGLTF& loader, 
        uint8_t const * const data, 
        size_t const size
    );

    static Model loadFromGLBFile(
        tinygltf::TinyGLTF& loader,
        std::string const & file
    );

    static Model loadFromGLTFModel(
        tinygltf::Model const model
    );

    struct Primitive {
        std::vector<uint8_t> const vertexData;
        std::vector<uint32_t> const indexData;
        bgfx::VertexLayout const layout;
    };

    std::vector<Primitive> primitives;
};

#define LOAD_MODEL(MODEL_NAME) modelLoader.getModel("./cookedModels/" MODEL_NAME ".pmdl")

struct ModelLoader {
    std::weak_ptr<Model const> getModel(std::string const & name);

    std::map<std::string, std::shared_ptr<Model const>> loadedModels;
};

extern ModelLoader modelLoader;
