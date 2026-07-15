#include "o2/stdafx.h"

#ifdef PLATFORM_ANDROID

#include "o2/Render/Render.h"
#include "o2/Render/Texture.h"
#include "o2/Utils/Debug/Log/LogStream.h"

namespace o2
{
    struct GLTextureFormat
    {
        GLint  internalFormat;
        GLenum format;
        GLenum type;
    };

#ifndef GL_COMPRESSED_RGBA_S3TC_DXT1_EXT
#define GL_COMPRESSED_RGBA_S3TC_DXT1_EXT 0x83F1
#endif
#ifndef GL_COMPRESSED_RGBA_S3TC_DXT5_EXT
#define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT 0x83F3
#endif
#ifndef GL_COMPRESSED_RGBA_BPTC_UNORM
#define GL_COMPRESSED_RGBA_BPTC_UNORM 0x8E8C
#endif
#ifndef GL_COMPRESSED_RGBA_ASTC_4x4_KHR
#define GL_COMPRESSED_RGBA_ASTC_4x4_KHR 0x93B0
#endif

    static GLTextureFormat MapTextureFormat(TextureFormat format)
    {
        switch (format)
        {
            case TextureFormat::R16G16B16A16F:
                // Half float targets are only used by the deferred pipeline, which requires an ES 3 context
                if (GLES3::available)
                    return { GL_RGBA16F, GL_RGBA, GL_HALF_FLOAT };
                return { GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE };

            case TextureFormat::R8G8B8A8: return { GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE };
            default:                      return { GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE }; // GLES has no DXT5; fall back to RGBA
        }
    }

    bool Texture::PlatformCreate()
    {
        auto prevTextureHandle = o2Render.mCurrentDrawTexture ? o2Render.mCurrentDrawTexture->mHandle : 0;

        glGenTextures(1, &mHandle);
        glBindTexture(GL_TEXTURE_2D, mHandle);
        GL_CHECK_ERROR();

        GLTextureFormat texFormat = MapTextureFormat(mFormat);
        glTexImage2D(GL_TEXTURE_2D, 0, texFormat.internalFormat, (GLsizei)mSize.x, (GLsizei)mSize.y, 0,
                     texFormat.format, texFormat.type, NULL);
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

            glGenRenderbuffers(1, &mDepthRenderBuffer);
            glBindRenderbuffer(GL_RENDERBUFFER, mDepthRenderBuffer);
            glRenderbufferStorage(GL_RENDERBUFFER, GLES3::available ? GL_DEPTH_COMPONENT24 : GL_DEPTH_COMPONENT16,
                                  (GLsizei)mSize.x, (GLsizei)mSize.y);
            glBindRenderbuffer(GL_RENDERBUFFER, 0);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, mDepthRenderBuffer);

            if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            {
                GLenum glError = glGetError();
                o2Render.mLog->Error((String)"Failed to create GL frame buffer object! GL Error " + (int)glError + " " +
                                     GetGLErrorDesc(glError));
                glBindFramebuffer(GL_FRAMEBUFFER, 0);
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

        if (mDepthRenderBuffer)
            glDeleteRenderbuffers(1, &mDepthRenderBuffer);

        if (mHandle)
            glDeleteTextures(1, &mHandle);
    }

    void Texture::PlatformUploadData(const Vec2I& size, Byte* data, TextureFormat format)
    {
        auto prevTextureHandle = o2Render.mCurrentDrawTexture ? o2Render.mCurrentDrawTexture->mHandle : 0;

        glBindTexture(GL_TEXTURE_2D, mHandle);

        if (format == TextureFormat::ASTC4x4)
        {
            int dataSize = ((size.x + 3)/4)*((size.y + 3)/4)*16;
            glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA_ASTC_4x4_KHR,
                                   (GLsizei)size.x, (GLsizei)size.y, 0, dataSize, data);
            GL_CHECK_ERROR();
        }
        else if (Texture::IsFormatCompressed(format))
        {
            // GLES devices have no BC support: use ASTC4x4 compression for the Android platform
            o2Render.mLog->Error("BC texture formats are not supported on Android, use ASTC4x4");
        }
        else
        {
            GLTextureFormat texFormat = MapTextureFormat(format);
            glTexImage2D(GL_TEXTURE_2D, 0, texFormat.internalFormat, (GLsizei)size.x, (GLsizei)size.y, 0,
                         texFormat.format, texFormat.type, data);
            GL_CHECK_ERROR();
        }

        glBindTexture(GL_TEXTURE_2D, prevTextureHandle);
    }

    void Texture::PlatformUploadRegionData(const Vec2I& offset, const Vec2I& size, Byte* data, TextureFormat format)
    {
        auto prevTextureHandle = o2Render.mCurrentDrawTexture ? o2Render.mCurrentDrawTexture->mHandle : 0;
        glBindTexture(GL_TEXTURE_2D, mHandle);

        GLTextureFormat texFormat = MapTextureFormat(format);
        glTexSubImage2D(GL_TEXTURE_2D, 0, offset.x, offset.y, size.x, size.y, texFormat.format, texFormat.type, data);
        GL_CHECK_ERROR();

        glBindTexture(GL_TEXTURE_2D, prevTextureHandle);
    }

    void Texture::Copy(const Texture& from, const RectI& rect)
    {
        auto prevTextureHandle = o2Render.mCurrentDrawTexture ? o2Render.mCurrentDrawTexture->mHandle : 0;
        glBindTexture(GL_TEXTURE_2D, from.mHandle);

        GLTextureFormat texFormat = MapTextureFormat(mFormat);
        glCopyTexImage2D(GL_TEXTURE_2D, 0, texFormat.internalFormat, rect.left, rect.top, rect.Width(), rect.Height(), 0);
        glBindTexture(GL_TEXTURE_2D, prevTextureHandle);
    }

    void Texture::PlatformGetData(Byte* data)
    {
        // GLES2 has no glGetTexImage. Readback via FBO + glReadPixels
        GLuint tmpFbo = 0;
        glGenFramebuffers(1, &tmpFbo);
        glBindFramebuffer(GL_FRAMEBUFFER, tmpFbo);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mHandle, 0);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE)
        {
            if (mFormat == TextureFormat::R16G16B16A16F && GLES3::available)
            {
                // Float target: read as floats, convert to 8-bit bitmap channels with clamping to [0, 1]
                size_t channelsCount = (size_t)mSize.x*(size_t)mSize.y*4;
                Vector<float> floatData;
                floatData.resize((int)channelsCount);
                glReadPixels(0, 0, mSize.x, mSize.y, GL_RGBA, GL_FLOAT, floatData.data());

                for (size_t i = 0; i < channelsCount; i++)
                    data[i] = (Byte)(Math::Clamp01(floatData[(int)i])*255.0f);
            }
            else
                glReadPixels(0, 0, mSize.x, mSize.y, GL_RGBA, GL_UNSIGNED_BYTE, data);
        }
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

#endif // PLATFORM_ANDROID
