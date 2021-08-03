#include <bgfx/bgfx.h>
#include <random>
#include <cmath>
#include <algorithm>

#include "chunk.h"
#include "mathUtils.h"
#include "modelInstance.h"

static auto const chunkIndices = std::vector<std::vector<uint32_t>>{
    #include "./chunkIndices/ind000"
    ,
    #include "./chunkIndices/ind001"
    ,
    #include "./chunkIndices/ind010"
    ,
    #include "./chunkIndices/ind011"
    ,
    #include "./chunkIndices/ind100"
    ,
    #include "./chunkIndices/ind101"
    ,
    #include "./chunkIndices/ind110"
    ,
    #include "./chunkIndices/ind111"
    ,
};

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

    world.patterns = {
        DecoratorPattern{
            .radius = 2,
            .chance = 0.001f,
            .seedOffset = 0xfee3,
            .decorate = [](int x, int z, Tile& on){
                auto tree = entitySystem.newEntity();
                entitySystem.addComponent(tree, "Tree");
                entitySystem.addComponent(tree, ModelInstance::fromModelPtr(LOAD_MODEL("tree.glb")));
                auto& o = entitySystem.getComponentData<ModelInstance>(tree).orientation;
                o[12] = x;
                o[13] = on.height;
                o[14] = z;
            }
        }
    };

    auto terrain = entitySystem.newEntity();
    entitySystem.addComponent(terrain, "World");
    entitySystem.addComponent(terrain, ModelInstance::fromModelPtr(world.updateModel(0, 0, 1)));
    auto& mi = entitySystem.getComponentData<ModelInstance>(terrain);
    mi.mustRender = true;
    auto& terrainOrientation = mi.orientation;
    terrainOrientation[12] = 0.f;
    terrainOrientation[13] = 0.f;
    terrainOrientation[14] = 0.f;

    return terrain;
}

Chunk Chunk::generate(int chunkX, int chunkZ, int seed, std::vector<DecoratorPattern> patterns) {
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

    for(auto pattern: patterns) {
        for(int i = 0; i < 256; i++) {
            auto x = chunkX * 16 + i / 16;
            auto z = chunkZ * 16 + i % 16;

            if(randFromCoord(x, z, seed ^ pattern.seedOffset) < pattern.chance) {
                bool invalidated = false;
                for(int d = 1; d < pattern.radius * pattern.radius; d++) {
                    auto dx = d / pattern.radius;
                    auto dz = d % pattern.radius;

                    invalidated = randFromCoord(x + dx, z + dz, seed ^ pattern.seedOffset) < pattern.chance;

                    if(invalidated) break;
                }

                if(!invalidated) {
                    pattern.decorate(x, z, ret.tiles[(i%16) * 16 + i / 16]);
                }
            }
        }
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
    std::vector<uint32_t> const & indices = chunkIndices[this->primitiveGenerationState];
    auto const vertexSize = 3 + 4;
    vertices.reserve((this->tiles.size() + 33) * vertexSize);

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

    auto vertexData = bgfx::copy(vertices.data(), vertices.size() * sizeof(float));
    auto indexData = bgfx::makeRef(indices.data(), indices.size() * sizeof(uint32_t));
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
                chunks.emplace(std::make_pair(i, j), Chunk::generate(i, j, this->worldSeed, this->patterns));
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
