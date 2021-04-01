#include "mathUtils.h"

float Vec3::dot(Vec3 const & other) const {
    return this->x * other.x 
         + this->y * other.y 
         + this->z * other.z;
}

Vec3 Vec3::cross(Vec3 const & other) const {
    return Vec3{
        this->y * other.z - this->z * other.y,
        this->z * other.x - this->x * other.z,
        this->x * other.y - this->y * other.x
    };
}

float Vec3::length() const {
    return std::sqrt(this->lengthSquared());
}

float Vec3::lengthSquared() const {
    return x * x + y * y + z * z;
}

Vec3 Vec3::normalized() const {
    return (*this) / this->length();
}

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
