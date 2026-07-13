#version 300 es

uniform mat4 u_transformMatrix;
uniform vec4 u_bones[192]; // Bones palette: three transposed rows (float4x3) per bone, 64 bones max

in vec4 a_position;
in vec4 a_color;
in vec2 a_texCoords;
in vec3 a_normal;
in vec4 a_boneIndices;
in vec4 a_boneWeights;

out vec4 v_color;
out vec2 v_texCoords;
out vec3 v_normal;
out vec3 v_worldPos;

vec3 skinPoint(vec3 p)
{
    vec4 point4 = vec4(p, 1.0);
    vec3 result = vec3(0.0);
    for (int i = 0; i < 4; i++)
    {
        int base = int(a_boneIndices[i])*3;
        result += vec3(dot(u_bones[base], point4), dot(u_bones[base + 1], point4),
                       dot(u_bones[base + 2], point4))*a_boneWeights[i];
    }
    return result;
}

vec3 skinDirection(vec3 d)
{
    vec3 result = vec3(0.0);
    for (int i = 0; i < 4; i++)
    {
        int base = int(a_boneIndices[i])*3;
        result += vec3(dot(u_bones[base].xyz, d), dot(u_bones[base + 1].xyz, d),
                       dot(u_bones[base + 2].xyz, d))*a_boneWeights[i];
    }
    return result;
}

void main()
{
    vec3 worldPos = skinPoint(a_position.xyz);

    v_color = a_color;
    v_texCoords = a_texCoords;
    v_normal = normalize(skinDirection(a_normal));
    v_worldPos = worldPos;
    gl_Position = u_transformMatrix * vec4(worldPos, 1.0);
}
