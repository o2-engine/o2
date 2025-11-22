#pragma once

#if defined(PLATFORM_LINUX) && !defined(O2_RENDER_GLES2)

#include "o2/Render/TextureRef.h"
#include "o2/Render/Linux/OpenGL.h"
#include "o2/Utils/Math/Vector2.h"
#include "o2/Utils/Types/CommonTypes.h"

namespace o2
{
    class Texture;

    class RenderBase
    {
    protected:
        GLXContext mGLContext; // OpenGL context

        GLuint mStdShader;               // Standard shader program
        GLint  mStdShaderMvpUniform;     // Standard shader matrix input parameter
        GLint  mStdShaderTextureSample;  // Standard shader texture sample input parameter
        GLint  mStdShaderPosAttribute;   // Standard shader vertex position attribute
        GLint  mStdShaderColorAttribute; // Standard shader vertex color attribute
        GLint  mStdShaderUVAttribute;    // Standard shader texture coords attribute

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
        // Builds vertex and fragment shaders
        GLuint LoadShader(GLenum shaderType, const char* source);

        // Builds vertex and fragment shaders
        GLuint BuildShaderProgram(const char* vertexSource, const char* fragmentSource);

        // Initializes standard shader
        void InitializeStandardShader();

        // BInds next buffers from pool
        void BindNextPoolBuffers();
    };
};

#endif // PLATFORM_LINUX
