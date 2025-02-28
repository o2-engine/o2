#pragma once

#ifdef PLATFORM_MAC
#include "o2/Utils/Types/CommonTypes.h"
#include "o2/Utils/Math/Color.h"
#include "o2/Utils/Math/Rect.h"
#include "o2/Render/TextureRef.h"

namespace o2
{
    class RenderBase
    {
    protected:
        UInt mVertexBufferSize = 1000000; // Maximum size of vertex buffer
        UInt mIndexBufferSize = 3000000;  // Maximum size of index buffer
        UInt mUniformBufferSize = 2000;    // Maximum size of uniforms buffer
        
        UInt mVertexBufferOffset = 0;
        UInt mIndexBufferOffset = 0;
        UInt mUniformBufferOffset = 0;

        Color4 mClearColor; // Clear color for next pass
        bool mNeedClear = false;
        
        float mProjMatrix[16];      // Render projection matrix
        float mViewModelMatrix[16]; // Render view and model matrix
        
        bool  mScissorEnabled = false;
        RectI mScissorRect;
        
        TextureRef mWhiteTexture; // Default white texture
        
        friend struct RenderDevice;
    };
};

#endif // PLATFORM_MAC
