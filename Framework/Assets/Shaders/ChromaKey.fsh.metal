struct O2ChromaKeyParams
{
    float4 u_keyColor;
    float u_similarity;
    float u_smoothness;
    float u_spill;
    float u_choke;
};

fragment float4 fragmentShader(O2RasterizerData input [[stage_in]],
                               texture2d<float> u_texture [[texture(0)]],
                               sampler textureSampler [[sampler(0)]],
                               constant O2ChromaKeyParams& params [[buffer(2)]])
{
    float4 tex = u_texture.sample(textureSampler, input.texCoords);
    float3 k = params.u_keyColor.rgb;

    // Difference key against the known uniform backdrop: the matte is a wide ramp of the
    // weighted distance to the key, so alpha stays proportional to the subject fraction in
    // mixed edge pixels — the edge keeps its halftones. The key's dominant channel counts
    // double so dark subject tones don't read as backdrop
    float3 w = float3(1.0);
    if (k.g >= k.r && k.g >= k.b)
        w.g = 2.0;
    else if (k.r >= k.g && k.r >= k.b)
        w.r = 2.0;
    else
        w.b = 2.0;

    float dist = length((tex.rgb - k)*w);

    float alpha = smoothstep(params.u_similarity, params.u_similarity + max(params.u_smoothness, 0.0001), dist);
    alpha = clamp((alpha - params.u_choke)/max(1.0 - params.u_choke, 0.0001), 0.0, 1.0);

    // Screen subtraction: the key's share of the pixel goes to black and the subject share
    // is restored by exact unpremultiply — the edge stays semi-transparent with no key tint
    float3 fg = clamp(tex.rgb - (1.0 - alpha)*k, 0.0, 1.0);
    fg = clamp(fg/max(alpha, 0.02), 0.0, 1.0);

    // Despill: clamp the screen's dominant channel to the other two
    float spill = clamp(params.u_spill, 0.0, 1.0);
    if (k.g >= k.r && k.g >= k.b)
        fg.g = mix(fg.g, min(fg.g, max(fg.r, fg.b)), spill);
    else if (k.r >= k.g && k.r >= k.b)
        fg.r = mix(fg.r, min(fg.r, max(fg.g, fg.b)), spill);
    else
        fg.b = mix(fg.b, min(fg.b, max(fg.r, fg.g)), spill);

    return float4(fg, alpha)*input.color;
}
