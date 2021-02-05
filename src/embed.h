#define STRINGIFY(X) STRINGIFY2(X)
#define STRINGIFY2(X) #X
#define INCLUDE_MODEL(MODEL_FILE) STRINGIFY(../cookedModels/MODEL_FILE.h)

[&](){
    uint8_t const data[] = {
#include INCLUDE_MODEL(MODEL_TO_LOAD)
    };
    return Model::loadFromGLBData(TINYGLTF_LOADER, data, sizeof(data));
}();

#undef STRINGIFY
#undef STRINGIFY2
#undef INCLUDE_MODEL
