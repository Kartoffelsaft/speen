$input v_color0 v_lightMapCoord v_lightNormal

#include <bgfx_shader.sh>
#include "rand.sh"

uniform sampler2D u_shadowMap;
uniform vec4 u_frame;

void main() {
/*
    vec2 shadowSampleCoord = v_lightMapCoord.xy + vec2(
        rand2(vec2(rand(v_lightMapCoord.x) * v_lightMapCoord.y, u_frame.x * 0.34)) - 0.5, 
        rand2(vec2(rand(v_lightMapCoord.y) * v_lightMapCoord.x, u_frame.x * 0.96)) - 0.5
    ) * 0.0003;
*/
    vec2 shadowSampleCoord = v_lightMapCoord.xy + vec2(
        rand2(vec2(rand(gl_FragCoord.x * 0.3), gl_FragCoord.y * 0.5)) - 0.5,
        rand2(vec2(rand(gl_FragCoord.y * 0.4), gl_FragCoord.x * 0.1)) - 0.5
    ) * 0.0014;

    vec4 shadowInfo = texture2D(u_shadowMap, shadowSampleCoord);

    float brightness = 0.9;
    if(shadowInfo.w < v_lightMapCoord.z - 0.005) {
        brightness = 0.6;
    }

    brightness += dot(v_lightNormal, vec3(0.0, 1.0, 0.0)) * 0.05 + 0.05;

    gl_FragColor = vec4(
        v_color0.r * shadowInfo.r * brightness, 
        v_color0.g * shadowInfo.g * brightness, 
        v_color0.b * shadowInfo.b * brightness, 
        1.0
    );

    //gl_FragColor = vec4(shadowInfo.w - v_lightMapCoord.z + 0.5);
}
