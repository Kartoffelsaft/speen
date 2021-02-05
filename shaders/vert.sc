$input a_position, a_normal, a_color0
$output v_color0

#include <bgfx_shader.sh>

void main() {
    gl_Position = mul(u_modelViewProj, vec4(a_position, 1.0));

    vec3 rotatedNormal = mul(u_modelViewProj, vec4(a_normal, 0.0)).xyz;

    v_color0 = a_color0 * (rotatedNormal.y - rotatedNormal.x + 1.4) * 0.3;
}
