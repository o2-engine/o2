// GLSL 1.x by default; with O2_GLSL_ES3 (the engine puts "#version 300 es" before it on WebGL 2 and GL ES 3) - GLSL ES 3
#ifdef O2_GLSL_ES3
#define O2_ATTRIBUTE in
#define O2_VARYING out
#else
#define O2_ATTRIBUTE attribute
#define O2_VARYING varying
#endif

uniform mat4 u_transformMatrix;

O2_ATTRIBUTE vec4 a_position;
O2_ATTRIBUTE vec4 a_color;
O2_ATTRIBUTE vec2 a_texCoords;
O2_ATTRIBUTE vec2 a_texCoords2;
O2_ATTRIBUTE vec2 a_texCoords3;
O2_ATTRIBUTE vec3 a_normal;

O2_VARYING vec4 v_color;
O2_VARYING vec2 v_texCoords;
O2_VARYING vec2 v_gradient;
O2_VARYING vec4 v_cell;
O2_VARYING float v_pxPerTexel;

// The text vertices: atlas coordinates, the gradient coordinates of the style, the field cell of the glyph (its
// corners in the second extra set and the normal) and pixels of the text per field pixel (the normal's z)
void main()
{
    v_color = a_color;
    v_texCoords = a_texCoords;
    v_gradient = a_texCoords2;
    v_cell = vec4(a_texCoords3, a_normal.xy);
    v_pxPerTexel = a_normal.z;
    gl_Position = u_transformMatrix * a_position;
}
