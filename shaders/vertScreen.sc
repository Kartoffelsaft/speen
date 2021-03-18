$input a_position
$output v_texCoord

void main() {
    gl_Position = vec4(a_position.xy * 2.0 - vec2(1.0, 1.0) , 0.5, 1.0);
    v_texCoord = a_position.xy;
}
