#if defined(GL_ES) && !defined(O2_GLSL_ES3)
#extension GL_OES_standard_derivatives : enable
#endif

#ifdef GL_ES
#ifdef GL_FRAGMENT_PRECISION_HIGH
precision highp float;
#else
precision mediump float;
#endif
#endif

// GLSL 1.x by default; with O2_GLSL_ES3 (the engine puts "#version 300 es" before it on WebGL 2 and GL ES 3) - GLSL ES 3
#ifdef O2_GLSL_ES3
#define O2_VARYING in
#define O2_TEXTURE texture
#define O2_FRAG_COLOR o2_FragColor
out vec4 o2_FragColor;
#else
#define O2_VARYING varying
#define O2_TEXTURE texture2D
#define O2_FRAG_COLOR gl_FragColor
#endif

// Screen derivatives: core in GLSL ES 3 and desktop GLSL, an extension in GLSL ES 1
#if defined(O2_GLSL_ES3) || !defined(GL_ES) || defined(GL_OES_standard_derivatives)
#define O2_DERIVATIVES
#endif

// Distance field text (o2::SdfFont, o2::SdfTextStyle): the layers of the style composited front to back. A layer is a
// colour (solid or a gradient) over the union of up to two taps of the field: the glyph grown, blurred and offset.
// Distances are in pixels of the text, the edges are antialiased over a screen pixel at any scale

#define MAX_LAYERS 6

O2_VARYING vec4 v_color;
O2_VARYING vec2 v_texCoords;
O2_VARYING vec2 v_gradient;
O2_VARYING vec4 v_cell;
O2_VARYING float v_pxPerTexel;

uniform sampler2D u_texture;
uniform vec4 u_field;      // layers count, distance range in field pixels, atlas width, atlas height
uniform vec4 u_layers[36]; // per layer: colour 1, colour 2, (gradient, taps, edge scale, sharp corners), 2 taps, alphas
uniform vec4 u_pass;       // the layer drawn by itself (the effects over the whole text: a pass per layer), -1 - all

float Median(vec3 v)
{
    return max(min(v.r, v.g), min(max(v.r, v.g), v.b));
}

// Coverage of the field at the point: the glyph grown by the dilation, its edge a smoothstep of the half width
float FieldCoverage(vec2 uv, float sharpCorners, float range, float dilation, float halfWidth)
{
    vec4 field = O2_TEXTURE(u_texture, clamp(uv, v_cell.xy, v_cell.zw));
    float distance = ((sharpCorners > 0.5 ? Median(field.rgb) : field.a) - 0.5)*range + dilation;
    return smoothstep(-halfWidth, halfWidth, distance);
}

void main()
{
    vec2 atlasSize = u_field.zw;
#ifdef O2_DERIVATIVES
    vec2 footprint = fwidth(v_texCoords)*atlasSize;
    float screenPx = max(0.5*(footprint.x + footprint.y), 0.0001)*v_pxPerTexel;
#else
    float screenPx = 1.0;
#endif
    vec2 pxToUV = vec2(1.0)/(atlasSize*v_pxPerTexel);
    float range = u_field.y*v_pxPerTexel;

    vec4 result = vec4(0.0);
    vec4 single = vec4(0.0);
    for (int i = 0; i < MAX_LAYERS; i++)
    {
        if (float(i) >= u_field.x)
            break;

        vec4 color1 = u_layers[i*6];
        vec4 color2 = u_layers[i*6 + 1];
        vec4 info = u_layers[i*6 + 2];
        vec4 alphas = u_layers[i*6 + 5];

        float uncovered = 1.0;
        for (int j = 0; j < 2; j++)
        {
            if (float(j) >= info.y)
                break;

            vec4 tap = u_layers[i*6 + 3 + j];
            vec2 uv = v_texCoords + vec2(-tap.x, tap.y)*pxToUV;
            float edge = 0.5*screenPx*info.z;
            float coverage;
            if (tap.w > 3.0*edge)
            {
                // A blur wider than the edge is sampled at four points of its kernel: a thin shape blurred is fainter
                // than its edge
                vec2 spread = vec2(0.322*tap.w)*pxToUV;
                float halfWidth = min(max(0.72*tap.w, edge), 0.5*range);
                float dilation = min(tap.z, 0.5*range - halfWidth);
                coverage = 0.25*(FieldCoverage(uv + vec2(-spread.x, -spread.y), info.w, range, dilation, halfWidth) +
                                 FieldCoverage(uv + vec2(spread.x, -spread.y), info.w, range, dilation, halfWidth) +
                                 FieldCoverage(uv + vec2(-spread.x, spread.y), info.w, range, dilation, halfWidth) +
                                 FieldCoverage(uv + vec2(spread.x, spread.y), info.w, range, dilation, halfWidth));
            }
            else
            {
                // The field keeps distances within the half range: a reach past it is cut there, not filled
                float halfWidth = min(max(tap.w, edge), 0.5*range);
                float dilation = min(tap.z, 0.5*range - halfWidth);
                coverage = FieldCoverage(uv, info.w, range, dilation, halfWidth);
            }

            uncovered *= 1.0 - coverage*(j == 0 ? alphas.x : alphas.y);
        }

        float t = info.x < -0.5 ? 0.0 : (info.x < 0.5 ? v_gradient.x : v_gradient.y);
        vec4 color = mix(color1, color2, clamp(t, 0.0, 1.0));
        float alpha = color.a*(1.0 - uncovered);

        // A pass of one layer: the layers of the letter in front of it are cut out of it
        if (u_pass.x > -0.5 && abs(float(i) - u_pass.x) < 0.5)
        {
            single = vec4(color.rgb, alpha*(1.0 - result.a));
            break;
        }

        result.rgb += (1.0 - result.a)*color.rgb*alpha;
        result.a += (1.0 - result.a)*alpha;
    }

    vec4 straight = result.a > 0.0 ? vec4(result.rgb/result.a, result.a) : vec4(0.0);
    O2_FRAG_COLOR = (u_pass.x > -0.5 ? single : straight)*v_color;
}
