#pragma once

#include <array>
#include <vector>
#include <tuple>
#include <random>
#include <algorithm>
#include <cmath>
#include <cfenv>
#include <bx/math.h>

using Mat4 = std::array<float, 16>;

struct Vec3 {
    float x;
    float y;
    float z;

    Vec3() = default;
    constexpr Vec3(float nx, float ny, float nz): x{nx}, y{ny}, z{nz} {}
    constexpr Vec3(bx::Vec3 const & bv): x{bv.x}, y{bv.y}, z{bv.z} {}

    using bxv3 = bx::Vec3;
    operator bxv3() const { return {x, y, z}; }
    constexpr Vec3 operator +(Vec3 const & other) const {
        return {this->x + other.x, this->y + other.y, this->z + other.z};
    }
    Vec3& operator +=(Vec3 const & other) {
        *this = *this + other;
        return *this;
    }
    constexpr Vec3 operator -() const {
        return {-x, -y, -z};
    }
    constexpr Vec3 operator -(Vec3 const & other) const {
        return (*this) + (-other);
    }
    Vec3& operator -=(Vec3 const & other) {
        *this = *this - other;
        return *this;
    }
    constexpr Vec3 operator *(float const & m) const {
        return {x * m, y * m, z * m};
    }
    Vec3 operator *=(float const & m) {
        *this = *this * m;
        return *this;
    }
    constexpr Vec3 operator /(float const & m) const {
        return {x / m, y / m, z / m};
    }
    Vec3 operator /=(float const & m) {
        *this = *this / m;
        return *this;
    }

    constexpr float dot(Vec3 const & other) const {
        return this->x * other.x 
            + this->y * other.y 
            + this->z * other.z;
    }
    constexpr Vec3 cross(Vec3 const & other) const {
        return Vec3{
            this->y * other.z - this->z * other.y,
            this->z * other.x - this->x * other.z,
            this->x * other.y - this->y * other.x
        };
    }

    constexpr float length() const {
        return std::sqrt(this->lengthSquared());
    }
    constexpr float lengthSquared() const {
        return x * x + y * y + z * z;
    }
    constexpr Vec3 normalized() const {
        return (*this) / this->length();
    }
};

inline Vec3 operator *(Mat4 const & mtx, Vec3 const & vec) {
    return bx::mul(vec, mtx.data());
}

inline Mat4 operator *(Mat4 const & mtxA, Mat4 const & mtxB) {
    Mat4 ret;
    bx::mtxMul(ret.data(), mtxB.data(), mtxA.data());
    return ret;
}

template<typename T>
struct RGB {
    T r;
    T g;
    T b;

    void appendToVector(std::vector<T>& vec) {
        vec.push_back(r);
        vec.push_back(g);
        vec.push_back(b);
    }
};
    
/**
 * @brief returns an integer and a fractional component of the input number, towards -inf.
 * 
 * @param x 
 * @return std::tuple<int, float> A tuple of the integer and fractional component
 */
std::tuple<int, float> floorFract(float const x);

/**
 * @brief Interpolates between the corners of a square.
 * 
 * @param s00 top left
 * @param s01 bottom left
 * @param s10 top right
 * @param s11 bottom right
 * @param x Horizontal component where 0.0 is left and 1.0 is right
 * @param y Vertical component where 0.0 is top and 1.0 is bottom
 * @return float 
 */
float interpolate(
    float const s00, 
    float const s01, 
    float const s10, 
    float const s11, 
    float const x, 
    float const y
);

/**
 * @brief Applies a convolutional kernel on a square array.
 * Final image will be shrunk to deal with edges.
 * 
 * see:
 * https://en.wikipedia.org/wiki/Kernel_(image_processing)
 * 
 * @tparam ImgSize The input image size
 * @tparam ConvSize The kernel size
 * @param image The input image
 * @param kernel The kernel
 * @param scale How much to multiply the output by (usually to keep the return values between 0.0 and 1.0)
 * @return std::array<float, (ImgSize - ConvSize + 1) * (ImgSize - ConvSize + 1)> 
 */
template<std::size_t ImgSize, std::size_t ConvSize>
std::array<float, (ImgSize - ConvSize + 1) * (ImgSize - ConvSize + 1)> convolute(
    std::array<float, ImgSize * ImgSize> const image,
    std::array<float, ConvSize * ConvSize> const kernel,
    float const scale = 1.f
) {
    auto const outSize = ImgSize - ConvSize + 1;
    std::array<float, outSize * outSize> ret;

    for(std::size_t i = 0; i < outSize; i++) for(std::size_t j = 0; j < outSize; j++) {
        ret[i * outSize + j] = [=](){
            float sum = 0.f;
            for(std::size_t ki = 0; ki < ConvSize; ki++) for(std::size_t kj = 0; kj < ConvSize; kj++) {
                sum += image[(i + ki) * ImgSize + j + kj] * kernel[ki * ConvSize + kj];
            }
            return sum;
        }() * scale;
    }

    return ret;
}

/**
 * @brief generates a float between 0 and 1 based off of the coord and seed you give it
 * 
 * @param x 
 * @param y 
 * @param seed 
 * @return float 
 */
inline float randFromCoord(int x, int y, std::size_t seed) {
    return std::hash<std::size_t>()(
        std::hash<double>()(std::tan(x * x * y * 137.54 + 0.3))
      ^ std::hash<double>()(std::atanh(y * y * x * 805.87 + 0.84))
      ^ std::hash<double>()(1 / std::sin(x * seed * y * 8.8 + 0.1))
    ) / (double) std::numeric_limits<std::size_t>::max();
}

/**
 * @brief Generates a square of noise
 * 
 * @tparam ImgSize how big the output image will be
 * @param seed The seed for the random number generator
 * @param offsetX Horizontal offset of the noise generator
 * @param offsetY Vertical offset of the noise generator
 * @param resolutions A vector of all the octaves of the noise generator
 *        Each element contains (in this order):
 *            Scale: A number in the range of (0.0, 1.0] describing how high frequency the noise is 
 *                (1.0 is effectively white noise and 0.01 is very smooth)
 *            Amplification: How much weight is given to this octave
 *            Seed offset: An additional seed to generate something different than other octaves
 * @return std::array<float, ImgSize * ImgSize> 
 */
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

    for(auto const & [scale, amplification, seedOffset]: resolutions) {
        for(int64_t i = 0; i < (int64_t)ImgSize; i++) for(int64_t j = 0; j < (int64_t)ImgSize; j++) {
            auto [u, uFract] = floorFract((i + offsetX) * scale);
            auto [v, vFract] = floorFract((j + offsetY) * scale);
            auto r00 = randFromCoord(u    , v    , seed ^ seedOffset);
            auto r01 = randFromCoord(u    , v + 1, seed ^ seedOffset);
            auto r10 = randFromCoord(u + 1, v    , seed ^ seedOffset);
            auto r11 = randFromCoord(u + 1, v + 1, seed ^ seedOffset);

            ret[j * ImgSize + i] += amplification * interpolate(r00, r01, r10, r11, uFract, vFract);
        }
    }

    return ret;
}
