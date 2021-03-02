#pragma once

#include <array>
#include <memory>

#include "model.h"

struct Tile {
    int height;
};

struct Chunk {
    std::array<Tile, 16 * 16> tiles;

    Model::Primitive asPrimitive(
        int chunkOffsetX,
        int chunkOffsetZ,
        std::optional<Chunk> rightChunk,
        std::optional<Chunk> bottomChunk,
        std::optional<Chunk> bottomRightChunk
    );

    static Chunk generate();

private:
    std::optional<Model::Primitive> primitive;
    uint8_t primitiveGenerationState = 0;
    uint8_t const GENERATION_STATE_RIGHT_FINISHED =       0b0000'0001;
    uint8_t const GENERATION_STATE_BOTTOM_FINISHED =      0b0000'0010;
    uint8_t const GENERATION_STATE_BOTTOM_RIGHT_FINISHED = 0b0000'0100;
};

struct World {
    std::map<std::pair<int, int>, Chunk> chunks;

    Model asModel(int cx, int cz, float renderDistance);
};