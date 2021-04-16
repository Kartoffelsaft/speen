#include <random>

#include "explosion.h"
#include "modelInstance.h"
#include "health.h"

EntityId createExplosion(Vec3 const & pos, float const radius) {
    auto explosion = entitySystem.newEntity();
    static std::default_random_engine rng{std::random_device{}()};
    std::uniform_real_distribution<> rotationGenerator(0.f, 360.f);

    entitySystem.addComponent(explosion, ModelInstance::fromModelPtr(LOAD_MODEL("explosion.glb")));
    entitySystem.addComponent(explosion, LifetimeComponent{.timeRemaining = 1.f,});

    Mat4 rotation;
    bx::mtxRotateXYZ(rotation.data(), rotationGenerator(rng), rotationGenerator(rng), rotationGenerator(rng));
    Mat4 scale;
    bx::mtxScale(scale.data(), radius);
    Mat4 position = {
        1.f  , 0.f  , 0.f  , 0.f,
        0.f  , 1.f  , 0.f  , 0.f,
        0.f  , 0.f  , 1.f  , 0.f,
        pos.x, pos.y, pos.z, 1.f,
    };

    entitySystem.getComponentData<ModelInstance>(explosion).orientation =
        position * rotation * scale;

    return explosion;
}