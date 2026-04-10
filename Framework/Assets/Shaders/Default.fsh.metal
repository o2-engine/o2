fragment float4 fragmentShader(O2RasterizerData input [[stage_in]],
                               texture2d<float> u_texture [[texture(0)]])
{
    constexpr sampler textureSampler(mag_filter::linear, min_filter::linear);
    return input.color * u_texture.sample(textureSampler, input.texCoords);
}