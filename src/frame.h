#pragma once
#include "entitySystem.h"

struct OnFrameComponent {
    void (*onFrame)(EntityId const id, float const delta);
};
