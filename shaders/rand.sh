float rand(float seed) {
    return fract(sin(seed * 12.98) * 43758.5453);
}

float rand2(vec2 seed) {
    return fract(sin(dot(seed, vec2(12.9898, 78.233))) * 43758.5453);
}
