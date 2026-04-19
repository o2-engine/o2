#pragma once

#ifdef PLATFORM_LINUX

#include "o2/Render/Linux/OpenGL.h"
#include "o2/Utils/Types/Containers/Vector.h"

namespace o2
{
    struct SamplerLocation
    {
        GLint samplerUniform = -1;
        GLint texCoordsAttribute = -1;
    };

    class MaterialBase
    {
        friend class Render;

    protected:
        GLuint mProgram = 0;

        GLint mTransformUniform = -1;
        GLint mTextureUniform = -1;
        GLint mPositionAttribute = -1;
        GLint mColorAttribute = -1;
        GLint mTexCoordsAttribute = -1;
        GLint mNormalAttribute = -1;

        mutable Vector<GLint> mParamUniformLocations;

        Vector<SamplerLocation> mSamplerLocations;
    };
}

#endif // PLATFORM_LINUX
