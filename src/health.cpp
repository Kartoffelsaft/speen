#include "health.h"

void killEntity(EntityId const id) {
    if(entitySystem.entityHasComponent<DeathComponent>(id)) entitySystem.getComponentData<DeathComponent>(id).onDeath(id);
    entitySystem.queueRemoveEntity(id);
}

bool LifetimeComponent::age(EntityId const id, float const delta) {
    if(auto* time = std::get_if<float>(&timeRemaining)) {
        *time -= delta;
        if(*time <= 0) {
            killEntity(id);
            return true;
        }
    } else if (auto* frames = std::get_if<int>(&timeRemaining)) {
        *frames -= 1;
        if(*frames <= 0) {
            killEntity(id);
            return true;
        }
    }

    return false;
}

bool HealthComponent::damage(EntityId const id, float const amount) {
    if(health <= 0) return false;

    this->health -= amount;
    if(health <= 0) {
        killEntity(id);
        return true;
    }

    return false;
}