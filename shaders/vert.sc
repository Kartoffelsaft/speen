$input a_position, a_normal, a_color0
$output v_color0 v_lightmapCoord v_lightNormal

#include <bgfx_shader.sh>

uniform mat4 u_lightmapMtx;
uniform mat4 u_lightDirMtx;
uniform mat4 u_modelMtx;

void main() {
    gl_Position = mul(u_modelViewProj, vec4(a_position, 1.0));
    v_lightmapCoord = mul(u_lightmapMtx, mul(u_modelMtx, vec4(a_position, 1.0))).xyz;
    v_lightmapCoord.x = v_lightmapCoord.x * 0.5 + 0.5;
    v_lightmapCoord.y = v_lightmapCoord.y * 0.5 + 0.5;

    v_lightNormal = mul(u_lightDirMtx, mul(u_modelMtx, vec4(a_normal, 0.0))).xyz;
    v_color0 = a_color0;
}
