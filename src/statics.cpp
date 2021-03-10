#pragma clang diagnostic push
#pragma ide diagnostic ignored "cert-err58-cpp"
#include "model.h"
#include "entitySystem.h"
#include "rendererState.h"
#include "config.h"
#include "chunk.h"

// Putting all the statics in here allows enforcing the order they are initialized in
// preventing the static initialization order fiasco

Config config = Config::init();

RendererState rendererState = RendererState::init();

ModelLoader modelLoader = ModelLoader::init();

EntitySystem entitySystem;

World world;


#pragma clang diagnostic pop
