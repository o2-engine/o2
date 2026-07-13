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

    O2RasterizerData output;
    output.position = uniforms.mvpMatrix * float4(worldPos, 1.0);
    output.color = o2_unpackColor(inputVertex.color);
    output.texCoords = inputVertex.texCoord0;
    output.texCoords2 = float2(worldPos.x, worldPos.y); // world position xy
    output.texCoords3 = float2(worldPos.z, 0.0);        // world position z
    output.normal = normalize(worldNormal);
    return output;
}
