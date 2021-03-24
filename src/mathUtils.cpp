#include "mathUtils.h"

std::tuple<int, float> floorFract(float const x) {
    float floor;
    float fract = modff(x, &floor);

    if(fract > 0) return {(int) floor, fract};
    else return {(int) floor - 1, fract + 1};
}

float smoothFloor(float const x) {
    auto [floor, fract] = floorFract(x);
    return floor + 4 * powf(fract - 0.5f, 3);
}

float smoothClamp(float const x, float const min, float const max) {
    float normalized = std::clamp((x - min)/(max - min), 0.f, 1.f);
    auto perlin = [](float const x){ return x * x * x * (x * (x * 6 - 15) + 10); };
    return perlin(normalized) * (max - min) + min;
}

float interpolate(
    float const s00, 
    float const s01, 
    float const s10, 
    float const s11, 
    float const x, 
    float const y
) {
    return  (
        s00 * (1 - x) * (1 - y) +
        s01 * (1 - x) * y +
        s10 * x * (1 - y) +
        s11 * x * y
    );
}
