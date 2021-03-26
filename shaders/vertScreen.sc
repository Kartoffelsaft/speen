$input a_position a_texcoord0 a_color0
$output v_texCoord v_color0

#include <bgfx_shader.sh>

void main() {
    gl_Position = mul(u_modelViewProj, vec4(a_position, 1.0));
    v_texCoord = a_texcoord0;
    if((a_color0.r > 0.0)
    || (a_color0.g > 0.0)
    || (a_color0.b > 0.0)
    ) {
        v_color0 = a_color0;
    } else {
        v_color0 = vec4(1.0, 1.0, 1.0, a_color0.a);
    }
}
