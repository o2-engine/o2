vertex O2RasterizerData vertexShader(uint vertexID [[vertex_id]],
                                     constant O2SkinnedVertexIn* vertices [[buffer(0)]],
                                     constant O2Uniforms& uniforms [[buffer(1)]],
                                     constant O2SkinnedParams& params [[buffer(2)]])
{
    O2SkinnedVertexIn inputVertex = vertices[vertexID];

    float3 worldPos = o2_skinPoint(params.u_bones, float4(inputVertex.boneIndices),
                                   float4(inputVertex.boneWeights), float3(inputVertex.x, inputVertex.y, inputVertex.z));
    float3 worldNormal = o2_skinDirection(params.u_bones, float4(inputVertex.boneIndices),
                                          float4(inputVertex.boneWeights), float3(inputVertex.normal));
    worldNormal = normalize(worldNormal);

    float4 color = o2_unpackColor(inputVertex.color);
    if (params.u_shaded > 0.5)
    {
        // Same baked lambert as Mesh3DPrimitives::BakedLightDirection with 0.35 ambient
        const float3 lightDir = normalize(float3(0.3, -0.5, -0.8));
        float ambient = 0.35;
        float intensity = ambient + (1.0 - ambient)*max(dot(worldNormal, -lightDir), 0.0);
        color.rgb *= intensity;
    }

    O2RasterizerData output;
    output.position = uniforms.mvpMatrix * float4(worldPos, 1.0);
    output.color = color;
    output.texCoords = inputVertex.texCoord0;
    output.texCoords2 = inputVertex.texCoord0;
    output.texCoords3 = inputVertex.texCoord0;
    output.normal = worldNormal;
    return output;
}
