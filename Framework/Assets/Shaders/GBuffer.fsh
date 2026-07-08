// GL backends don't support MRT G-buffer yet: outputs albedo only (deferred pipeline falls back to forward)
varying vec4 v_color;
varying vec2 v_texCoords;
varying vec3 v_normal;
varying vec3 v_worldPos;

uniform sampler2D u_texture;

void main()
{
    gl_FragColor = v_color * texture2D(u_texture, v_texCoords);
}
