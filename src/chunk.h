#pragma once

#include <array>
#include <memory>

#include "model.h"

struct Tile {
    int height;
};

struct Chunk {
    std::array<Tile, 16 * 16> tiles;

    Model asModel();

    static Chunk generate();
};
