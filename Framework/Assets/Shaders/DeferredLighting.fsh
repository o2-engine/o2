precision highp float;

varying vec4 v_color;
varying vec2 v_texCoords;

uniform sampler2D u_texture;      // G-buffer albedo
uniform sampler2D u_normalsTex;   // G-buffer world normals
uniform sampler2D u_positionsTex; // G-buffer world positions
uniform sampler2D u_shadowMap;    // Light-space depth in R channel

uniform float u_lightsCount;
uniform float u_ambient;
uniform float u_shadowsEnabled;
uniform float u_shadowLightIndex;
uniform float u_shadowMapSize;
uniform mat4  u_lightVP;         // Light view-projection for shadow depth reconstruction
uniform vec4  u_lightColors[8];  // rgb: color*intensity, w: 0 - directional, 1 - point
uniform vec4  u_lightVectors[8]; // directional: xyz - direction to light; point: xyz - position, w - range

float shadowFactor(vec3 worldPosition, float ndl)
{
    vec4 lightClip = u_lightVP * vec4(worldPosition, 1.0);
    vec3 lightNdc = lightClip.xyz / max(lightClip.w, 0.0001);
    vec2 shadowUV = vec2(lightNdc.x*0.5 + 0.5, 0.5 - lightNdc.y*0.5);
    float depth = lightNdc.z;

    if (shadowUV.x < 0.0 || shadowUV.x > 1.0 || shadowUV.y < 0.0 || shadowUV.y > 1.0 || depth > 1.0)
        return 1.0;

    float bias = max(0.004*(1.0 - ndl), 0.0015);
    float texel = 1.0/max(u_shadowMapSize, 1.0);

    // PCF 2x2
    float lit = 0.0;
    for (int sy = 0; sy < 2; sy++)
    {
        for (int sx = 0; sx < 2; sx++)
        {
            float mapDepth = texture2D(u_shadowMap, shadowUV + vec2(float(sx), float(sy))*texel).r;
            lit += depth - bias > mapDepth ? 0.0 : 1.0;
        }
    }

    return lit*0.25;
}

void main()
{
    vec4 albedo = texture2D(u_texture, v_texCoords);
    vec4 normalSample = texture2D(u_normalsTex, v_texCoords);

    // Background pixels have cleared alpha and no valid normal: output transparent,
    // keeping the main target content (cleared with camera fill color, or editor grid)
    if (normalSample.a < 0.5)
    {
        gl_FragColor = vec4(0.0, 0.0, 0.0, 0.0);
        return;
    }

    vec3 normal = normalize(normalSample.xyz);
    vec3 worldPosition = texture2D(u_positionsTex, v_texCoords).xyz;

    vec3 lighting = vec3(u_ambient);
    int lightsCount = int(u_lightsCount + 0.5);
    int shadowLightIndex = int(u_shadowLightIndex + 0.5);
    for (int i = 0; i < 8; i++)
    {
        if (i >= lightsCount)
            break;

        vec4 lightColor = u_lightColors[i];
        vec4 lightVector = u_lightVectors[i];

        vec3 toLight;
        float attenuation = 1.0;
        if (lightColor.w > 0.5)
        {
            vec3 delta = lightVector.xyz - worldPosition;
            float dist = max(length(delta), 0.0001);
            toLight = delta/dist;
            attenuation = clamp(1.0 - dist/max(lightVector.w, 0.0001), 0.0, 1.0);
        }
        else
            toLight = normalize(lightVector.xyz);

        float ndl = max(dot(normal, toLight), 0.0);

        if (u_shadowsEnabled > 0.5 && i == shadowLightIndex)
            attenuation *= shadowFactor(worldPosition, ndl);

        lighting += lightColor.rgb*attenuation*ndl;
    }

    gl_FragColor = vec4(albedo.rgb*lighting, 1.0);
}
