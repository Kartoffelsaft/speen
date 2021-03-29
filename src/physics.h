#pragma once
#include "entitySystem.h"

struct Collidable {
    float collisionRange;
    void (*onCollision)(EntityId const id, EntityId const otherId) = nullptr;
};

struct PhysicsComponent {
    float posX = 0.f;
    float posY = 0.f;
    float posZ = 0.f;
    float velX = 0.f;
    float velY = 0.f;
    float velZ = 0.f;

    bool grounded = false;

    std::optional<Collidable> collidable = std::nullopt;

    void step(EntityId const id, float const delta);
};
