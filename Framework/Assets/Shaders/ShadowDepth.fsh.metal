fragment float4 fragmentShader(O2RasterizerData input [[stage_in]],
                               texture2d<float> u_texture [[texture(0)]],
                               sampler textureSampler [[sampler(0)]])
{
    // Window-space z is the light clip-space depth in [0, 1]
    return float4(input.position.z, 0.0, 0.0, 1.0);
}
