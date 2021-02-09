$input v_color0 v_lightmapCoord

#include <bgfx_shader.sh>

uniform sampler2D u_shadowmap;

void main() {
    vec4 shadowInfo = texture2D(u_shadowmap, v_lightmapCoord.xy);

    float brightness = 1.0;
    if(shadowInfo.w < v_lightmapCoord.z - 0.01) {
        brightness = 0.6;
    }

    gl_FragColor = vec4(
        v_color0.r * shadowInfo.r, 
        v_color0.g * shadowInfo.g, 
        v_color0.b * shadowInfo.b, 
        1.0
    ) * brightness;

    //gl_FragColor = vec4(shadowInfo.w - v_lightmapCoord.z);
}
