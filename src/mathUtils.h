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
    std::vector<float> const & resolutions
) {
    std::array<float, ImgSize * ImgSize> ret;
    ret.fill(0.f);

    auto getPsuedoRand = [=](int x, int y){
        static std::mt19937_64 rng; // NOLINT(cert-msc51-cpp)
        static std::uniform_real_distribution<> distribution(0.0, 1.0);
        rng.seed(
            std::hash<double>()(std::tan(x + 0.3))
          ^ std::hash<double>()(std::sinh(y))
          ^ seed
        );
        return distribution(rng);
    };

    for(auto const & res: resolutions) {
        for(int i = 0; i < ImgSize; i++) for(int j = 0; j < ImgSize; j++) {
            auto [u, uFract] = floorFract((i + offsetX) * res);
            auto [v, vFract] = floorFract((j + offsetY) * res);
            auto r00 = getPsuedoRand(u, v);
            auto r01 = getPsuedoRand(u, v + 1);
            auto r10 = getPsuedoRand(u + 1, v);
            auto r11 = getPsuedoRand(u + 1, v + 1);

            ret[j * ImgSize + i] = (
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