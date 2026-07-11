#pragma once

#ifdef PLATFORM_WASM

#include "o2/Render/TextureRef.h"
#include "o2/Render/WebAssembly/OpenGL.h"
#include "o2/Utils/Math/Vector2.h"
#include "o2/Utils/Math/Vertex.h"
#include "o2/Utils/Types/CommonTypes.h"

namespace o2
{
    class Texture;

    class RenderBase
    {
    protected:
        GLuint mActiveProgram = 0;               // Currently active shader program
        GLint  mActiveMvpUniform = -1;           // Currently active transform uniform location
        GLint  mActiveTextureSample = -1;        // Currently active texture sampler uniform location
        GLint  mActivePosAttribute = -1;         // Currently active position attribute location
        GLint  mActiveColorAttribute = -1;       // Currently active color attribute location
        GLint  mActiveUVAttribute = -1;          // Currently active texcoords attribute location
        GLint  mActiveNormalAttribute = -1;      // Currently active normal attribute location
        GLint  mActiveBoneIndicesAttribute = -1; // Currently active bone indices attribute location
        GLint  mActiveBoneWeightsAttribute = -1; // Currently active bone weights attribute location

        VertexType mBoundAttributesVertexType; // Vertex type the attribute pointers are bound for

        bool mRenderTargetAttachmentsDirty = false; // Current FBO needs MRT attachments and draw buffers sync

        float mCurrentMvp[16]; // Cached MVP matrix for material rebinding

        const static int mBuffersPoolsSize = 3;       // Count of buffers in pools
        GLuint mVertexBuffersPool[mBuffersPoolsSize]; // GPU vertex buffer objects
        GLuint mIndexBuffersPool[mBuffersPoolsSize];  // GPU index buffer objects
        int    mCurrentBufferIdx = 0;                 // Current buffer pool index
    };
};

#endif // PLATFORM_WASM
