uniform mat4 u_transformMatrix;
uniform vec4 u_bones[192]; // Bones palette: three transposed rows (float4x3) per bone, 64 bones max
uniform float u_shaded;

attribute vec4 a_position;
attribute vec4 a_color;
attribute vec2 a_texCoords;
attribute vec3 a_normal;
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
    vec3 worldNormal = normalize(skinDirection(a_normal));

    vec4 color = a_color;
    if (u_shaded > 0.5)
    {
        // Same baked lambert as Mesh3DPrimitives::BakedLightDirection with 0.35 ambient
        vec3 lightDir = normalize(vec3(0.3, -0.5, -0.8));
        float ambient = 0.35;
        float intensity = ambient + (1.0 - ambient)*max(dot(worldNormal, -lightDir), 0.0);
        color.rgb *= intensity;
    }

    v_color = color;
    v_texCoords = a_texCoords;
    gl_Position = u_transformMatrix * vec4(worldPos, 1.0);
}
