$input a_position, a_normal, a_color0
$output v_color0 v_lightmapCoord

#include <bgfx_shader.sh>

uniform mat4 u_lightmapMtx;

void main() {
    gl_Position = mul(u_modelViewProj, vec4(a_position, 1.0));
    v_lightmapCoord = mul(u_lightmapMtx, vec4(a_position, 1.0)).xyz;
    v_lightmapCoord.x = v_lightmapCoord.x * 0.5 + 0.5;
    v_lightmapCoord.y = v_lightmapCoord.y * 0.5 + 0.5;

    v_color0 = a_color0;
}
