$input v_color0 v_lightmapCoord v_lightNormal

#include <bgfx_shader.sh>
#include "rand.sh"

uniform sampler2D u_shadowmap;
uniform vec4 u_frame;

void main() {
    vec2 shadowSampleCoord = v_lightmapCoord.xy + vec2(
        rand2(vec2(v_lightmapCoord.x, u_frame.x)) - 0.5, 
        rand2(vec2(v_lightmapCoord.y, u_frame.x)) - 0.5
    ) * 0.001;

    vec4 shadowInfo = texture2D(u_shadowmap, shadowSampleCoord);

    float brightness = 0.9;
    if(shadowInfo.w < v_lightmapCoord.z - 0.01) {
        brightness = 0.6;
    }

    brightness += dot(v_lightNormal, vec3(0.0, -1.0, 0.0)) * 0.05 + 0.05;

    gl_FragColor = vec4(
        v_color0.r * shadowInfo.r, 
        v_color0.g * shadowInfo.g, 
        v_color0.b * shadowInfo.b, 
        1.0
    ) * brightness;

    //gl_FragColor = vec4(shadowInfo.w - v_lightmapCoord.z);
}
