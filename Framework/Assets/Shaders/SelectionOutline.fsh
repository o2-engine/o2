varying vec4 v_color;
varying vec2 v_texCoords;

uniform sampler2D u_texture;
uniform vec2 u_texelSize;

void main()
{
    // Silhouette border: the mask is empty here, but covered nearby (1-2 texels)
    float center = texture2D(u_texture, v_texCoords).a;
    if (center > 0.5)
    {
        gl_FragColor = vec4(0.0, 0.0, 0.0, 0.0);
        return;
    }

    float neighbors = 0.0;
    for (int y = -1; y <= 1; y++)
    {
        for (int x = -1; x <= 1; x++)
        {
            vec2 offset = vec2(float(x), float(y))*u_texelSize;
            neighbors = max(neighbors, texture2D(u_texture, v_texCoords + offset).a);
            neighbors = max(neighbors, texture2D(u_texture, v_texCoords + offset*2.0).a);
        }
    }

    if (neighbors > 0.5)
        gl_FragColor = v_color;
    else
        gl_FragColor = vec4(0.0, 0.0, 0.0, 0.0);
}
