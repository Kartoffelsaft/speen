#pragma once

#include <array>
#include <memory>

#include "model.h"

struct Tile {
    float height;
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

    void unloadPrimitive();

    static Chunk generate(int chunkX, int chunkZ, int seed);

private:
    std::optional<Model::Primitive> primitive;
    uint8_t primitiveGenerationState = 0;
    uint8_t const GENERATION_STATE_RIGHT_FINISHED =       0b0000'0001;
    uint8_t const GENERATION_STATE_BOTTOM_FINISHED =      0b0000'0010;
    uint8_t const GENERATION_STATE_BOTTOM_RIGHT_FINISHED = 0b0000'0100;
};

struct World {
    std::map<std::pair<int, int>, Chunk> chunks;
    int const worldSeed = 666666;

    std::optional<std::shared_ptr<Model>> model;

    std::weak_ptr<Model> updateModel(int cx, int cz, int renderDistance);

    Tile& getTile(int x, int z);
    float sampleHeight(float x, float z);

private:
    int oldCx = -999;
    int oldCz = -998;
    int oldRenderDistance = -1;

    Model asModel(int cx, int cz, int renderDistance, int unloadDistance);
};

extern World world;
