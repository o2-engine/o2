#version 300 es

uniform mat4 u_transformMatrix;

in vec4 a_position;
in vec4 a_color;
in vec2 a_texCoords;
in vec3 a_normal;

out vec4 v_color;
out vec2 v_texCoords;
out vec3 v_normal;
out vec3 v_worldPos;

void main()
{
    v_color = a_color;
    v_texCoords = a_texCoords;
    v_normal = a_normal;
    v_worldPos = a_position.xyz; // Vertices are pre-transformed to world space
    gl_Position = u_transformMatrix * a_position;
}
