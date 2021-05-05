#include <bgfx/bgfx.h>
#include <random>
#include <cmath>
#include <algorithm>

#include "chunk.h"
#include "mathUtils.h"
#include "modelInstance.h"

constexpr RGB<float> Tile::color() const {
    float const rGrassColorScale = 0.01f;
    float const gGrassColorScale = 0.02f;
    float const bGrassColorScale = 0.01f;
    float const rGrassColorOffset = 0.05f;
    float const gGrassColorOffset = 0.7f;
    float const bGrassColorOffset = 0.02;
    switch(this->type) {
    case Tile::Type::Grass:

        return {
            .r = this->height * rGrassColorScale + rGrassColorOffset,
            .g = this->height * gGrassColorScale + gGrassColorOffset,
            .b = this->height * bGrassColorScale + bGrassColorOffset,
        };
    case Tile::Type::Blasted:
        return {
            .r = 0.23f,
            .g = 0.23f,
            .b = 0.23f,
        };
    }
}

EntityId createWorldEntity() { 
    auto terrain = entitySystem.newEntity();
    entitySystem.addComponent(terrain, ModelInstance::fromModelPtr(world.updateModel(0, 0, 1)));
    auto& mi = entitySystem.getComponentData<ModelInstance>(terrain);
    mi.mustRender = true;
    auto& terrainOrientation = mi.orientation;
    terrainOrientation[12] = 0.f;
    terrainOrientation[13] = 0.f;
    terrainOrientation[14] = 0.f;

    return terrain;
}

Chunk Chunk::generate(int chunkX, int chunkZ, int seed) {
    Chunk ret;

    auto preConvHeightMap = generateNoise<18>(
        seed,
        chunkX * 16 - 1,
        chunkZ * 16 - 1,
        {{0.3f, 0.6f, 4}, {0.04f, 6.f, 7}, {0.6f, 0.2f, 333}}
    );
    auto postConvHeightMap = convolute<18, 3>(preConvHeightMap, {
        1.f, 1.f, 1.f,
        1.f, 4.f, 1.f,
        1.f, 1.f, 1.f
    }, 1.f/12);

    for(std::size_t i = 0; i < ret.tiles.size(); i++) {
        ret.tiles[i] = {
            .height = postConvHeightMap[i] * 5 - 20,
            .type = Tile::Type::Grass,
        };
    }

    return ret;
}

Model::Primitive Chunk::asPrimitive(
    int const chunkOffsetX,
    int const chunkOffsetZ,
    std::optional<Chunk> rightChunk,
    std::optional<Chunk> bottomChunk,
    std::optional<Chunk> bottomRightChunk
) {
    if(this->primitive.has_value()
    && (rightChunk.has_value()       <= (this->primitiveGenerationState & GENERATION_STATE_RIGHT_FINISHED))
    && (bottomChunk.has_value()      <= (this->primitiveGenerationState & GENERATION_STATE_BOTTOM_FINISHED))
    && (bottomRightChunk.has_value() <= (this->primitiveGenerationState & GENERATION_STATE_BOTTOM_RIGHT_FINISHED))
    ) {
        return this->primitive.value();
    }

    this->unloadPrimitive();

    this->primitiveGenerationState = 0
        | (rightChunk.has_value()? GENERATION_STATE_RIGHT_FINISHED : 0)
        | (bottomChunk.has_value()? GENERATION_STATE_BOTTOM_FINISHED : 0)
        | (bottomRightChunk.has_value()? GENERATION_STATE_BOTTOM_RIGHT_FINISHED : 0);

    std::vector<float> vertices;
    std::vector<uint32_t> indices;
    auto const vertexSize = 3 + 4;
    vertices.reserve((this->tiles.size() + 33) * vertexSize);
    indices.reserve((this->tiles.size()) * 3 * 2);

    for(int64_t i = 0; i < (int64_t)this->tiles.size(); i++) {
        vertices.push_back((float)(i % 16 + chunkOffsetX * 16));
        vertices.push_back((float)(this->tiles[i].height));
        vertices.push_back((float)(i / 16 + chunkOffsetZ * 16)); // NOLINT(bugprone-integer-division)
        this->tiles[i].color().appendToVector(vertices);
        vertices.push_back(1.f);
    }
    std::size_t const rightChunkOffset = vertices.size() / vertexSize;
    if(rightChunk.has_value()) for(int i = 0; i < 16; i++) {
        vertices.push_back((float)(16 + chunkOffsetX * 16));
        vertices.push_back((float)(rightChunk->tiles[i * 16].height));
        vertices.push_back((float)(i + chunkOffsetZ * 16)); // NOLINT(bugprone-integer-division)
        rightChunk->tiles[i * 16].color().appendToVector(vertices);
        vertices.push_back(1.f);
    }
    std::size_t const bottomChunkOffset = vertices.size() / vertexSize;
    if(bottomChunk.has_value()) for(int i = 0; i < 16; i++) {
        vertices.push_back((float)(i + chunkOffsetX * 16));
        // yes I know "% 16" is redundant, but it's consistent
        // a good compiler will optimize it out anyways
        vertices.push_back((float)(bottomChunk->tiles[i % 16].height));
        vertices.push_back((float)(16 + chunkOffsetZ * 16)); // NOLINT(bugprone-integer-division)
        bottomChunk->tiles[i % 16].color().appendToVector(vertices);
        vertices.push_back(1.f);
    }
    std::size_t const bottomRightChunkOffset = vertices.size() / vertexSize;
    if(bottomRightChunk.has_value()) {
        vertices.push_back((float)(16 + chunkOffsetX * 16));
        vertices.push_back((float)(bottomRightChunk->tiles[0].height));
        vertices.push_back((float)(16 + chunkOffsetZ * 16)); // NOLINT(bugprone-integer-division)
        bottomRightChunk->tiles[0].color().appendToVector(vertices);
        vertices.push_back(1.f);
    }

    // the tris are shaped like this:
    // t t t t t    ┌─┬─┬─┬─┐
    //              │╲│╱│╲│╱│
    // t x t x t -> ├─┼─┼─┼─┤
    //              │╱│╲│╱│╲│
    // t t t t t    └─┴─┴─┴─┘
    //
    // where the t's are regular tiles and the x's are "xtile"s

    // Also, holy crap this will need cleaning up
    for(std::size_t i = 0; i < this->tiles.size() / 4; i++) {
        auto const xtile = 
            (i/8 * 32)     // row
          + ((i * 2) % 16) // column
          + 17;            // down 1 right 1
        auto const xtileRow = xtile / 16;
        auto const xtileColumn = xtile % 16;
        auto const onBottomEdge = xtileRow == 15;
        auto const onRightEdge  = xtileColumn == 15;
        for(auto ni: {
            xtile, xtile - 16, xtile - 17, 
            xtile, xtile - 17, xtile - 1
        }) { indices.push_back(ni); }
        if(!onBottomEdge) for(auto ni: {
            xtile, xtile - 1,  xtile + 15, 
            xtile, xtile + 15, xtile + 16
        }) { indices.push_back(ni); }
        else if(bottomChunk.has_value()) for(auto ni: {
            xtile, xtile - 1, bottomChunkOffset + xtileColumn - 1,
            xtile, bottomChunkOffset + xtileColumn - 1, bottomChunkOffset + xtileColumn
        }) { indices.push_back(ni); }
        if(!onRightEdge) for(auto ni: {
            xtile, xtile - 15, xtile - 16, 
            xtile, xtile + 1,  xtile - 15
        }) { indices.push_back(ni); }
        else if(rightChunk.has_value()) for(auto ni: {
            xtile, rightChunkOffset + xtileRow - 1, xtile - 16,
            xtile, rightChunkOffset + xtileRow, rightChunkOffset + xtileRow - 1
        }) { indices.push_back(ni); }
        if((!onBottomEdge) && (!onRightEdge)) for(auto ni: {
            xtile, xtile + 16, xtile + 17, 
            xtile, xtile + 17, xtile + 1
        }) { indices.push_back(ni); }
        else if(
            onBottomEdge
         && onRightEdge
         && bottomRightChunk.has_value()
         && bottomChunk.has_value()
         && rightChunk.has_value()) for(auto ni: {
            xtile, bottomChunkOffset + xtileColumn, bottomRightChunkOffset,
            xtile, bottomRightChunkOffset, rightChunkOffset + xtileRow
        }) { indices.push_back(ni); }
        else if((onBottomEdge) && (!onRightEdge) && bottomChunk.has_value()) for(auto ni: {
            xtile, bottomChunkOffset + xtileColumn, bottomChunkOffset + xtileColumn + 1,
            xtile, bottomChunkOffset + xtileColumn + 1, xtile + 1
        }) { indices.push_back(ni); }
        else if((!onBottomEdge) && (onRightEdge) && rightChunk.has_value()) for(auto ni: {
            xtile, xtile + 16, rightChunkOffset + xtileRow + 1,
            xtile, rightChunkOffset + xtileRow + 1, rightChunkOffset + xtileRow
        }) { indices.push_back(ni); }
    }

    auto vertexData = bgfx::copy(vertices.data(), vertices.size() * sizeof(float));
    auto indexData = bgfx::copy(indices.data(), indices.size() * sizeof(uint32_t));
    bgfx::VertexLayout layout;
    layout
        .begin()
        .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
        .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Float)
        .end();

    this->primitive.emplace(Model::Primitive{
        .vertexBuffer = bgfx::createVertexBuffer(vertexData, layout),
        .indexBuffer = bgfx::createIndexBuffer(indexData, BGFX_BUFFER_INDEX32),
        .layout = layout
    });

    return this->primitive.value();
}

void Chunk::unloadPrimitive() {
    if(this->primitive.has_value()) {
        this->primitive.value().destroy();
        this->primitive.reset();
        this->primitiveGenerationState = 0;
    }
}

std::weak_ptr<Model> World::updateModel(int cx, int cz, int renderDistance) {
    if(
        this->outdatedChunks.size() > 0
     || cx != this->oldCx
     || cz != this->oldCz
     || renderDistance != this->oldRenderDistance
     || !this->model.has_value()
    ) {
        if(this->model.has_value()) {
            *this->model.value() = this->asModel(cx, cz, renderDistance, renderDistance + 2);
        } else {
            this->model = std::make_optional(std::make_shared<Model>(this->asModel(cx, cz, renderDistance, renderDistance + 2)));
        }
        this->oldCx = cx;
        this->oldCz = cz;
        this->oldRenderDistance = renderDistance;
    }

    return this->model.value();
}

Model World::asModel(int cx, int cz, int renderDistance, int unloadDistance) {
    std::vector<Model::Primitive> primitives;
    primitives.reserve((renderDistance * 2 + 1) * (renderDistance * 2 + 1));
    for(int i = cx - renderDistance; i < cx + renderDistance; i++)
        for(int j = cz - renderDistance; j < cz + renderDistance; j++) {
            if(!chunks.contains({i, j})) {
                chunks.emplace(std::make_pair(i, j), Chunk::generate(i, j, this->worldSeed));
            }
        }

    for(auto & [coord, chunk]: chunks) {
        auto [x, z] = coord;
        if(
            outdatedChunks.contains({x, z})
         || x < cx - unloadDistance
         || x > cx + unloadDistance
         || z < cz - unloadDistance
         || z > cz + unloadDistance
        ) {
            chunk.unloadPrimitive();
        }
    }

    outdatedChunks.clear();

    for(int i = cx - renderDistance; i < cx + renderDistance; i++)
        for(int j = cz - renderDistance; j < cz + renderDistance; j++) {
            std::optional<Chunk> rightChunk = chunks.contains({i + 1, j})?
                std::make_optional(chunks.at({i + 1, j}))
                    :
                std::nullopt;
            std::optional<Chunk> bottomChunk = chunks.contains({i, j + 1})?
                std::make_optional(chunks.at({i, j + 1}))
                    :
                std::nullopt;
            std::optional<Chunk> bottomRightChunk = chunks.contains({i + 1, j + 1})?
                std::make_optional(chunks.at({i + 1, j + 1}))
                    :
                std::nullopt;
            primitives.push_back(chunks.at({i, j}).asPrimitive(i, j, rightChunk, bottomChunk, bottomRightChunk));
        }

    return Model{
        .primitives = primitives
    };
}

bool World::withinRenderDistance(ModelInstance const & mod) const {
    // using the old render distance values because I'm lazy
    // they get updated frequently anyways
    auto dx = std::abs(this->oldCx - (int)(mod.orientation[12] / 16));
    auto dz = std::abs(this->oldCz - (int)(mod.orientation[14] / 16));

    return dx <= oldRenderDistance
        && dz <= oldRenderDistance;
}

Tile* World::getTileMut(int x, int z) {
    auto xdiv = std::div(x, 16);
    if(xdiv.rem < 0) {
        xdiv.rem += 16;
        xdiv.quot -= 1;
    }
    auto zdiv = std::div(z, 16);
    if(zdiv.rem < 0) {
        zdiv.rem += 16;
        zdiv.quot -= 1;
    }

    if(!chunks.contains({xdiv.quot, zdiv.quot})) {
        return nullptr;
    }

    // if the caller is using the mutable version, then it's probably getting mutated
    // therefor this chunk is likely outdated (and adjacent ones if it's on the edge)
    // TODO: deal with corners too
    outdatedChunks.insert({xdiv.quot, zdiv.quot});
    if(xdiv.rem == 0 ) outdatedChunks.insert({xdiv.quot - 1, zdiv.quot     });
    if(xdiv.rem == 15) outdatedChunks.insert({xdiv.quot + 1, zdiv.quot     });
    if(zdiv.rem == 0 ) outdatedChunks.insert({xdiv.quot    , zdiv.quot  - 1});
    if(zdiv.rem == 15) outdatedChunks.insert({xdiv.quot    , zdiv.quot  + 1});

    return &chunks.at({xdiv.quot, zdiv.quot}).tiles[zdiv.rem * 16 + xdiv.rem];
}

Tile const * World::getTile(int x, int z) const {
    auto xdiv = std::div(x, 16);
    if(xdiv.rem < 0) {
        xdiv.rem += 16;
        xdiv.quot -= 1;
    }
    auto zdiv = std::div(z, 16);
    if(zdiv.rem < 0) {
        zdiv.rem += 16;
        zdiv.quot -= 1;
    }

    if(!chunks.contains({xdiv.quot, zdiv.quot})) {
        return nullptr;
    }

    return &chunks.at({xdiv.quot, zdiv.quot}).tiles[zdiv.rem * 16 + xdiv.rem];
}

std::optional<float> World::sampleHeight(float x, float z) {
    auto [ix, rx] = floorFract(x);
    auto [iz, rz] = floorFract(z);
    
    auto r00 = this->getTile(ix    , iz    );
    auto r01 = this->getTile(ix    , iz + 1);
    auto r10 = this->getTile(ix + 1, iz    );
    auto r11 = this->getTile(ix + 1, iz + 1);

    if(r00 && r01 && r10 && r11) {
        return interpolate(
            r00->height, 
            r01->height, 
            r10->height, 
            r11->height, 
            rx, rz
        );
    } else {
        return std::nullopt;
    }
}

std::optional<Vec3> World::getWorldNormal(float x, float z) {
    // would look a lot simpler with rust's '?' operator
    auto c = sampleHeight(x, z);
    auto a = sampleHeight(x, z + 0.1f);
    auto b = sampleHeight(x + 0.1f, z);

    if(c && a && b) {
        auto o =  Vec3{x       , c.value(), z       };
        auto pa = Vec3{x       , a.value(), z + 0.1f};
        auto pb = Vec3{x + 0.1f, b.value(), z       };

        return ((pa - o).cross(pb - o)).normalized();
    } else {
        return std::nullopt;
    }
}
