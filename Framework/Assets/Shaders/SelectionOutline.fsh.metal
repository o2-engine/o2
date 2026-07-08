struct O2SelectionOutlineParams
{
    float2 u_texelSize;
};

fragment float4 fragmentShader(O2RasterizerData input [[stage_in]],
                               texture2d<float> u_texture [[texture(0)]],
                               sampler maskSampler [[sampler(0)]],
                               constant O2SelectionOutlineParams& params [[buffer(2)]])
{
    // Silhouette border: the mask is empty here, but covered nearby (1-2 texels)
    float center = u_texture.sample(maskSampler, input.texCoords).a;
    if (center > 0.5)
        return float4(0.0, 0.0, 0.0, 0.0);

    float neighbors = 0.0;
    for (int y = -1; y <= 1; y++)
    {
        for (int x = -1; x <= 1; x++)
        {
            float2 offset = float2(x, y)*params.u_texelSize;
            neighbors = max(neighbors, u_texture.sample(maskSampler, input.texCoords + offset).a);
            neighbors = max(neighbors, u_texture.sample(maskSampler, input.texCoords + offset*2.0).a);
        }
    }

    if (neighbors > 0.5)
        return input.color;

    return float4(0.0, 0.0, 0.0, 0.0);
}
