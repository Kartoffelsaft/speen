#pragma once

#include "entitySystem.h"
#include "mathUtils.h"

struct Collidable {
    float collisionRange;
    void (*onCollision)(EntityId const id, EntityId const otherId) = nullptr;
};

struct PhysicsComponent {
    Vec3 position = {0.f, 0.f, 0.f};
    Vec3 velocity = {0.f, 0.f, 0.f};

    bool grounded = false;

    std::optional<Collidable> collidable = std::nullopt;

    void step(EntityId const id, float const delta);
};
