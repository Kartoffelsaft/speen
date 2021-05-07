#include "enemy.h"

#include "physics.h"
#include "health.h"
#include "modelInstance.h"

PhysicsComponent enemyPhysicsComponent(Vec3 pos) {
    return PhysicsComponent{
        .position = pos,
        .collidable = Collidable{
            .collisionRange = 1.0,
            .colliderOffset = {0, 1, 0},
            .layer = 0b0000'0001,
            .mask = 0b0000'0001,
        }
    };
}

HealthComponent enemyHealthComponent() {
    return HealthComponent{
        .health = 103.f
    };
}

EntityId createEnemy(Vec3 pos) {
    auto enemy = entitySystem.newEntity();
    entitySystem.addComponent(enemy, "Enemy");
    entitySystem.addComponent(enemy, ModelInstance::fromModelPtr(LOAD_MODEL("man.glb")));
    entitySystem.addComponent(enemy, enemyPhysicsComponent(pos));
    entitySystem.addComponent(enemy, enemyHealthComponent());

    return enemy;
}