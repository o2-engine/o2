#include "o2/stdafx.h"

#ifdef PLATFORM_WINDOWS

#include "OpenGL.h"
#include "o2/Utils/Debug/Log/LogStream.h"

// Returns address of function
PROC GetSafeWGLProcAddress(const char* id, o2::LogStream* log)
{
    PROC res = wglGetProcAddress(id);
    if (!res)
        log->Error("Failed to get function address: " + (o2::String)id);

    return res;
}

void GetGLExtensions(o2::LogStream* log /*= nullptr*/)
{
    glGenFramebuffersEXT = (PFNGLGENFRAMEBUFFERSEXTPROC)GetSafeWGLProcAddress("glGenFramebuffersEXT", log);
    glBindFramebufferEXT = (PFNGLBINDFRAMEBUFFEREXTPROC)GetSafeWGLProcAddress("glBindFramebufferEXT", log);
    glFramebufferTexture = (PFNGLFRAMEBUFFERTEXTUREPROC)GetSafeWGLProcAddress("glFramebufferTexture", log);
    glDrawBuffers = (PFNGLDRAWBUFFERSPROC)GetSafeWGLProcAddress("glDrawBuffers", log);
    glDeleteBuffers = (PFNGLDELETEBUFFERSPROC)GetSafeWGLProcAddress("glDeleteBuffers", log);
    glDeleteFramebuffersEXT = (PFNGLDELETEFRAMEBUFFERSPROC)GetSafeWGLProcAddress("glDeleteFramebuffersEXT", log);
    glCheckFramebufferStatusEXT = (PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC)GetSafeWGLProcAddress("glCheckFramebufferStatusEXT", log);
    glGenBuffers = (PFNGLGENBUFFERSPROC)GetSafeWGLProcAddress("glGenBuffers", log);
    glBindBuffer = (PFNGLBINDBUFFERPROC)GetSafeWGLProcAddress("glBindBuffer", log);
    glBufferData = (PFNGLBUFFERDATAPROC)GetSafeWGLProcAddress("glBufferData", log);
    glCreateShader = (PFNGLCREATESHADERPROC)GetSafeWGLProcAddress("glCreateShader", log);
    glDeleteShader = (PFNGLDELETESHADERPROC)GetSafeWGLProcAddress("glDeleteShader", log);
    glShaderSource = (PFNGLSHADERSOURCEPROC)GetSafeWGLProcAddress("glShaderSource", log);
    glCompileShader = (PFNGLCOMPILESHADERPROC)GetSafeWGLProcAddress("glCompileShader", log);
    glGetShaderiv = (PFNGLGETSHADERIVPROC)GetSafeWGLProcAddress("glGetShaderiv", log);
    glGetShaderInfoLog = (PFNGLGETSHADERINFOLOGPROC)GetSafeWGLProcAddress("glGetShaderInfoLog", log);
    glCreateProgram = (PFNGLCREATEPROGRAMPROC)GetSafeWGLProcAddress("glCreateProgram", log);
    glAttachShader = (PFNGLATTACHSHADERPROC)GetSafeWGLProcAddress("glAttachShader", log);
    glLinkProgram = (PFNGLLINKPROGRAMPROC)GetSafeWGLProcAddress("glLinkProgram", log);
    glGetProgramiv = (PFNGLGETPROGRAMIVPROC)GetSafeWGLProcAddress("glGetProgramiv", log);
    glGetProgramInfoLog = (PFNGLGETPROGRAMINFOLOGPROC)GetSafeWGLProcAddress("glGetProgramInfoLog", log);
    glUseProgram = (PFNGLUSEPROGRAMPROC)GetSafeWGLProcAddress("glUseProgram", log);
    glGetUniformLocation = (PFNGLGETUNIFORMLOCATIONPROC)GetSafeWGLProcAddress("glGetUniformLocation", log);
    glGetAttribLocation = (PFNGLGETATTRIBLOCATIONPROC)GetSafeWGLProcAddress("glGetAttribLocation", log);
    glDeleteProgram = (PFNGLDELETEPROGRAMPROC)GetSafeWGLProcAddress("glDeleteProgram", log);
    glVertexAttribPointer = (PFNGLVERTEXATTRIBPOINTERPROC)GetSafeWGLProcAddress("glVertexAttribPointer", log);
    glEnableVertexAttribArray = (PFNGLENABLEVERTEXATTRIBARRAYPROC)GetSafeWGLProcAddress("glEnableVertexAttribArray", log);
    glUniformMatrix4fv = (PFNGLUNIFORMMATRIX4FVPROC)GetSafeWGLProcAddress("glUniformMatrix4fv", log);
    glActiveTexture = (PFNGLACTIVETEXTUREPROC)GetSafeWGLProcAddress("glActiveTexture", log);
    glUniform1i = (PFNGLUNIFORM1IPROC)GetSafeWGLProcAddress("glUniform1i", log);
    glUniform1f = (PFNGLUNIFORM1FPROC)GetSafeWGLProcAddress("glUniform1f", log);
    glUniform2f = (PFNGLUNIFORM2FPROC)GetSafeWGLProcAddress("glUniform2f", log);
    glUniform4f = (PFNGLUNIFORM4FPROC)GetSafeWGLProcAddress("glUniform4f", log);
    glUniform1fv = (PFNGLUNIFORM1FVPROC)GetSafeWGLProcAddress("glUniform1fv", log);
    glUniform4fv = (PFNGLUNIFORM4FVPROC)GetSafeWGLProcAddress("glUniform4fv", log);
    glGetActiveUniform = (PFNGLGETACTIVEUNIFORMPROC)GetSafeWGLProcAddress("glGetActiveUniform", log);
    glCompressedTexImage2D = (PFNGLCOMPRESSEDTEXIMAGE2DPROC)GetSafeWGLProcAddress("glCompressedTexImage2D", log);
    glBlendFuncSeparate = (PFNGLBLENDFUNCSEPARATEPROC)GetSafeWGLProcAddress("glBlendFuncSeparate", log);
    glBufferSubData = (PFNGLBUFFERSUBDATAPROC)GetSafeWGLProcAddress("glBufferSubData", log);
    wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)GetSafeWGLProcAddress("wglSwapIntervalEXT", log);
    glStencilFuncSeparate = (PFNGLSTENCILFUNCSEPARATEPROC)GetSafeWGLProcAddress("glStencilFuncSeparate", log);
    glStencilOpSeparate = (PFNGLSTENCILOPSEPARATEPROC)GetSafeWGLProcAddress("glStencilOpSeparate", log);
    glStencilMaskSeparate = (PFNGLSTENCILMASKSEPARATEPROC)GetSafeWGLProcAddress("glStencilMaskSeparate", log);
    glBlendEquationSeparate = (PFNGLBLENDEQUATIONSEPARATEPROC)GetSafeWGLProcAddress("glBlendEquationSeparate", log);
    glCompressedTexSubImage2D = (PFNGLCOMPRESSEDTEXSUBIMAGE2DPROC)GetSafeWGLProcAddress("glCompressedTexSubImage2D", log);
    glGenerateMipmap = (PFNGLGENERATEMIPMAPPROC)GetSafeWGLProcAddress("glGenerateMipmap", log);
    glGenFramebuffers = (PFNGLGENFRAMEBUFFERSPROC)GetSafeWGLProcAddress("glGenFramebuffers", log);
    glBindFramebuffer = (PFNGLBINDFRAMEBUFFERPROC)GetSafeWGLProcAddress("glBindFramebuffer", log);
    glFramebufferTexture2D = (PFNGLFRAMEBUFFERTEXTURE2DPROC)GetSafeWGLProcAddress("glFramebufferTexture2D", log);
    glDeleteFramebuffers = (PFNGLDELETEFRAMEBUFFERSPROC)GetSafeWGLProcAddress("glDeleteFramebuffers", log);
    glGetActiveAttrib = (PFNGLGETACTIVEATTRIBPROC)GetSafeWGLProcAddress("glGetActiveAttrib", log);
    glGetActiveUniform = (PFNGLGETACTIVEUNIFORMPROC)GetSafeWGLProcAddress("glGetActiveUniform", log);
    glIsRenderbuffer = (PFNGLISRENDERBUFFERPROC)GetSafeWGLProcAddress("glIsRenderbuffer", log);
    glBindRenderbuffer = (PFNGLBINDRENDERBUFFERPROC)GetSafeWGLProcAddress("glBindRenderbuffer", log);
    glDeleteRenderbuffers = (PFNGLDELETERENDERBUFFERSPROC)GetSafeWGLProcAddress("glDeleteRenderbuffers", log);
    glGenRenderbuffers = (PFNGLGENRENDERBUFFERSPROC)GetSafeWGLProcAddress("glGenRenderbuffers", log);
    glRenderbufferStorage = (PFNGLRENDERBUFFERSTORAGEPROC)GetSafeWGLProcAddress("glRenderbufferStorage", log);
    glGetRenderbufferParameteriv = (PFNGLGETRENDERBUFFERPARAMETERIVPROC)GetSafeWGLProcAddress("glGetRenderbufferParameteriv", log);
    glIsFramebuffer = (PFNGLISFRAMEBUFFERPROC)GetSafeWGLProcAddress("glIsFramebuffer", log);
    glCheckFramebufferStatus = (PFNGLCHECKFRAMEBUFFERSTATUSPROC)GetSafeWGLProcAddress("glCheckFramebufferStatus", log);
    glFramebufferTexture1D = (PFNGLFRAMEBUFFERTEXTURE1DPROC)GetSafeWGLProcAddress("glFramebufferTexture1D", log);
    glFramebufferTexture3D = (PFNGLFRAMEBUFFERTEXTURE3DPROC)GetSafeWGLProcAddress("glFramebufferTexture3D", log);
    glFramebufferRenderbuffer = (PFNGLFRAMEBUFFERRENDERBUFFERPROC)GetSafeWGLProcAddress("glFramebufferRenderbuffer", log);
    glGetFramebufferAttachmentParameteriv = (PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVPROC)GetSafeWGLProcAddress("glGetFramebufferAttachmentParameteriv", log);
    glUniform1iv = (PFNGLUNIFORM1IVPROC)GetSafeWGLProcAddress("glUniform1iv", log);
    glUniform2iv = (PFNGLUNIFORM2IVPROC)GetSafeWGLProcAddress("glUniform2iv", log);
    glUniform3iv = (PFNGLUNIFORM3IVPROC)GetSafeWGLProcAddress("glUniform3iv", log);
    glUniform4iv = (PFNGLUNIFORM4IVPROC)GetSafeWGLProcAddress("glUniform4iv", log);
    glUniform2i = (PFNGLUNIFORM2IPROC)GetSafeWGLProcAddress("glUniform2i", log);
    glUniform3i = (PFNGLUNIFORM3IPROC)GetSafeWGLProcAddress("glUniform3i", log);
    glUniform4i = (PFNGLUNIFORM4IPROC)GetSafeWGLProcAddress("glUniform4i", log);
    glUniform1fv = (PFNGLUNIFORM1FVPROC)GetSafeWGLProcAddress("glUniform1fv", log);
    glUniform2fv = (PFNGLUNIFORM2FVPROC)GetSafeWGLProcAddress("glUniform2fv", log);
    glUniform3fv = (PFNGLUNIFORM3FVPROC)GetSafeWGLProcAddress("glUniform3fv", log);
    glUniform4fv = (PFNGLUNIFORM4FVPROC)GetSafeWGLProcAddress("glUniform4fv", log);
    glUniform1f = (PFNGLUNIFORM1FPROC)GetSafeWGLProcAddress("glUniform1f", log);
    glUniform2f = (PFNGLUNIFORM2FPROC)GetSafeWGLProcAddress("glUniform2f", log);
    glUniform3f = (PFNGLUNIFORM3FPROC)GetSafeWGLProcAddress("glUniform3f", log);
    glUniform4f = (PFNGLUNIFORM4FPROC)GetSafeWGLProcAddress("glUniform4f", log);
    glUniformMatrix2fv = (PFNGLUNIFORMMATRIX2FVPROC)GetSafeWGLProcAddress("glUniformMatrix2fv", log);
    glUniformMatrix3fv = (PFNGLUNIFORMMATRIX3FVPROC)GetSafeWGLProcAddress("glUniformMatrix3fv", log);
}

bool IsGLExtensionSupported(const char *extension)
{
    const GLubyte *extensions = NULL;
    const GLubyte *start;

    GLubyte *where, *terminator;
    /* Extension names should not have spaces. */

    where = (GLubyte *)strchr(extension, ' ');

    if (where || *extension == '\0')
        return 0;

    extensions = glGetString(GL_EXTENSIONS);

    /* It takes a bit of care to be fool-proof about parsing the
    OpenGL extensions string. Don't be fooled by sub-strings,
    etc. */

    start = extensions;
    for (;;)
    {
        where = (GLubyte *)strstr((const char *)start, extension);

        if (!where)
            break;

        terminator = where + strlen(extension);

        if (where == start || *(where - 1) == ' ')
            if (*terminator == ' ' || *terminator == '\0')
                return true;

        start = terminator;
    }

    return false;
}

const char* GetGLErrorDesc(GLenum errorId)
{
    if (errorId == GL_NO_ERROR) return "GL_NO_ERROR";
    if (errorId == GL_INVALID_ENUM) return "GL_INVALID_ENUM";
    if (errorId == GL_INVALID_VALUE) return "GL_INVALID_VALUE";
    if (errorId == GL_INVALID_OPERATION) return "GL_INVALID_OPERATION";
    if (errorId == GL_INVALID_FRAMEBUFFER_OPERATION) return "GL_INVALID_FRAMEBUFFER_OPERATION";
    if (errorId == GL_OUT_OF_MEMORY) return "GL_OUT_OF_MEMORY";
    if (errorId == GL_STACK_UNDERFLOW) return "GL_STACK_UNDERFLOW";
    if (errorId == GL_STACK_OVERFLOW) return "GL_STACK_OVERFLOW";

    return "UNKNOWN";
}

void glCheckError(const char* filename /*= nullptr*/, unsigned int line /*= 0*/)
{
    GLenum errId = glGetError();
    if (errId != GL_NO_ERROR)
    {
        o2Debug.LogError("OpenGL ERROR " + (o2::String)errId + ": " + (o2::String)GetGLErrorDesc(errId) +
                         " at file: " + (o2::String)(filename ? filename : "unknown") + " line: " + (o2::String)line);
    }
}

extern PFNGLGENFRAMEBUFFERSEXTPROC        glGenFramebuffersEXT = NULL;
extern PFNGLBINDFRAMEBUFFEREXTPROC        glBindFramebufferEXT = NULL;
extern PFNGLFRAMEBUFFERTEXTUREPROC        glFramebufferTexture = NULL;
extern PFNGLDRAWBUFFERSPROC               glDrawBuffers = NULL;
extern PFNGLDELETEBUFFERSPROC             glDeleteBuffers = NULL;
extern PFNGLDELETEFRAMEBUFFERSPROC        glDeleteFramebuffersEXT = NULL;
extern PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC glCheckFramebufferStatusEXT = NULL;
extern PFNGLGENBUFFERSPROC                glGenBuffers = NULL;
extern PFNGLBINDBUFFERPROC                glBindBuffer = NULL;
extern PFNGLBUFFERDATAPROC                glBufferData = NULL;
extern PFNGLCREATESHADERPROC              glCreateShader = NULL;
extern PFNGLDELETESHADERPROC              glDeleteShader = NULL;
extern PFNGLSHADERSOURCEPROC              glShaderSource = NULL;
extern PFNGLCOMPILESHADERPROC             glCompileShader = NULL;
extern PFNGLGETSHADERIVPROC               glGetShaderiv = NULL;
extern PFNGLGETSHADERINFOLOGPROC          glGetShaderInfoLog = NULL;
extern PFNGLCREATEPROGRAMPROC             glCreateProgram = NULL;
extern PFNGLATTACHSHADERPROC              glAttachShader = NULL;
extern PFNGLLINKPROGRAMPROC               glLinkProgram = NULL;
extern PFNGLGETPROGRAMIVPROC              glGetProgramiv = NULL;
extern PFNGLGETPROGRAMINFOLOGPROC         glGetProgramInfoLog = NULL;
extern PFNGLUSEPROGRAMPROC                glUseProgram = NULL;
extern PFNGLGETUNIFORMLOCATIONPROC        glGetUniformLocation = NULL;
extern PFNGLGETATTRIBLOCATIONPROC         glGetAttribLocation = NULL;
extern PFNGLDELETEPROGRAMPROC             glDeleteProgram = NULL;
extern PFNGLVERTEXATTRIBPOINTERPROC       glVertexAttribPointer = NULL;
extern PFNGLENABLEVERTEXATTRIBARRAYPROC   glEnableVertexAttribArray = NULL;
extern PFNGLUNIFORMMATRIX4FVPROC          glUniformMatrix4fv = NULL;
extern PFNGLACTIVETEXTUREPROC             glActiveTexture = NULL;
extern PFNGLUNIFORM1IPROC                 glUniform1i = NULL;
extern PFNGLUNIFORM1FPROC                 glUniform1f = NULL;
extern PFNGLUNIFORM2FPROC                 glUniform2f = NULL;
extern PFNGLUNIFORM4FPROC                 glUniform4f = NULL;
extern PFNGLUNIFORM1FVPROC                glUniform1fv = NULL;
extern PFNGLUNIFORM4FVPROC                glUniform4fv = NULL;
extern PFNGLGETACTIVEUNIFORMPROC          glGetActiveUniform = NULL;
extern PFNGLCOMPRESSEDTEXIMAGE2DPROC      glCompressedTexImage2D = NULL;
extern PFNGLBLENDFUNCSEPARATEPROC         glBlendFuncSeparate = NULL;
extern PFNGLBUFFERSUBDATAPROC             glBufferSubData = NULL;
extern PFNWGLSWAPINTERVALEXTPROC          wglSwapIntervalEXT = NULL;
extern PFNGLSTENCILFUNCSEPARATEPROC       glStencilFuncSeparate = NULL;
extern PFNGLSTENCILOPSEPARATEPROC         glStencilOpSeparate = NULL;
extern PFNGLSTENCILMASKSEPARATEPROC       glStencilMaskSeparate = NULL;
extern PFNGLBLENDEQUATIONSEPARATEPROC     glBlendEquationSeparate = NULL;
extern PFNGLCOMPRESSEDTEXSUBIMAGE2DPROC   glCompressedTexSubImage2D = NULL;
extern PFNGLGENERATEMIPMAPPROC            glGenerateMipmap = NULL;
extern PFNGLGENFRAMEBUFFERSPROC           glGenFramebuffers = NULL;
extern PFNGLBINDFRAMEBUFFERPROC           glBindFramebuffer = NULL;
extern PFNGLFRAMEBUFFERTEXTURE2DPROC      glFramebufferTexture2D = NULL;
extern PFNGLDELETEFRAMEBUFFERSPROC        glDeleteFramebuffers = NULL;
extern PFNGLGETACTIVEATTRIBPROC           glGetActiveAttrib = NULL;
extern PFNGLGETACTIVEUNIFORMPROC          glGetActiveUniform = NULL;
extern PFNGLISRENDERBUFFERPROC            glIsRenderbuffer = NULL;
extern PFNGLBINDRENDERBUFFERPROC          glBindRenderbuffer = NULL;
extern PFNGLDELETERENDERBUFFERSPROC       glDeleteRenderbuffers = NULL;
extern PFNGLGENRENDERBUFFERSPROC          glGenRenderbuffers = NULL;
extern PFNGLRENDERBUFFERSTORAGEPROC       glRenderbufferStorage = NULL;
extern PFNGLGETRENDERBUFFERPARAMETERIVPROC glGetRenderbufferParameteriv = NULL;
extern PFNGLISFRAMEBUFFERPROC             glIsFramebuffer = NULL;
extern PFNGLCHECKFRAMEBUFFERSTATUSPROC    glCheckFramebufferStatus = NULL;
extern PFNGLFRAMEBUFFERTEXTURE1DPROC      glFramebufferTexture1D = NULL;
extern PFNGLFRAMEBUFFERTEXTURE3DPROC      glFramebufferTexture3D = NULL;
extern PFNGLFRAMEBUFFERRENDERBUFFERPROC   glFramebufferRenderbuffer = NULL;
extern PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVPROC glGetFramebufferAttachmentParameteriv = NULL;
extern PFNGLUNIFORM1IVPROC                glUniform1iv = NULL;
extern PFNGLUNIFORM2IVPROC                glUniform2iv = NULL;
extern PFNGLUNIFORM3IVPROC                glUniform3iv = NULL;
extern PFNGLUNIFORM4IVPROC                glUniform4iv = NULL;
extern PFNGLUNIFORM2IPROC                 glUniform2i = NULL;
extern PFNGLUNIFORM3IPROC                 glUniform3i = NULL;
extern PFNGLUNIFORM4IPROC                 glUniform4i = NULL;
extern PFNGLUNIFORM1FVPROC                glUniform1fv = NULL;
extern PFNGLUNIFORM2FVPROC                glUniform2fv = NULL;
extern PFNGLUNIFORM3FVPROC                glUniform3fv = NULL;
extern PFNGLUNIFORM4FVPROC                glUniform4fv = NULL;
extern PFNGLUNIFORM1FPROC                 glUniform1f = NULL;
extern PFNGLUNIFORM2FPROC                 glUniform2f = NULL;
extern PFNGLUNIFORM3FPROC                 glUniform3f = NULL;
extern PFNGLUNIFORM4FPROC                 glUniform4f = NULL;
extern PFNGLUNIFORMMATRIX2FVPROC          glUniformMatrix2fv = NULL;
extern PFNGLUNIFORMMATRIX3FVPROC          glUniformMatrix3fv = NULL;

#endif // PLATFORM_WINDOWS
