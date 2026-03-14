#pragma once

#ifdef PLATFORM_WINDOWS

#include "o2/Render/Windows/OpenGL.h"
#include "o2/Utils/Types/Containers/Vector.h"

namespace o2
{
    // ------------------------------------------------------------
    // Platform-specific (Windows/OpenGL) material data and handles
    // ------------------------------------------------------------
    class MaterialBase
    {
        friend class Render;

    protected:
        GLuint mProgram = 0; // OpenGL shader program handle

        GLint mTransformUniform = -1;   // Location of u_transformMatrix uniform
        GLint mTextureUniform = -1;     // Location of u_texture uniform
        GLint mPositionAttribute = -1;  // Location of a_position attribute
        GLint mColorAttribute = -1;     // Location of a_color attribute
        GLint mTexCoordsAttribute = -1; // Location of a_texCoords attribute

        // Cached uniform locations for custom params (filled in PlatformBuild, used in PlatformApplyParams)
        mutable Vector<GLint> mParamUniformLocations;
    };
}

#endif // PLATFORM_WINDOWS
