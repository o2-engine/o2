// GL backends stub: outputs nothing (the editor selection outline is Metal-only for now)
varying vec4 v_color;
varying vec2 v_texCoords;

uniform sampler2D u_texture;

void main()
{
    gl_FragColor = vec4(0.0, 0.0, 0.0, 0.0);
}
