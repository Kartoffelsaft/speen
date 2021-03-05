#include "mathUtils.h"

float smoothFloor(float const x) {
    auto const oldRoundingSetting = std::fegetround();
    std::fesetround(FE_DOWNWARD);
    float floor;
    float fract = modff(x, &floor);
    fesetround(oldRoundingSetting);
    return floor + 4 * powf(fract - 0.5f, 3);
}

float smoothClamp(float const x, float const min, float const max) {
    float normalized = std::clamp((x - min)/(max - min), 0.f, 1.f);
    auto perlin = [](float const x){ return x * x * x * (x * (x * 6 - 15) + 10); };
    return perlin(normalized) * (max - min) + min;
}
