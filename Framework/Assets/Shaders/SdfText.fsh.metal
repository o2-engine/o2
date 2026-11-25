// Distance field text (o2::SdfFont, o2::SdfTextStyle): the layers of the style composited front to back. A layer is a
// colour (solid or a gradient) over the union of up to two taps of the field: the glyph grown, blurred and offset.
// Distances are in pixels of the text, the edges are antialiased over a screen pixel at any scale. The vertices carry
// the atlas coordinates, the gradient coordinates (texCoords2), the field cell (texCoords3 and the normal's xy) and
// pixels of the text per field pixel (the normal's z)

struct O2SdfTextParams
{
    float4 u_field;      // layers count, distance range in field pixels, atlas width, atlas height
    float4 u_layers[36]; // per layer: colour 1, colour 2, (gradient, taps, edge scale, sharp corners), 2 taps, alphas
    float4 u_pass;       // the layer drawn by itself (the effects over the whole text: a pass per layer), -1 - all
};

inline float o2_median(float3 v)
{
    return max(min(v.r, v.g), min(max(v.r, v.g), v.b));
}

// Coverage of the field at the point: the glyph grown by the dilation, its edge a smoothstep of the half width
inline float o2_fieldCoverage(texture2d<float> field, sampler fieldSampler, float2 uv, float4 cell, float sharpCorners,
                              float range, float dilation, float halfWidth)
{
    float4 value = field.sample(fieldSampler, clamp(uv, cell.xy, cell.zw));
    float distance = ((sharpCorners > 0.5 ? o2_median(value.rgb) : value.a) - 0.5)*range + dilation;
    return smoothstep(-halfWidth, halfWidth, distance);
}

fragment float4 fragmentShader(O2RasterizerData input [[stage_in]],
                               texture2d<float> u_texture [[texture(0)]],
                               sampler textureSampler [[sampler(0)]],
                               constant O2SdfTextParams& params [[buffer(2)]])
{
    float2 atlasSize = params.u_field.zw;
    float pxPerTexel = input.normal.z;
    float4 cell = float4(input.texCoords3, input.normal.xy);

    float2 footprint = fwidth(input.texCoords)*atlasSize;
    float screenPx = max(0.5*(footprint.x + footprint.y), 0.0001)*pxPerTexel;
    float2 pxToUV = float2(1.0)/(atlasSize*pxPerTexel);
    float range = params.u_field.y*pxPerTexel;

    float4 result = float4(0.0);
    int layersCount = int(params.u_field.x + 0.5);
    int drawnLayer = params.u_pass.x < -0.5 ? -1 : int(params.u_pass.x + 0.5);
    for (int i = 0; i < 6 && i < layersCount; i++)
    {
        float4 color1 = params.u_layers[i*6];
        float4 color2 = params.u_layers[i*6 + 1];
        float4 info = params.u_layers[i*6 + 2];
        float4 alphas = params.u_layers[i*6 + 5];

        float uncovered = 1.0;
        int tapsCount = int(info.y + 0.5);
        for (int j = 0; j < 2 && j < tapsCount; j++)
        {
            float4 tap = params.u_layers[i*6 + 3 + j];
            float2 uv = input.texCoords + float2(-tap.x, tap.y)*pxToUV;
            float edge = 0.5*screenPx*info.z;
            float coverage;
            if (tap.w > 3.0*edge)
            {
                // A blur wider than the edge is sampled at four points of its kernel: a thin shape blurred is fainter
                // than its edge
                float2 spread = float2(0.322*tap.w)*pxToUV;
                float halfWidth = min(max(0.72*tap.w, edge), 0.5*range);
                float dilation = min(tap.z, 0.5*range - halfWidth);
                coverage = 0.25*(o2_fieldCoverage(u_texture, textureSampler, uv + float2(-spread.x, -spread.y), cell, info.w, range, dilation, halfWidth) +
                                 o2_fieldCoverage(u_texture, textureSampler, uv + float2(spread.x, -spread.y), cell, info.w, range, dilation, halfWidth) +
                                 o2_fieldCoverage(u_texture, textureSampler, uv + float2(-spread.x, spread.y), cell, info.w, range, dilation, halfWidth) +
                                 o2_fieldCoverage(u_texture, textureSampler, uv + float2(spread.x, spread.y), cell, info.w, range, dilation, halfWidth));
            }
            else
            {
                // The field keeps distances within the half range: a reach past it is cut there, not filled
                float halfWidth = min(max(tap.w, edge), 0.5*range);
                float dilation = min(tap.z, 0.5*range - halfWidth);
                coverage = o2_fieldCoverage(u_texture, textureSampler, uv, cell, info.w, range, dilation, halfWidth);
            }

            uncovered *= 1.0 - coverage*(j == 0 ? alphas.x : alphas.y);
        }

        float t = info.x < -0.5 ? 0.0 : (info.x < 0.5 ? input.texCoords2.x : input.texCoords2.y);
        float4 color = mix(color1, color2, clamp(t, 0.0, 1.0));
        float alpha = color.a*(1.0 - uncovered);

        // A pass of one layer: the layers of the letter in front of it are cut out of it
        if (drawnLayer >= 0 && i == drawnLayer)
            return float4(color.rgb, alpha*(1.0 - result.a))*input.color;

        result.rgb += (1.0 - result.a)*color.rgb*alpha;
        result.a += (1.0 - result.a)*alpha;
    }

    if (drawnLayer >= 0)
        return float4(0.0);

    float4 straight = result.a > 0.0 ? float4(result.rgb/result.a, result.a) : float4(0.0);
    return straight*input.color;
}
