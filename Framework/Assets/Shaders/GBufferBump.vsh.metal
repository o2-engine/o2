vertex O2RasterizerData vertexShader(uint vertexID [[vertex_id]],
                                     constant O2VertexIn* vertices [[buffer(0)]],
                                     constant O2Uniforms& uniforms [[buffer(1)]])
{
    O2VertexIn inputVertex = vertices[vertexID];

    O2RasterizerData output;
    output.position = uniforms.mvpMatrix * float4(inputVertex.x, inputVertex.y, inputVertex.z, 1.0);
    output.color = o2_unpackColor(inputVertex.color);
    output.texCoords = inputVertex.texCoord0;
    output.texCoords2 = float2(inputVertex.x, inputVertex.y); // world position xy (vertices are pre-transformed)
    output.texCoords3 = float2(inputVertex.z, 0.0);           // world position z
    output.normal = inputVertex.normal;
    return output;
}
