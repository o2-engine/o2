struct O2DeferredLightingParams
{
    float u_lightsCount;
    float u_ambient;
    float u_shadowsEnabled;
    float u_shadowLightIndex;
    float u_shadowMapSize;
    float4x4 u_lightVP;       // Light view-projection for shadow depth reconstruction
    float4 u_lightColors[8];  // rgb: color*intensity, w: 0 - directional, 1 - point
    float4 u_lightVectors[8]; // directional: xyz - direction to light; point: xyz - position, w - range
};

static float o2_shadowFactor(constant O2DeferredLightingParams& params,
                             texture2d<float> shadowMap, sampler shadowMapSampler,
                             float3 worldPosition, float ndl)
{
    float4 lightClip = params.u_lightVP * float4(worldPosition, 1.0);
    float3 lightNdc = lightClip.xyz / max(lightClip.w, 0.0001);
    float2 shadowUV = float2(lightNdc.x*0.5 + 0.5, 0.5 - lightNdc.y*0.5);
    float depth = lightNdc.z;

    if (shadowUV.x < 0.0 || shadowUV.x > 1.0 || shadowUV.y < 0.0 || shadowUV.y > 1.0 || depth > 1.0)
        return 1.0;

    float bias = max(0.004*(1.0 - ndl), 0.0015);
    float texel = 1.0/max(params.u_shadowMapSize, 1.0);

    // PCF 2x2
    float lit = 0.0;
    for (int sy = 0; sy < 2; sy++)
    {
        for (int sx = 0; sx < 2; sx++)
        {
            float mapDepth = shadowMap.sample(shadowMapSampler, shadowUV + float2(sx, sy)*texel).r;
            lit += depth - bias > mapDepth ? 0.0 : 1.0;
        }
    }

    return lit*0.25;
}

fragment float4 fragmentShader(O2RasterizerData input [[stage_in]],
                               texture2d<float> u_texture [[texture(0)]],
                               sampler albedoSampler [[sampler(0)]],
                               texture2d<float> u_normalsTex [[texture(1)]],
                               sampler normalsSampler [[sampler(1)]],
                               texture2d<float> u_positionsTex [[texture(2)]],
                               sampler positionsSampler [[sampler(2)]],
                               texture2d<float> u_shadowMap [[texture(3)]],
                               sampler shadowMapSampler [[sampler(3)]],
                               constant O2DeferredLightingParams& params [[buffer(2)]])
{
    float4 albedo = u_texture.sample(albedoSampler, input.texCoords);
    float4 normalSample = u_normalsTex.sample(normalsSampler, input.texCoords);

    // Background pixels have cleared alpha and no valid normal: output transparent,
    // keeping the main target content (cleared with camera fill color, or editor grid)
    if (normalSample.a < 0.5)
        return float4(0.0, 0.0, 0.0, 0.0);

    float3 normal = normalize(normalSample.xyz);
    float3 worldPosition = u_positionsTex.sample(positionsSampler, input.texCoords).xyz;

    float3 lighting = float3(params.u_ambient);
    int lightsCount = int(params.u_lightsCount + 0.5);
    int shadowLightIndex = int(params.u_shadowLightIndex + 0.5);
    for (int i = 0; i < lightsCount && i < 8; i++)
    {
        float4 lightColor = params.u_lightColors[i];
        float4 lightVector = params.u_lightVectors[i];

        float3 toLight;
        float attenuation = 1.0;
        if (lightColor.w > 0.5)
        {
            float3 delta = lightVector.xyz - worldPosition;
            float dist = max(length(delta), 0.0001);
            toLight = delta/dist;
            attenuation = clamp(1.0 - dist/max(lightVector.w, 0.0001), 0.0, 1.0);
        }
        else
            toLight = normalize(lightVector.xyz);

        float ndl = max(dot(normal, toLight), 0.0);

        if (params.u_shadowsEnabled > 0.5 && i == shadowLightIndex)
            attenuation *= o2_shadowFactor(params, u_shadowMap, shadowMapSampler, worldPosition, ndl);

        lighting += lightColor.rgb*attenuation*ndl;
    }

    return float4(albedo.rgb*lighting, 1.0);
}
