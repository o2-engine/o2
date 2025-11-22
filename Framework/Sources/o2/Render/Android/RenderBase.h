#pragma once

#ifdef PLATFORM_ANDROID

#include "Render/Android/OpenGL.h"

namespace o2
{
    class RenderBase
    {
    protected:
        GLuint   mStdShader;                      // Standard shader program
        GLint    mStdShaderMvpUniform;            // Standard shader matrix input parameter
        GLint    mStdShaderTextureSample;         // Standard shader texture sample input parameter
        GLint    mStdShaderPosAttribute;          // Standard shader vertex position attribute
        GLint    mStdShaderColorAttribute;        // Standard shader vertex color attribute
        GLint    mStdShaderUVAttribute;           // Standard shader texture coords attribute

        GLuint   mVertexBufferObject;             // Batch vercities buffer
        GLuint   mIndexBufferObject;              // Batch polygons indexes buffer

        UInt8*   mVertexData = nullptr;           // Vertex data buffer
        UInt16*  mVertexIndexData = nullptr;      // Index data buffer
        UInt     mVertexBufferSize;               // Maximum size of vertex buffer
        UInt     mIndexBufferSize;                // Maximum size of index buffer

    protected:
        // Builds vertex and fragment shaders
        GLuint LoadShader(GLenum shaderType, const char *source);

        // Builds vertex and fragment shaders
        GLuint BuildShaderProgram(const char* vertexSource, const char* fragmentSource);

        // Initializes standard shader
        void InitializeStandardShader();
    };
};

#endif // PLATFORM_ANDROID
