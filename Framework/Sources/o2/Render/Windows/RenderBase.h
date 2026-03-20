#pragma once

#ifdef PLATFORM_WINDOWS

#include "o2/Render/TextureRef.h"
#include "o2/Render/Windows/OpenGL.h"
#include "o2/Utils/Math/Vector2.h"
#include "o2/Utils/Math/Vertex.h"
#include "o2/Utils/Types/CommonTypes.h"


namespace o2
{
    class Texture;

    class RenderBase
    {
    protected:
        HGLRC mGLContext; // OpenGL context
        HDC   mHDC;       // Windows frame device context

        GLuint mActiveProgram;           // Currently active shader program
        GLint  mActiveMvpUniform;        // Currently active transform uniform location
        GLint  mActiveTextureSample;     // Currently active texture sampler uniform location
        GLint  mActivePosAttribute;      // Currently active position attribute location
        GLint  mActiveColorAttribute;    // Currently active color attribute location
        GLint  mActiveUVAttribute;       // Currently active texcoords attribute location
        GLint  mActiveNormalAttribute = -1; // Currently active normal attribute location

        float  mCurrentMvp[16];          // Cached MVP matrix for material rebinding

        const static int mBuffersPoolsSize = 3;       // Count of buffers in pools
        GLuint mVertexBuffersPool[mBuffersPoolsSize]; // GPU vertex buffer objects
        GLuint mIndexBuffersPool[mBuffersPoolsSize];  // GPU index buffer objects
        int    mCurrentBufferIdx = 0;                 // Current buffer pool index

    protected:
    };
};

#endif // PLATFORM_WINDOWS
