#include "o2/stdafx.h"

#ifdef PLATFORM_WASM

#include "o2/Render/Render.h"
#include "o2/Render/Texture.h"
#include "o2/Utils/Debug/Log/LogStream.h"

namespace o2
{
    static GLint MapTextureFormat(TextureFormat format)
    {
        switch (format)
        {
            case TextureFormat::R8G8B8A8: return GL_RGBA;
            default:                      return GL_RGBA; // WebGL2 has no DXT5; fall back to RGBA
        }
    }

    bool Texture::PlatformCreate()
    {
        auto prevTextureHandle = o2Render.mCurrentDrawTexture ? o2Render.mCurrentDrawTexture->mHandle : 0;

        glGenTextures(1, &mHandle);
        glBindTexture(GL_TEXTURE_2D, mHandle);
        GL_CHECK_ERROR();

        GLint texFormat = MapTextureFormat(mFormat);
        glTexImage2D(GL_TEXTURE_2D, 0, texFormat, (GLsizei)mSize.x, (GLsizei)mSize.y, 0, texFormat, GL_UNSIGNED_BYTE, NULL);
        GL_CHECK_ERROR();

        GLint wrap = mWrap == Wrap::Repeat ? GL_REPEAT : GL_CLAMP_TO_EDGE;
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        GL_CHECK_ERROR();

        if (mUsage == Usage::RenderTarget)
        {
            glGenFramebuffers(1, &mFrameBuffer);
            glBindFramebuffer(GL_FRAMEBUFFER, mFrameBuffer);

            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mHandle, 0);

            if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            {
                GLenum glError = glGetError();
                o2Render.mLog->Error((String)"Failed to create GL frame buffer object! GL Error " + (int)glError + " " +
                                     GetGLErrorDesc(glError));
                glBindTexture(GL_TEXTURE_2D, prevTextureHandle);
                return false;
            }

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            GL_CHECK_ERROR();
        }

        glBindTexture(GL_TEXTURE_2D, prevTextureHandle);
        return true;
    }

    void Texture::PlatformDestroy()
    {
        if (mUsage == Usage::RenderTarget && mFrameBuffer)
            glDeleteFramebuffers(1, &mFrameBuffer);

        if (mHandle)
            glDeleteTextures(1, &mHandle);
    }

    void Texture::PlatformUploadData(const Vec2I& size, Byte* data, TextureFormat format)
    {
        auto prevTextureHandle = o2Render.mCurrentDrawTexture ? o2Render.mCurrentDrawTexture->mHandle : 0;

        glBindTexture(GL_TEXTURE_2D, mHandle);

        GLint texFormat = MapTextureFormat(format);
        glTexImage2D(GL_TEXTURE_2D, 0, texFormat, (GLsizei)size.x, (GLsizei)size.y, 0, texFormat, GL_UNSIGNED_BYTE, data);
        GL_CHECK_ERROR();

        glBindTexture(GL_TEXTURE_2D, prevTextureHandle);
    }

    void Texture::PlatformUploadRegionData(const Vec2I& offset, const Vec2I& size, Byte* data, TextureFormat format)
    {
        auto prevTextureHandle = o2Render.mCurrentDrawTexture ? o2Render.mCurrentDrawTexture->mHandle : 0;
        glBindTexture(GL_TEXTURE_2D, mHandle);

        GLint texFormat = MapTextureFormat(format);
        glTexSubImage2D(GL_TEXTURE_2D, 0, offset.x, offset.y, size.x, size.y, texFormat, GL_UNSIGNED_BYTE, data);
        GL_CHECK_ERROR();

        glBindTexture(GL_TEXTURE_2D, prevTextureHandle);
    }

    void Texture::Copy(const Texture& from, const RectI& rect)
    {
        auto prevTextureHandle = o2Render.mCurrentDrawTexture ? o2Render.mCurrentDrawTexture->mHandle : 0;
        glBindTexture(GL_TEXTURE_2D, from.mHandle);

        GLint texFormat = MapTextureFormat(mFormat);
        glCopyTexImage2D(GL_TEXTURE_2D, 0, texFormat, rect.left, rect.top, rect.Width(), rect.Height(), 0);
        glBindTexture(GL_TEXTURE_2D, prevTextureHandle);
    }

    void Texture::PlatformGetData(Byte* data)
    {
        // WebGL has no glGetTexImage. Readback via FBO + glReadPixels
        GLuint tmpFbo = 0;
        glGenFramebuffers(1, &tmpFbo);
        glBindFramebuffer(GL_FRAMEBUFFER, tmpFbo);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mHandle, 0);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE)
            glReadPixels(0, 0, mSize.x, mSize.y, GL_RGBA, GL_UNSIGNED_BYTE, data);
        else
            o2Render.mLog->Error("Texture::PlatformGetData: framebuffer incomplete for readback");

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDeleteFramebuffers(1, &tmpFbo);
    }

    void Texture::PlatformSetFilter()
    {
        GLint type = GL_LINEAR;
        if (mFilter == Filter::Nearest)
            type = GL_NEAREST;

        auto prevTextureHandle = o2Render.mCurrentDrawTexture ? o2Render.mCurrentDrawTexture->mHandle : 0;
        o2Render.DrawPrimitives();

        glBindTexture(GL_TEXTURE_2D, mHandle);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, type);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, type);

        glBindTexture(GL_TEXTURE_2D, prevTextureHandle);
        GL_CHECK_ERROR();
    }

    void Texture::PlatformSetWrap()
    {
        GLint wrap = mWrap == Wrap::Repeat ? GL_REPEAT : GL_CLAMP_TO_EDGE;

        auto prevTextureHandle = o2Render.mCurrentDrawTexture ? o2Render.mCurrentDrawTexture->mHandle : 0;
        o2Render.DrawPrimitives();

        glBindTexture(GL_TEXTURE_2D, mHandle);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap);

        glBindTexture(GL_TEXTURE_2D, prevTextureHandle);
        GL_CHECK_ERROR();
    }
}

#endif // PLATFORM_WASM
