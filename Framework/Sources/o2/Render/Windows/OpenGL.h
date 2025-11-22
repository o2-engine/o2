#pragma once

#ifdef PLATFORM_WINDOWS

#include <windows.h>    
#include <GL/gl.h>
#include <GL/glu.h>
#include "3rdPartyLibs/OpenGL/glext.h"
#include "3rdPartyLibs/OpenGL/wglext.h"


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
extern PFNGLGETACTIVEATTRIBPROC           glGetActiveAttrib;
extern PFNGLGETACTIVEUNIFORMPROC          glGetActiveUniform;
extern PFNGLGETPROGRAMINFOLOGPROC         glGetProgramInfoLog;
extern PFNGLUSEPROGRAMPROC                glUseProgram;
extern PFNGLGETUNIFORMLOCATIONPROC        glGetUniformLocation;
extern PFNGLGETATTRIBLOCATIONPROC         glGetAttribLocation;
extern PFNGLDELETEPROGRAMPROC             glDeleteProgram;
extern PFNGLVERTEXATTRIBPOINTERPROC       glVertexAttribPointer;
extern PFNGLENABLEVERTEXATTRIBARRAYPROC   glEnableVertexAttribArray;
extern PFNGLUNIFORMMATRIX4FVPROC          glUniformMatrix4fv;
extern PFNGLACTIVETEXTUREPROC             glActiveTexture;
extern PFNGLUNIFORM1IPROC                 glUniform1i;
extern PFNGLCOMPRESSEDTEXIMAGE2DPROC      glCompressedTexImage2D;
extern PFNGLBLENDFUNCSEPARATEPROC         glBlendFuncSeparate;
extern PFNGLBUFFERSUBDATAPROC             glBufferSubData;
extern PFNWGLSWAPINTERVALEXTPROC          wglSwapIntervalEXT;

#endif // PLATFORM_WINDOWS
