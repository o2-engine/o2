varying vec4 v_color;
varying vec2 v_texCoords;

void main()
{
    gl_FragColor = vec4(gl_FragCoord.z, 0.0, 0.0, 1.0);
}
