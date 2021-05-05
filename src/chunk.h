#pragma once

#include <array>
#include <memory>
#include <set>

#include "model.h"
#include "entitySystem.h"
#include "mathUtils.h"

EntityId createWorldEntity();

struct Tile {
    float height;
    enum Type {
        Grass,
        Blasted,
    }; Type type;

    constexpr RGB<float> color() const;
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
    std::set<std::pair<int, int>> outdatedChunks;
    int const worldSeed = 666666;

    std::optional<std::shared_ptr<Model>> model;

    std::weak_ptr<Model> updateModel(int cx, int cz, int renderDistance);

    /**
     * @brief Get a mutable pointer to a tile
     * 
     * @param x 
     * @param z 
     * @return Tile*. Will be a nullptr if the tile has not been generated yet
     */
    Tile* getTileMut(int x, int z);

    /**
     * @brief Get a pointer to a tile
     * Like getTileMut, but const
     * 
     * @param x 
     * @param z 
     * @return Tile const*. Will be a nullptr if the tile has not been generated yet
     */
    Tile const * getTile(int x, int z) const;

    /**
     * @brief Get the height of the world at a given coordinate
     * Allows for values between tiles
     * 
     * @param x 
     * @param z 
     * @return std::optional<float> 
     */
    std::optional<float> sampleHeight(float x, float z);


    std::optional<Vec3> getWorldNormal(float x, float z);

private:
    int oldCx = -999;
    int oldCz = -998;
    int oldRenderDistance = -1;

    Model asModel(int cx, int cz, int renderDistance, int unloadDistance);
};

extern World world;
