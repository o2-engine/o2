vertex O2RasterizerData vertexShader(uint vertexID [[vertex_id]],
                                     constant O2SkinnedVertexIn* vertices [[buffer(0)]],
                                     constant O2Uniforms& uniforms [[buffer(1)]],
                                     constant O2SkinnedParams& params [[buffer(2)]])
{
    O2SkinnedVertexIn inputVertex = vertices[vertexID];

    float3 worldPos = o2_skinPoint(params.u_bones, float4(inputVertex.boneIndices),
                                   float4(inputVertex.boneWeights), float3(inputVertex.x, inputVertex.y, inputVertex.z));

    O2RasterizerData output;
    output.position = uniforms.mvpMatrix * float4(worldPos, 1.0);
    output.color = o2_unpackColor(inputVertex.color);
    output.texCoords = inputVertex.texCoord0;
    output.texCoords2 = inputVertex.texCoord0;
    output.texCoords3 = inputVertex.texCoord0;
    output.normal = float3(inputVertex.normal);
    return output;
}
