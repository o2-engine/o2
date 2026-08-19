vertex O2RasterizerData vertexShader(uint vertexID [[vertex_id]],
                                     constant O2VertexIn* vertices [[buffer(0)]],
                                     constant O2Uniforms& uniforms [[buffer(1)]])
{
    O2VertexIn inputVertex = vertices[vertexID];

    O2RasterizerData output;
    output.position = uniforms.mvpMatrix * float4(inputVertex.x, inputVertex.y, inputVertex.z, 1.0);
    output.color = o2_unpackColor(inputVertex.color);
    output.texCoords = inputVertex.texCoord0;
    output.texCoords2 = inputVertex.texCoord1;
    output.texCoords3 = inputVertex.texCoord2;
    output.normal = inputVertex.normal;
    return output;
}
