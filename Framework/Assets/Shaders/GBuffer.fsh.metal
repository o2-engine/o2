struct O2GBufferOut
{
    float4 albedo [[color(0)]];
    float4 normal [[color(1)]];
    float4 position [[color(2)]];
};

fragment O2GBufferOut fragmentShader(O2RasterizerData input [[stage_in]],
                                     texture2d<float> u_texture [[texture(0)]],
                                     sampler textureSampler [[sampler(0)]])
{
    O2GBufferOut output;
    output.albedo = input.color * u_texture.sample(textureSampler, input.texCoords);
    output.normal = float4(normalize(input.normal), 1.0);
    output.position = float4(input.texCoords2.x, input.texCoords2.y, input.texCoords3.x, 1.0);
    return output;
}
