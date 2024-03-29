#include "entitySystem.h"

EntityId EntitySystem::newEntity() {
    auto const nid = nextEntityId++;
    entities.insert(nid);
    return nid;
}

void EntitySystem::removeEntity(EntityId const id) {
    entities.erase(id);
    invalidEntities.insert(id);

    for(auto& [_, component]: components) if(component->containsEntity(id)) {
        component->removeEntity(id);
    }
}

void EntitySystem::queueRemoveEntity(EntityId const id) {
    entityRemovalQueue.emplace(id);
}

void EntitySystem::removeQueuedEntities() {
    for(auto const id: entityRemovalQueue) {
        this->removeEntity(id);
    }

    entityRemovalQueue.clear();
}