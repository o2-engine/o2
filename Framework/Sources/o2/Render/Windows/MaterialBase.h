#pragma once

#ifdef PLATFORM_WINDOWS

#include "o2/Render/Windows/OpenGL.h"
#include "o2/Utils/Types/Containers/Vector.h"

namespace o2
{
    // Cached GL locations for an additional texture sampler
    struct SamplerLocation
    {
        GLint samplerUniform = -1;   // Sampler uniform location
        GLint texCoordsAttribute = -1; // Texcoord attribute location
    };

    // ------------------------------------------------------------
    // Platform-specific (Windows/OpenGL) material data and handles
    // ------------------------------------------------------------
    class MaterialBase
    {
        friend class Render;

    protected:
        GLuint mProgram = 0; // OpenGL shader program handle

        GLint mTransformUniform = -1;   // Location of u_transformMatrix uniform
        GLint mTextureUniform = -1;     // Location of u_texture uniform (primary)
        GLint mPositionAttribute = -1;  // Location of a_position attribute
        GLint mColorAttribute = -1;     // Location of a_color attribute
        GLint mTexCoordsAttribute = -1; // Location of a_texCoords attribute (primary)
        GLint mNormalAttribute = -1;    // Location of a_normal attribute

        // Cached uniform locations for custom params (filled in PlatformBuild, used in PlatformApplyParams)
        mutable Vector<GLint>  mParamUniformLocations;
        mutable Vector<GLenum> mParamUniformTypes; // Declared uniform types, for float vector dispatch
        mutable Vector<GLint>  mParamUniformSizes; // Declared uniform array sizes, in elements

        // Cached locations for additional texture samplers
        Vector<SamplerLocation> mSamplerLocations;
    };
}

#endif // PLATFORM_WINDOWS
