#define STRINGIFY(X) STRINGIFY2(X)
#define STRINGIFY2(X) #X

[&](){
    return Model::loadFromGLBFile(TINYGLTF_LOADER, "cookedModels/" STRINGIFY(MODEL_TO_LOAD) ".pmdl");
}();

#undef STRINGIFY
#undef STRINGIFY2
