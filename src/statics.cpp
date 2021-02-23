#include "model.h"
#include "entitySystem.h"
#include "rendererState.h"
#include "config.h"

Config config = Config::init();

RendererState rendererState = RendererState::init();

ModelLoader modelLoader = ModelLoader::init();

EntitySystem entitySystem;

