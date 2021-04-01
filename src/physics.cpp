#include "physics.h"
#include "modelInstance.h"
#include "chunk.h"

void PhysicsComponent::step(EntityId const id, float const delta) {
    velocity += accelleration * delta;
    position += velocity * delta;

    switch (type) {
        case PhysicsType::Floating:
            break;
        case PhysicsType::Grounded:
            position.y = world.sampleHeight(position.x, position.z);
            break;
        case PhysicsType::Bouncy:
            auto h = world.sampleHeight(position.x, position.z);
            if(position.y < h) {
                position.y = h;
                auto norm = world.getWorldNormal(position.x, position.z);
                velocity += norm * velocity.dot(norm) * -2.f;
            }
            break;
    }

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
              (this->collidable->layer & other.collidable->layer)
            ||(this->collidable->mask  & other.collidable->layer)
            )) {
                auto collDst = other.collidable->collisionRange + this->collidable->collisionRange;
                if(((this->position + this->collidable->colliderOffset) - (other.position + other.collidable->colliderOffset)).lengthSquared() < collDst * collDst) {
                    this->collidable->onCollision(id, otherId);
                }
            }
        }
    }
}
