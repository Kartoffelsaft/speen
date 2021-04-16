#pragma once

#include <variant>

#include "entitySystem.h"

struct DeathComponent {
    void (*onDeath)(EntityId const id);
};

void killEntity(EntityId const id);

struct LifetimeComponent {
    std::variant<float, int> timeRemaining; // float is seconds, int is frames

    bool age(EntityId const id, float const delta);
};

struct HealthComponent {
    float health;
    float maxHealth = health;

    bool damage(EntityId const id, float const amount);
};