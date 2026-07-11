uniform mat4 u_transformMatrix;
uniform vec4 u_bones[192]; // Bones palette: three transposed rows (float4x3) per bone, 64 bones max

attribute vec4 a_position;
attribute vec4 a_color;
attribute vec2 a_texCoords;
attribute vec4 a_boneIndices;
attribute vec4 a_boneWeights;

varying vec4 v_color;
varying vec2 v_texCoords;

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

void main()
{
    v_color = a_color;
    v_texCoords = a_texCoords;
    gl_Position = u_transformMatrix * vec4(skinPoint(a_position.xyz), 1.0);
}
