#pragma once

#include <array>
#include <vector>
#include <tuple>
#include <random>
#include <algorithm>
#include <cmath>
#include <cfenv>

using Mat4 = std::array<float, 16>;

std::tuple<int, float> floorFract(float const x);

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

template<std::size_t ImgSize>
std::array<float, ImgSize * ImgSize> generateNoise(
    std::size_t const seed,
    int const offsetX,
    int const offsetY,
    //                     scale  amplify seed offset
    std::vector<std::tuple<float, float, std::size_t>> const & resolutions
) {
    std::array<float, ImgSize * ImgSize> ret;
    ret.fill(0.f);

    auto getPsuedoRand = [=](int x, int y, std::size_t seedOffset){
        return std::hash<std::size_t>()(
            std::hash<double>()(std::tan(x * x * y * 137.54 + 0.3))
          ^ std::hash<double>()(std::atanh(y * y * x * 805.87 + 0.84))
          ^ seed
          ^ seedOffset
        ) / (double) std::numeric_limits<std::size_t>::max();
    };

    for(auto const & [scale, amplification, seedOffset]: resolutions) {
        for(int i = 0; i < ImgSize; i++) for(int j = 0; j < ImgSize; j++) {
            auto [u, uFract] = floorFract((i + offsetX) * scale);
            auto [v, vFract] = floorFract((j + offsetY) * scale);
            auto r00 = getPsuedoRand(u, v, seedOffset);
            auto r01 = getPsuedoRand(u, v + 1, seedOffset);
            auto r10 = getPsuedoRand(u + 1, v, seedOffset);
            auto r11 = getPsuedoRand(u + 1, v + 1, seedOffset);

            ret[j * ImgSize + i] += amplification * (
                r00 * (1 - uFract) * (1 - vFract) +
                r01 * (1 - uFract) * vFract +
                r10 * uFract * (1 - vFract) +
                r11 * uFract * vFract
            );
        }
    }

    return ret;
}

float smoothFloor(float const x);
float smoothClamp(float const x, float const min, float const max);