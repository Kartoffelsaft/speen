#pragma once

#include "entitySystem.h"
#include "mathUtils.h"

using CollisionBitmask = uint8_t;

struct Collidable {
    float collisionRange;

    // Before checking for collision (where A is checking for B), A.layer & B.layer || A.mask & B.layer
    // is checked before proceeding
    CollisionBitmask layer = 0b0000'0001;
    CollisionBitmask mask  = 0b0000'0001;

    void (*onCollision)(EntityId const id, EntityId const otherId) = nullptr;
};

struct PhysicsComponent {
    Vec3 position = {0.f, 0.f, 0.f};
    Vec3 velocity = {0.f, 0.f, 0.f};

    bool grounded = false;

    std::optional<Collidable> collidable = std::nullopt;

    void step(EntityId const id, float const delta);
};
