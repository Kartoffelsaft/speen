#include "mathUtils.h"

std::tuple<int, float> floorFract(float const x) {
    float floor;
    float fract = modff(x, &floor);

    if(fract > 0) return {(int) floor, fract};
    else return {(int) floor - 1, fract + 1};
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
