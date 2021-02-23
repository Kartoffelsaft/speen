#include "entitySystem.h"

EntityId EntitySystem::newEntity() {
    auto const nid = nextEntityId++;
    entities.insert(nid);
    return nid;
}

void EntitySystem::removeEntity(EntityId const id) {
    entities.erase(id);

    for(auto& [_, component]: components) if(component->containsEntity(id)) {
        component->removeEntity(id);
    }
}
