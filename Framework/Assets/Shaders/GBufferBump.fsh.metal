struct O2GBufferOut
{
    float4 albedo [[color(0)]];
    float4 normal [[color(1)]];
    float4 position [[color(2)]];
};

// G-buffer variant with normal mapping: meshes carry no vertex tangents, the tangent
// basis is reconstructed per pixel as a screen-space cotangent frame from derivatives
fragment O2GBufferOut fragmentShader(O2RasterizerData input [[stage_in]],
                                     texture2d<float> u_texture [[texture(0)]],
                                     sampler textureSampler [[sampler(0)]],
                                     texture2d<float> u_normalMap [[texture(1)]],
                                     sampler normalMapSampler [[sampler(1)]])
{
    float3 worldPos = float3(input.texCoords2.x, input.texCoords2.y, input.texCoords3.x);
    float2 uv = input.texCoords;
    float3 baseNormal = normalize(input.normal);

    float3 dp1 = dfdx(worldPos);
    float3 dp2 = dfdy(worldPos);
    float2 duv1 = dfdx(uv);
    float2 duv2 = dfdy(uv);

    float3 dp2perp = cross(dp2, baseNormal);
    float3 dp1perp = cross(baseNormal, dp1);
    float3 tangent = dp2perp*duv1.x + dp1perp*duv2.x;
    float3 bitangent = dp2perp*duv1.y + dp1perp*duv2.y;

    float3 bumpedNormal = baseNormal;
    float maxLength = max(dot(tangent, tangent), dot(bitangent, bitangent));
    if (maxLength > 1.0e-12)
    {
        float invMax = rsqrt(maxLength);
        float3 mapNormal = u_normalMap.sample(normalMapSampler, uv).xyz*2.0 - 1.0;
        bumpedNormal = normalize(mapNormal.x*(tangent*invMax) + mapNormal.y*(bitangent*invMax) +
                                 mapNormal.z*baseNormal);
    }

    O2GBufferOut output;
    output.albedo = input.color * u_texture.sample(textureSampler, input.texCoords);
    output.normal = float4(bumpedNormal, 1.0);
    output.position = float4(worldPos, 1.0);
    return output;
}
