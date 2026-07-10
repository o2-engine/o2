// GL backends don't support GPU skinning and the shadow path yet
uniform mat4 u_transformMatrix;

attribute vec4 a_position;
attribute vec4 a_color;
attribute vec2 a_texCoords;

varying vec4 v_color;
varying vec2 v_texCoords;

void main()
{
    v_color = a_color;
    v_texCoords = a_texCoords;
    gl_Position = u_transformMatrix * a_position;
}
