#pragma once

#include "entitySystem.h"
#include "mathUtils.h"

using CollisionBitmask = uint8_t;

struct Collidable {
    float collisionRange;
    Vec3 colliderOffset = Vec3{0, 0, 0};

    // Before checking for collision (where A is checking for B), A.layer & B.layer || A.mask & B.layer
    // is checked before proceeding
    // layers as of now:
    // 0b0000'0001: Enemies
    // 0b0000'0010: Player
    // 0b0000'0100: Enemy attacks
    // 0b0000'1000: Player attacks
    // 0b0001'0000: Unused
    // 0b0010'0000: Unused
    // 0b0100'0000: Unused
    // 0b1000'0000: Unused
    CollisionBitmask layer;
    CollisionBitmask mask;

    void (*onCollision)(EntityId const id, EntityId const otherId) = nullptr;
};

enum PhysicsType {
    Floating,
    Grounded,
    Bouncy,
};

struct PhysicsComponent {
    Vec3 position = {0.f, 0.f, 0.f};
    Vec3 velocity = {0.f, 0.f, 0.f};
    Vec3 accelleration = {0.f, 0.f, 0.f};

    PhysicsType type = PhysicsType::Floating;

    std::optional<Collidable> collidable = std::nullopt;

    void step(EntityId const id, float const delta);
};
