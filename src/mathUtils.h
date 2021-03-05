#pragma once

#include <array>
#include <algorithm>
#include <cmath>
#include <cfenv>

using Mat4 = std::array<float, 16>;

// https://en.wikipedia.org/wiki/Kernel_(image_processing)
template<std::size_t ImgSize, std::size_t ConvSize>
std::array<float, (ImgSize - ConvSize + 1) * (ImgSize - ConvSize + 1)> convolute(
    std::array<float, ImgSize * ImgSize> const image,
    std::array<float, ConvSize * ConvSize> const kernel,
    float const scale = 1.f
) {
    auto const outSize = ImgSize - ConvSize + 1;
    std::array<float, outSize * outSize> ret;

    for(int i = 0; i < outSize; i++) for(int j = 0; j < outSize; j++) {
        ret[i * outSize + j] = [=](){
            float sum = 0.f;
            for(int ki = 0; ki < ConvSize; ki++) for(int kj = 0; kj < ConvSize; kj++) {
                sum += image[(i + ki) * ImgSize + j + kj] * kernel[ki * ConvSize + kj];
            }
            return sum;
        }() * scale;
    }

    return ret;
}

float smoothFloor(float const x);
float smoothClamp(float const x, float const min, float const max);