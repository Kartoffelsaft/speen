#include <bgfx/bgfx.h>

#include "chunk.h"

Chunk Chunk::generate() {
    Chunk ret;
    ret.tiles = std::array<Tile, 16 * 16>({
        {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {1}, {2}, {2},
        {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {1}, {1}, {6}, {9}, {9}, {2},
        {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {1}, {1}, {1}, {6}, {8}, {9}, {2},
        {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {1}, {6}, {7}, {7}, {1},
        {1}, {1}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {6}, {5}, {5}, {1},
        {1}, {1}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {5}, {3}, {5}, {1},
        {1}, {1}, {1}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {2}, {3}, {3}, {1},
        {1}, {4}, {1}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {1}, {1}, {1}, {1},
        {2}, {5}, {1}, {1}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {1}, {1},
        {2}, {4}, {1}, {1}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {1},
        {2}, {8}, {3}, {1}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {1},
        {2}, {8}, {4}, {1}, {1}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {1},
        {2}, {9}, {8}, {3}, {1}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0},
        {2}, {9}, {9}, {7}, {2}, {1}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0},
        {2}, {8}, {8}, {7}, {3}, {1}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0},
        {2}, {2}, {2}, {1}, {1}, {1}, {1}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}
    });
    return ret;
}

#pragma clang diagnostic push
// because we all know information is lost when converting from 5 to 5.0
#pragma ide diagnostic ignored "cppcoreguidelines-narrowing-conversions"
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

    this->primitiveGenerationState = 0
        | (rightChunk.has_value()? GENERATION_STATE_RIGHT_FINISHED : 0)
        | (bottomChunk.has_value()? GENERATION_STATE_BOTTOM_FINISHED : 0)
        | (bottomRightChunk.has_value()? GENERATION_STATE_BOTTOM_RIGHT_FINISHED : 0);

    std::vector<float> vertices;
    std::vector<uint32_t> indices;
    auto const vertexSize = 3 + 4;
    vertices.reserve((this->tiles.size() + 33) * vertexSize);
    indices.reserve((this->tiles.size()) * 3 * 2);

    float const heightScale = 0.35f;
    float const rColorScale = 0.01f;
    float const gColorScale = 0.03f;
    float const bColorScale = 0.01f;
    float const rColorOffset = 0.05f;
    float const gColorOffset = 0.6f;
    float const bColorOffset = 0.02;

    for(int i = 0; i < this->tiles.size(); i++) {
        vertices.push_back((float)(i % 16 + chunkOffsetX * 16));
        vertices.push_back((float)(this->tiles[i].height) * heightScale);
        vertices.push_back((float)(i / 16 + chunkOffsetZ * 16)); // NOLINT(bugprone-integer-division)
        vertices.push_back(this->tiles[i].height * rColorScale + rColorOffset);
        vertices.push_back(this->tiles[i].height * gColorScale + gColorOffset);
        vertices.push_back(this->tiles[i].height * bColorScale + bColorOffset);
        vertices.push_back(1.f);
    }
    unsigned int const rightChunkOffset = vertices.size() / vertexSize;
    if(rightChunk.has_value()) for(int i = 0; i < 16; i++) {
        vertices.push_back((float)(16 + chunkOffsetX * 16));
        vertices.push_back((float)(rightChunk->tiles[i * 16].height) * heightScale);
        vertices.push_back((float)(i + chunkOffsetZ * 16)); // NOLINT(bugprone-integer-division)
        vertices.push_back(rightChunk->tiles[i * 16].height * rColorScale + rColorOffset);
        vertices.push_back(rightChunk->tiles[i * 16].height * gColorScale + gColorOffset);
        vertices.push_back(rightChunk->tiles[i * 16].height * bColorScale + bColorOffset);
        vertices.push_back(1.f);
    }
    unsigned int const bottomChunkOffset = vertices.size() / vertexSize;
    if(bottomChunk.has_value()) for(int i = 0; i < 16; i++) {
        vertices.push_back((float)(i + chunkOffsetX * 16));
        // yes I know "% 16" is redundant, but it's consistent
        // a good compiler will optimize it out anyways
        vertices.push_back((float)(bottomChunk->tiles[i % 16].height) * heightScale);
        vertices.push_back((float)(16 + chunkOffsetZ * 16)); // NOLINT(bugprone-integer-division)
        vertices.push_back(bottomChunk->tiles[i % 16].height * rColorScale + rColorOffset);
        vertices.push_back(bottomChunk->tiles[i % 16].height * gColorScale + gColorOffset);
        vertices.push_back(bottomChunk->tiles[i % 16].height * bColorScale + bColorOffset);
        vertices.push_back(1.f);
    }
    unsigned int const bottomRightChunkOffset = vertices.size() / vertexSize;
    if(bottomRightChunk.has_value()) {
        vertices.push_back((float)(16 + chunkOffsetX * 16));
        vertices.push_back((float)(bottomRightChunk->tiles[0].height) * heightScale);
        vertices.push_back((float)(16 + chunkOffsetZ * 16)); // NOLINT(bugprone-integer-division)
        vertices.push_back(bottomRightChunk->tiles[0].height * rColorScale + rColorOffset);
        vertices.push_back(bottomRightChunk->tiles[0].height * gColorScale + gColorOffset);
        vertices.push_back(bottomRightChunk->tiles[0].height * bColorScale + bColorOffset);
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
    for(unsigned int i = 0; i < this->tiles.size() / 4; i++) {
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

Model World::asModel(int cx, int cz, float renderDistance) {
    std::vector<Model::Primitive> primitives;
    primitives.reserve((renderDistance * 2 + 1) * (renderDistance * 2 + 1));
    for(int i = cx - renderDistance; i < cx + renderDistance; i++)
        for(int j = cz - renderDistance; j < cz + renderDistance; j++) {
            if(!chunks.contains({i, j})) {
                chunks.emplace(std::make_pair(i, j), Chunk::generate());
            }
        }

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
#pragma clang diagnostic pop
