#pragma once
#include "entitySystem.h"

struct PhysicsComponent {
    float posX;
    float posY;
    float posZ;
    float velX;
    float velY;
    float velZ;

    void step(EntityId const id, float const delta);
};
