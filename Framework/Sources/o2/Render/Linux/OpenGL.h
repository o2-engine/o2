#pragma once

#if defined(PLATFORM_LINUX) && !defined(O2_RENDER_GLES2)

#define Font XFont

#include <GL/gl.h>
#include <GL/glx.h>

// Undef X-lib shit
#undef Bool
#undef None
#undef CurrentTime
#undef Font

namespace o2
{
    class LogStream;
}

// Getting openGL extensions
void GetGLExtensions(o2::LogStream* log = nullptr);

// Returns opengl error description by id
const char* GetGLErrorDesc(GLenum errorId);

// Checks OpenGL extension supporting
bool IsGLExtensionSupported(const char *extension);

// Checks OpenGL error
void glCheckError(const char* filename = nullptr, unsigned int line = 0);

#if RENDER_DEBUG
#    define GL_CHECK_ERROR() glCheckError(__FILE__, __LINE__);
#else
#    define GL_CHECK_ERROR()
#endif

extern PFNGLGENFRAMEBUFFERSEXTPROC        glGenFramebuffersEXT;
extern PFNGLBINDFRAMEBUFFEREXTPROC        glBindFramebufferEXT;
extern PFNGLFRAMEBUFFERTEXTUREPROC        glFramebufferTexture;
extern PFNGLDRAWBUFFERSPROC               glDrawBuffers;
extern PFNGLDELETEBUFFERSPROC             glDeleteBuffers;
extern PFNGLDELETEFRAMEBUFFERSPROC        glDeleteFramebuffersEXT;
extern PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC glCheckFramebufferStatusEXT;
extern PFNGLGENBUFFERSPROC                glGenBuffers;
extern PFNGLBINDBUFFERPROC                   glBindBuffer;
extern PFNGLBUFFERDATAPROC                glBufferData;
extern PFNGLCREATESHADERPROC              glCreateShader;
extern PFNGLDELETESHADERPROC              glDeleteShader;
extern PFNGLSHADERSOURCEPROC              glShaderSource;
extern PFNGLCOMPILESHADERPROC             glCompileShader;
extern PFNGLGETSHADERIVPROC               glGetShaderiv;
extern PFNGLGETSHADERINFOLOGPROC          glGetShaderInfoLog;
extern PFNGLCREATEPROGRAMPROC             glCreateProgram;
extern PFNGLATTACHSHADERPROC              glAttachShader;
extern PFNGLLINKPROGRAMPROC               glLinkProgram;
extern PFNGLGETPROGRAMIVPROC              glGetProgramiv;
extern PFNGLGETPROGRAMINFOLOGPROC         glGetProgramInfoLog;
extern PFNGLUSEPROGRAMPROC                glUseProgram;
extern PFNGLGETUNIFORMLOCATIONPROC        glGetUniformLocation;
extern PFNGLGETATTRIBLOCATIONPROC         glGetAttribLocation;
extern PFNGLDELETEPROGRAMPROC             glDeleteProgram;
extern PFNGLVERTEXATTRIBPOINTERPROC       glVertexAttribPointer;
extern PFNGLENABLEVERTEXATTRIBARRAYPROC   glEnableVertexAttribArray;
extern PFNGLUNIFORMMATRIX4FVPROC          glUniformMatrix4fv;
extern PFNGLUNIFORM1IPROC                 glUniform1i;
extern PFNGLUNIFORM1FPROC                 glUniform1f;
extern PFNGLUNIFORM2FPROC                 glUniform2f;
extern PFNGLUNIFORM4FPROC                 glUniform4f;
extern PFNGLBLENDFUNCSEPARATEPROC         glBlendFuncSeparate;
extern PFNGLBUFFERSUBDATAPROC             glBufferSubData;

#endif // PLATFORM_LINUX
