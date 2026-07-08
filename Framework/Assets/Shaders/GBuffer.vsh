uniform mat4 u_transformMatrix;

attribute vec4 a_position;
attribute vec4 a_color;
attribute vec2 a_texCoords;
attribute vec3 a_normal;

varying vec4 v_color;
varying vec2 v_texCoords;
varying vec3 v_normal;
varying vec3 v_worldPos;

void main()
{
    v_color = a_color;
    v_texCoords = a_texCoords;
    v_normal = a_normal;
    v_worldPos = a_position.xyz;
    gl_Position = u_transformMatrix * a_position;
}
