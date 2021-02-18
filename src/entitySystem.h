#pragma once

#include <cstdlib>
#include <unordered_set>
#include <unordered_map>
#include <typeinfo>
#include <ranges>

using ComponentId = std::size_t;
using EntityId = std::size_t;

struct Component {
    // So that the type doesn't have to be known to do certain things
    virtual void removeEntity(EntityId const) {/* dummy */}
    virtual bool containsEntity(EntityId const) const { return false; }
};

template<typename T>
struct ComponentType: std::unordered_map<EntityId, T>, Component {
    static constexpr ComponentId id() { return typeid(T).hash_code(); }
    void removeEntity(EntityId const id) override { this->erase(id); }
    bool containsEntity(EntityId const id) const override { return this->contains(id); }
};

struct EntitySystem {
    std::unordered_set<EntityId> entities;

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
    
    template<typename T>
    bool entityHasComponent(EntityId const id) {
        return getComponent<T>().containsEntity(id);
    }
    
    template<typename T>
    std::unordered_set<EntityId> filterByComponent(std::unordered_set<EntityId>&& ids) {
        auto const & component = getComponent<T>();
        for(auto e = ids.begin(); e != ids.end();) if(!component.containsEntity(*e)) {
            e = ids.erase(e);
        } else {e++;}
        return std::move(ids);
    }
    
    template<typename T>
    std::unordered_set<EntityId> filterByComponent() {
        std::unordered_set<EntityId> ret;
        ret.reserve(entities.size());
        auto const & component = getComponent<T>();
        for(auto const & e: entities) if(component.containsEntity(e)) {
            ret.insert(e);
        }
        return ret;
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
