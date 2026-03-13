#pragma once

#ifdef PLATFORM_WINDOWS

#include "o2/Render/TextureRef.h"
#include "o2/Render/Windows/OpenGL.h"
#include "o2/Utils/Math/Vector2.h"
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

        float  mCurrentMvp[16];          // Cached MVP matrix for material rebinding

        const static int mBuffersPoolsSize = 3;       // Count of buffers in pools
        GLuint mVertexBuffersPool[mBuffersPoolsSize]; // Batch vertices buffer
        GLuint mIndexBuffersPool[mBuffersPoolsSize];  // Batch polygons indexes buffer
        int    mCurrentBufferIdx = 0;                 // Current buffer index
        int    mVertexBufferIdx = 0;                  // Current vertex index in vertex buffer
        int    mIndexBufferIdx = 0;                   // Current index count in index buffer

        UInt8*       mVertexData = nullptr;      // Vertex data buffer
        VertexIndex* mVertexIndexData = nullptr; // Index data buffer
        UInt         mVertexBufferSize;          // Maximum size of vertex buffer
        UInt         mIndexBufferSize;           // Maximum size of index buffer

    protected:
        // Binds next buffers from pool
        void BindNextPoolBuffers();
    };
};

#endif // PLATFORM_WINDOWS
