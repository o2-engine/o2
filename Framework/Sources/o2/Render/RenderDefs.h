#pragma once

namespace o2
{
    enum class ShaderType
    {
        Vertex,
        Fragment
    };

    // shader parameters
    inline const String VS_POSITION{ "a_position" };
    inline const String VS_COLOR{ "a_color" };
    inline const String VS_TEX_COORDS{ "a_texCoords" };
    inline const String VS_MVP{ "u_transformMatrix" };

    inline const String FS_TEX_SAMPLE{ "u_texture" };

}
// --- META ---

PRE_ENUM_META(o2::ShaderType);
// --- END META ---
