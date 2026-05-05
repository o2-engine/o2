fragment float4 fragmentShader(O2RasterizerData input [[stage_in]],
                               texture2d<float> u_texture [[texture(0)]],
                               sampler textureSampler [[sampler(0)]])
{
    return input.color * u_texture.sample(textureSampler, input.texCoords);
}
