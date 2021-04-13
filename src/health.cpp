#include "health.h"

bool HealthComponent::damage(EntityId const id, float const amount) {
    if(health <= 0) return false;

    this->health -= amount;
    if(health <= 0) {
        entitySystem.queueRemoveEntity(id);
        return true;
    }

    return false;
}