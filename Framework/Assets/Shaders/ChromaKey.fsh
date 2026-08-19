varying vec4 v_color;
varying vec2 v_texCoords;

uniform sampler2D u_texture;

uniform vec4 u_keyColor;    // screen color, alpha ignored; needs one dominant channel (green/blue screen)
uniform float u_similarity; // screen amount where transparency starts
uniform float u_smoothness; // matte transition width on top of similarity
uniform float u_spill;      // despill strength: how much the screen channel is clamped to the others
uniform float u_choke;      // matte black clip: erodes the semi-transparent edge

void main()
{
    vec4 tex = texture2D(u_texture, v_texCoords);
    vec3 k = u_keyColor.rgb;

    // Difference key against the known uniform backdrop: the matte is a wide ramp of the
    // weighted distance to the key, so alpha stays proportional to the subject fraction in
    // mixed edge pixels — the edge keeps its halftones. The key's dominant channel counts
    // double so dark subject tones don't read as backdrop
    vec3 w = vec3(1.0);
    if (k.g >= k.r && k.g >= k.b)
        w.g = 2.0;
    else if (k.r >= k.g && k.r >= k.b)
        w.r = 2.0;
    else
        w.b = 2.0;

    float dist = length((tex.rgb - k)*w);

    float alpha = smoothstep(u_similarity, u_similarity + max(u_smoothness, 0.0001), dist);
    alpha = clamp((alpha - u_choke)/max(1.0 - u_choke, 0.0001), 0.0, 1.0);

    // Screen subtraction: the key's share of the pixel goes to black and the subject share
    // is restored by exact unpremultiply — the edge stays semi-transparent with no key tint
    vec3 fg = clamp(tex.rgb - (1.0 - alpha)*k, 0.0, 1.0);
    fg = clamp(fg/max(alpha, 0.02), 0.0, 1.0);

    // Despill: clamp the screen's dominant channel to the other two
    float spill = clamp(u_spill, 0.0, 1.0);
    if (k.g >= k.r && k.g >= k.b)
        fg.g = mix(fg.g, min(fg.g, max(fg.r, fg.b)), spill);
    else if (k.r >= k.g && k.r >= k.b)
        fg.r = mix(fg.r, min(fg.r, max(fg.g, fg.b)), spill);
    else
        fg.b = mix(fg.b, min(fg.b, max(fg.r, fg.g)), spill);

    gl_FragColor = vec4(fg, alpha)*v_color;
}
