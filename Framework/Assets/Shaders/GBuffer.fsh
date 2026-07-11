#version 300 es
precision highp float;

in vec4 v_color;
in vec2 v_texCoords;
in vec3 v_normal;
in vec3 v_worldPos;

uniform sampler2D u_texture;

layout(location = 0) out vec4 o_albedo;
layout(location = 1) out vec4 o_normal;
layout(location = 2) out vec4 o_position;

void main()
{
    o_albedo = v_color * texture(u_texture, v_texCoords);
    o_normal = vec4(normalize(v_normal), 1.0);
    o_position = vec4(v_worldPos, 1.0);
}
