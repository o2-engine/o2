// GL backends don't support the deferred path yet: composites albedo only (deferred pipeline falls back to forward)
varying vec4 v_color;
varying vec2 v_texCoords;

uniform sampler2D u_texture;

void main()
{
    gl_FragColor = vec4((v_color * texture2D(u_texture, v_texCoords)).rgb, 1.0);
}
