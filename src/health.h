#pragma once

#include "entitySystem.h"

struct DeathComponent {
    void (*onDeath)(EntityId const id);
};

void killEntity(EntityId const id);

struct LifetimeComponent {
    float timeRemaining;

    bool age(EntityId const id, float const delta);
};

struct HealthComponent {
    float health;
    float maxHealth = health;

    bool damage(EntityId const id, float const amount);
};