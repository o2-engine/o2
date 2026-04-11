#pragma once

#ifdef PLATFORM_IOS

#include "o2/Utils/Math/Vertex.h"
#include "o2/Utils/Types/Containers/Vector.h"

namespace o2
{
    struct MTLMaterialImpl;

    struct SamplerLocation
    {
        int  textureIndex = -1;
        UInt texCoordParam = VertexParam::TexCoord0;
    };

    class MaterialBase
    {
        friend class Render;

    protected:
        MTLMaterialImpl* mImpl = nullptr;

        UInt mProgram = 0;

        int mTransformUniform = -1;
        int mTextureUniform = -1;
        int mPositionAttribute = -1;
        int mColorAttribute = -1;
        int mTexCoordsAttribute = -1;
        int mNormalAttribute = -1;

        mutable Vector<int> mParamUniformLocations;
        Vector<SamplerLocation> mSamplerLocations;
    };
}

#endif // PLATFORM_IOS
