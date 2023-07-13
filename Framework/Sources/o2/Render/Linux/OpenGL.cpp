#include "o2/stdafx.h"

#if defined(PLATFORM_LINUX) && !defined(O2_RENDER_GLES2)

#include "OpenGL.h"
#include "o2/Utils/Debug/Log/LogStream.h"

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
// Returns address of function
auto GetSafeWGLProcAddress(const char* id, o2::LogStream* log)
{
    auto res = glXGetProcAddress(reinterpret_cast<const GLubyte*>(id));
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
    glUniform1i = (PFNGLUNIFORM1IPROC)GetSafeWGLProcAddress("glUniform1i", log);
    glBlendFuncSeparate = (PFNGLBLENDFUNCSEPARATEPROC)GetSafeWGLProcAddress("glBlendFuncSeparate", log);
    glBufferSubData = (PFNGLBUFFERSUBDATAPROC)GetSafeWGLProcAddress("glBufferSubData", log);
}

PFNGLGENFRAMEBUFFERSEXTPROC        glGenFramebuffersEXT = NULL;
PFNGLBINDFRAMEBUFFEREXTPROC        glBindFramebufferEXT = NULL;
PFNGLFRAMEBUFFERTEXTUREPROC        glFramebufferTexture = NULL;
PFNGLDRAWBUFFERSPROC               glDrawBuffers = NULL;
PFNGLDELETEBUFFERSPROC             glDeleteBuffers = NULL;
PFNGLDELETEFRAMEBUFFERSPROC        glDeleteFramebuffersEXT = NULL;
PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC glCheckFramebufferStatusEXT = NULL;
PFNGLGENBUFFERSPROC                glGenBuffers = NULL;
PFNGLBINDBUFFERPROC                glBindBuffer = NULL;
PFNGLBUFFERDATAPROC                glBufferData = NULL;
PFNGLCREATESHADERPROC              glCreateShader = NULL;
PFNGLDELETESHADERPROC              glDeleteShader = NULL;
PFNGLSHADERSOURCEPROC              glShaderSource = NULL;
PFNGLCOMPILESHADERPROC             glCompileShader = NULL;
PFNGLGETSHADERIVPROC               glGetShaderiv = NULL;
PFNGLGETSHADERINFOLOGPROC          glGetShaderInfoLog = NULL;
PFNGLCREATEPROGRAMPROC             glCreateProgram = NULL;
PFNGLATTACHSHADERPROC              glAttachShader = NULL;
PFNGLLINKPROGRAMPROC               glLinkProgram = NULL;
PFNGLGETPROGRAMIVPROC              glGetProgramiv = NULL;
PFNGLGETPROGRAMINFOLOGPROC         glGetProgramInfoLog = NULL;
PFNGLUSEPROGRAMPROC                glUseProgram = NULL;
PFNGLGETUNIFORMLOCATIONPROC        glGetUniformLocation = NULL;
PFNGLGETATTRIBLOCATIONPROC         glGetAttribLocation = NULL;
PFNGLDELETEPROGRAMPROC             glDeleteProgram = NULL;
PFNGLVERTEXATTRIBPOINTERPROC       glVertexAttribPointer = NULL;
PFNGLENABLEVERTEXATTRIBARRAYPROC   glEnableVertexAttribArray = NULL;
PFNGLUNIFORMMATRIX4FVPROC          glUniformMatrix4fv = NULL;
PFNGLUNIFORM1IPROC                 glUniform1i = NULL;
PFNGLBLENDFUNCSEPARATEPROC         glBlendFuncSeparate = NULL;
PFNGLBUFFERSUBDATAPROC             glBufferSubData = NULL;

#endif // PLATFORM_LINUX
