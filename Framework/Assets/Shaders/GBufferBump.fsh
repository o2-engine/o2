#version 300 es
precision highp float;

in vec4 v_color;
in vec2 v_texCoords;
in vec3 v_normal;
in vec3 v_worldPos;

uniform sampler2D u_texture;
uniform sampler2D u_normalMap;

layout(location = 0) out vec4 o_albedo;
layout(location = 1) out vec4 o_normal;
layout(location = 2) out vec4 o_position;

// G-buffer variant with normal mapping: meshes carry no vertex tangents, the tangent
// basis is reconstructed per pixel as a screen-space cotangent frame from derivatives
void main()
{
    vec3 baseNormal = normalize(v_normal);

    vec3 dp1 = dFdx(v_worldPos);
    vec3 dp2 = dFdy(v_worldPos);
    vec2 duv1 = dFdx(v_texCoords);
    vec2 duv2 = dFdy(v_texCoords);

    vec3 dp2perp = cross(dp2, baseNormal);
    vec3 dp1perp = cross(baseNormal, dp1);
    vec3 tangent = dp2perp*duv1.x + dp1perp*duv2.x;
    vec3 bitangent = dp2perp*duv1.y + dp1perp*duv2.y;

    vec3 bumpedNormal = baseNormal;
    float maxLength = max(dot(tangent, tangent), dot(bitangent, bitangent));
    if (maxLength > 1.0e-12)
    {
        float invMax = inversesqrt(maxLength);
        vec3 mapNormal = texture(u_normalMap, v_texCoords).xyz*2.0 - 1.0;
        bumpedNormal = normalize(mapNormal.x*(tangent*invMax) + mapNormal.y*(bitangent*invMax) +
                                 mapNormal.z*baseNormal);
    }

    o_albedo = v_color * texture(u_texture, v_texCoords);
    o_normal = vec4(bumpedNormal, 1.0);
    o_position = vec4(v_worldPos, 1.0);
}
