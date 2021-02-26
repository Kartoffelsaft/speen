#include <bgfx/bgfx.h>

#include "chunk.h"

Chunk Chunk::generate() {
    return {
        // refuses to compile if not wrapped in a constructor (???)
        // and refuses to lint if it is (?????)
        // looks stupid anyways, will probably change this soon
        .tiles = std::array<Tile, 16 * 16>({
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
        })
    };
}

#pragma clang diagnostic push
// because we all know information is lost when converting from 5 to 5.0
#pragma ide diagnostic ignored "cppcoreguidelines-narrowing-conversions"
Model Chunk::asModel() {
    std::vector<float> vertices;
    std::vector<uint32_t> indices;
    vertices.reserve(this->tiles.size() * (3 + 4));
    indices.reserve((this->tiles.size() - 15) * 3 * 2);
    for(int i = 0; i < this->tiles.size(); i++) {
        vertices.push_back((float)(i % 16));
        vertices.push_back((float)(this->tiles[i].height) * 0.35);
        vertices.push_back((float)(i / 16)); // NOLINT(bugprone-integer-division)
        vertices.push_back(this->tiles[i].height * 0.01f + 0.05f);
        vertices.push_back(this->tiles[i].height * 0.03f + 0.6f);
        vertices.push_back(this->tiles[i].height * 0.01f + 0.2f);
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
    for(unsigned int i = 0; i < this->tiles.size() / 4; i++) {
        auto const xtile = 
            (i/8 * 32)     // row
          + ((i * 2) % 16) // column
          + 17;            // down 1 right 1
        auto const onBottomEdge = xtile / 16 == 15;
        auto const onRightEdge  = xtile % 16 == 15;
        for(auto ni: {
            xtile, xtile - 16, xtile - 17, 
            xtile, xtile - 17, xtile - 1
        }) { indices.push_back(ni); }
        if(!onBottomEdge) for(auto ni: {
            xtile, xtile - 1,  xtile + 15, 
            xtile, xtile + 15, xtile + 16
        }) { indices.push_back(ni); }
        if(!onRightEdge) for(auto ni: {
            xtile, xtile - 15, xtile - 16, 
            xtile, xtile + 1,  xtile - 15
        }) { indices.push_back(ni); }
        if((!onBottomEdge) && (!onRightEdge)) for(auto ni: {
            xtile, xtile + 16, xtile + 17, 
            xtile, xtile + 17, xtile + 1
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

    return Model{
        .primitives = {{
            .vertexBuffer = bgfx::createVertexBuffer(vertexData, layout),
            .indexBuffer = bgfx::createIndexBuffer(indexData, BGFX_BUFFER_INDEX32),
            .layout = layout
        }}
    };
}
#pragma clang diagnostic pop
