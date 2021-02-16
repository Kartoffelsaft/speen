#pragma once

#include <cstdlib>
#include <set>
#include <unordered_map>
#include <typeinfo>

using ComponentId = std::size_t;
using EntityId = std::size_t;

struct Component {
    // So that the type doesn't have to be known to do certain things
    virtual void removeEntity(EntityId const id) {/* dummy */}
    virtual bool containsEntity(EntityId const id) { return false; }
};

template<typename T>
struct ComponentType: std::unordered_map<EntityId, T>, Component {
    static constexpr ComponentId id() { return typeid(T).hash_code(); }
    void removeEntity(EntityId const id) override { this->erase(id); }
    bool containsEntity(EntityId const id) override { return this->contains(id); }
};

struct EntitySystem {
    std::set<EntityId> entities;

    EntityId newEntity();
    void removeEntity(EntityId const id);

    // c++ requires templates to be instatiable from where they are used, so
    // these definitions are stuck here
    template<typename T>
    void initComponent() {
        auto const nid = ComponentType<T>::id();

        if(components.contains(nid)) {
            return;
        } else {
            components[nid] = new ComponentType<T>();
        }
    }

    template<typename T>
    void addComponent(EntityId const id, T data) {
        getComponent<T>()[id] = data;
    }

    template<typename T>
    T* getComponentData(EntityId const id) {
        return &getComponent<T>().at(id);
    }

private:

    EntityId nextEntityId = 0;
    
    std::unordered_map<ComponentId, Component*> components;
    template<typename T>
    ComponentType<T>& getComponent() {
        return *static_cast<ComponentType<T>*>(components.at(ComponentType<T>::id()));
    }
};

extern EntitySystem entitySystem;
