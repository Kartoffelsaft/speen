#include <random>

#include "explosion.h"
#include "modelInstance.h"
#include "health.h"
#include "physics.h"

std::tuple<EntityId, EntityId> createExplosion(Vec3 const & pos, float const radius) {
    auto explosionVisual = entitySystem.newEntity(); // separate entities because the visual lasts much longer
    auto explosionPhysical = entitySystem.newEntity();

    static std::default_random_engine rng{std::random_device{}()};
    std::uniform_real_distribution<> rotationGenerator(0.f, 360.f);

    entitySystem.addComponent(explosionVisual, ModelInstance::fromModelPtr(LOAD_MODEL("explosion.glb")));
    entitySystem.addComponent(explosionVisual, LifetimeComponent{.timeRemaining = 1.f,});

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

    entitySystem.getComponentData<ModelInstance>(explosionVisual).orientation =
        position * rotation * scale;

    entitySystem.addComponent(explosionPhysical, PhysicsComponent{
        .position = pos,
        .type = PhysicsType::Floating,
        .collidable = Collidable{
            .collisionRange = radius,
            .layer = 0b0000'1000, // TODO: Paramaterize layer/mask
            .mask = 0b0000'1001,
            .onCollision = [](EntityId const id, EntityId const otherId) {
                entitySystem.queueRemoveEntity(id);
                if(entitySystem.entityHasComponent<HealthComponent>(otherId)) {
                    entitySystem.getComponentData<HealthComponent>(otherId).damage(otherId, 50);
                }
            }
        }
    });

    // the damaging part of the explosion should exist for at most one frame
    entitySystem.addComponent(explosionPhysical, LifetimeComponent{.timeRemaining = 1});

    return {explosionVisual, explosionPhysical};
}