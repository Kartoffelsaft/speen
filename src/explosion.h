#pragma once

#include <tuple>

#include "entitySystem.h"
#include "mathUtils.h"

std::tuple<EntityId, EntityId> createExplosion(Vec3 const & pos, float const radius);