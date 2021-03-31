#include "physics.h"
#include "modelInstance.h"
#include "chunk.h"

void PhysicsComponent::step(EntityId const id, float const delta) {
    position += velocity * delta;

    if(grounded) position.y = world.sampleHeight(position.x, position.z);

    if(entitySystem.entityHasComponent<ModelInstance>(id)) {
        auto& model = entitySystem.getComponentData<ModelInstance>(id);
        model.orientation[12] = position.x;
        model.orientation[13] = position.y;
        model.orientation[14] = position.z;
    }

    if(collidable && collidable->onCollision) {
        for(auto const & otherId: entitySystem.filterByComponent<PhysicsComponent>()) if(otherId != id) {
            auto& other = entitySystem.getComponentData<PhysicsComponent>(otherId);
            if(other.collidable && (
               this->collidable->layer & other.collidable->layer
            || this->collidable->mask  & other.collidable->layer
            )) {
                auto collDst = other.collidable->collisionRange + this->collidable->collisionRange;
                if((this->position - other.position).lengthSquared() < collDst * collDst) {
                    this->collidable->onCollision(id, otherId);
                }
            }
        }
    }
}
