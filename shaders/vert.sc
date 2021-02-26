$input a_position, a_normal, a_color0
$output v_color0 v_lightMapCoord v_lightNormal

#include <bgfx_shader.sh>

uniform mat4 u_lightMapMtx;
uniform mat4 u_lightDirMtx;
uniform mat4 u_modelMtx;

void main() {
    gl_Position = mul(u_modelViewProj, vec4(a_position, 1.0));
    v_lightMapCoord = mul(u_lightMapMtx, mul(u_modelMtx, vec4(a_position, 1.0))).xyz;
    v_lightMapCoord.x = v_lightMapCoord.x * 0.5 + 0.5;
    v_lightMapCoord.y = v_lightMapCoord.y * 0.5 + 0.5;

    v_lightNormal = mul(u_lightDirMtx, mul(u_modelMtx, vec4(a_normal, 0.0))).xyz;
    v_color0 = a_color0;
}
