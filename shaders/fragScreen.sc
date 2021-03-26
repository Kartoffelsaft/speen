$input v_texCoord v_color0

uniform sampler2D u_texture;

void main() {
    gl_FragColor = texture2D(u_texture, v_texCoord);
    gl_FragColor *= v_color0;
}
