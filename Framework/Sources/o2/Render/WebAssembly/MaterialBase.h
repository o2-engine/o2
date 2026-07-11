#pragma once

#ifdef PLATFORM_WASM

#include "o2/Render/WebAssembly/OpenGL.h"
#include "o2/Utils/Types/Containers/Vector.h"

namespace o2
{
    // Cached GL locations for an additional texture sampler
    struct SamplerLocation
    {
        GLint samplerUniform = -1;     // Sampler uniform location
        GLint texCoordsAttribute = -1; // Texcoord attribute location
    };

    // ----------------------------------------------------------------
    // Platform-specific (WebAssembly/WebGL) material data and handles
    // ----------------------------------------------------------------
    class MaterialBase
    {
        friend class Render;

    protected:
        GLuint mProgram = 0; // OpenGL shader program handle

        GLint mTransformUniform = -1;     // Location of u_transformMatrix uniform
        GLint mTextureUniform = -1;       // Location of u_texture uniform (primary)
        GLint mPositionAttribute = -1;    // Location of a_position attribute
        GLint mColorAttribute = -1;       // Location of a_color attribute
        GLint mTexCoordsAttribute = -1;   // Location of a_texCoords attribute (primary)
        GLint mNormalAttribute = -1;      // Location of a_normal attribute
        GLint mBoneIndicesAttribute = -1; // Location of a_boneIndices attribute
        GLint mBoneWeightsAttribute = -1; // Location of a_boneWeights attribute

        mutable Vector<GLint>  mParamUniformLocations;
        mutable Vector<GLenum> mParamUniformTypes; // Declared uniform types, for float vector dispatch
        mutable Vector<GLint>  mParamUniformSizes; // Declared uniform array sizes, in elements

        Vector<SamplerLocation> mSamplerLocations;
    };
}

#endif // PLATFORM_WASM
