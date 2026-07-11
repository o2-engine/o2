precision highp float;

varying vec4 v_color;
varying vec2 v_texCoords;

void main()
{
    // Window-space z is the light clip-space depth in [0, 1]
    gl_FragColor = vec4(gl_FragCoord.z, 0.0, 0.0, 1.0);
}
