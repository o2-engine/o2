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
extern PFNGLUNIFORM1FPROC                 glUniform1f;
extern PFNGLUNIFORM2FPROC                 glUniform2f;
extern PFNGLUNIFORM4FPROC                 glUniform4f;
extern PFNGLUNIFORM1FVPROC                glUniform1fv;
extern PFNGLUNIFORM4FVPROC                glUniform4fv;
extern PFNGLGETACTIVEUNIFORMPROC          glGetActiveUniform;
extern PFNGLCOMPRESSEDTEXIMAGE2DPROC      glCompressedTexImage2D;
extern PFNGLBLENDFUNCSEPARATEPROC         glBlendFuncSeparate;
extern PFNGLBUFFERSUBDATAPROC             glBufferSubData;
extern PFNWGLSWAPINTERVALEXTPROC          wglSwapIntervalEXT;
extern PFNGLSTENCILFUNCSEPARATEPROC       glStencilFuncSeparate;
extern PFNGLSTENCILOPSEPARATEPROC         glStencilOpSeparate;
extern PFNGLSTENCILMASKSEPARATEPROC       glStencilMaskSeparate;
extern PFNGLBLENDEQUATIONSEPARATEPROC     glBlendEquationSeparate;
extern PFNGLCOMPRESSEDTEXSUBIMAGE2DPROC   glCompressedTexSubImage2D;
extern PFNGLGENERATEMIPMAPPROC            glGenerateMipmap;
extern PFNGLGENFRAMEBUFFERSPROC           glGenFramebuffers;
extern PFNGLBINDFRAMEBUFFERPROC           glBindFramebuffer;
extern PFNGLFRAMEBUFFERTEXTURE2DPROC      glFramebufferTexture2D;
extern PFNGLDELETEFRAMEBUFFERSPROC        glDeleteFramebuffers;
extern PFNGLGETACTIVEATTRIBPROC           glGetActiveAttrib;
extern PFNGLGETACTIVEUNIFORMPROC          glGetActiveUniform;
extern PFNGLISRENDERBUFFERPROC            glIsRenderbuffer;
extern PFNGLBINDRENDERBUFFERPROC          glBindRenderbuffer;
extern PFNGLDELETERENDERBUFFERSPROC       glDeleteRenderbuffers;
extern PFNGLGENRENDERBUFFERSPROC          glGenRenderbuffers;
extern PFNGLRENDERBUFFERSTORAGEPROC       glRenderbufferStorage;
extern PFNGLGETRENDERBUFFERPARAMETERIVPROC glGetRenderbufferParameteriv;
extern PFNGLISFRAMEBUFFERPROC             glIsFramebuffer;
extern PFNGLCHECKFRAMEBUFFERSTATUSPROC    glCheckFramebufferStatus;
extern PFNGLFRAMEBUFFERTEXTURE1DPROC      glFramebufferTexture1D;
extern PFNGLFRAMEBUFFERTEXTURE3DPROC      glFramebufferTexture3D;
extern PFNGLFRAMEBUFFERRENDERBUFFERPROC   glFramebufferRenderbuffer;
extern PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVPROC glGetFramebufferAttachmentParameteriv;
extern PFNGLUNIFORM1IVPROC                glUniform1iv;
extern PFNGLUNIFORM2IVPROC                glUniform2iv;
extern PFNGLUNIFORM3IVPROC                glUniform3iv;
extern PFNGLUNIFORM4IVPROC                glUniform4iv;
extern PFNGLUNIFORM2IPROC                 glUniform2i;
extern PFNGLUNIFORM3IPROC                 glUniform3i;
extern PFNGLUNIFORM4IPROC                 glUniform4i;
extern PFNGLUNIFORM1FVPROC                glUniform1fv;
extern PFNGLUNIFORM2FVPROC                glUniform2fv;
extern PFNGLUNIFORM3FVPROC                glUniform3fv;
extern PFNGLUNIFORM4FVPROC                glUniform4fv;
extern PFNGLUNIFORM1FPROC                 glUniform1f;
extern PFNGLUNIFORM2FPROC                 glUniform2f;
extern PFNGLUNIFORM3FPROC                 glUniform3f;
extern PFNGLUNIFORM4FPROC                 glUniform4f;
extern PFNGLUNIFORMMATRIX2FVPROC          glUniformMatrix2fv;
extern PFNGLUNIFORMMATRIX3FVPROC          glUniformMatrix3fv;

#endif // PLATFORM_WINDOWS
