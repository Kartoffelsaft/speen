#pragma once

#include "entitySystem.h"

struct HealthComponent {
    float health;
    float maxHealth = health;

    bool damage(EntityId const id, float const amount);
    void (*onDeath)(EntityId const id) = nullptr;
};