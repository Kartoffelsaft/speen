#include "physics.h"
#include "modelInstance.h"
#include "chunk.h"

void PhysicsComponent::step(EntityId const id, float const delta) {
    this->posX += this->velX * delta;
    this->posY += this->velY * delta;
    this->posZ += this->velZ * delta;

    if(grounded) posY = world.sampleHeight(posX, posZ);

    if(entitySystem.entityHasComponent<ModelInstance>(id)) {
        auto& model = entitySystem.getComponentData<ModelInstance>(id);
        model.orientation[12] = posX;
        model.orientation[13] = posY;
        model.orientation[14] = posZ;
    }
}
