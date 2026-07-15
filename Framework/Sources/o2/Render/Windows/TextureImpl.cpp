#include "o2/stdafx.h"

#ifdef PLATFORM_WINDOWS
#include "o2/Render/Texture.h"
#include "o2/Utils/Debug/Log/LogStream.h"

namespace o2
{
#ifndef GL_COMPRESSED_RGBA_S3TC_DXT1_EXT
#define GL_COMPRESSED_RGBA_S3TC_DXT1_EXT 0x83F1
#endif
#ifndef GL_COMPRESSED_RGBA_BPTC_UNORM
#define GL_COMPRESSED_RGBA_BPTC_UNORM 0x8E8C
#endif
#ifndef GL_COMPRESSED_RGBA_ASTC_4x4_KHR
#define GL_COMPRESSED_RGBA_ASTC_4x4_KHR 0x93B0
#endif

    Map<TextureFormat, GLint> formatMap =
    {
        { TextureFormat::R8G8B8A8, GL_RGBA },
        { TextureFormat::DXT1, GL_COMPRESSED_RGBA_S3TC_DXT1_EXT },
        { TextureFormat::DXT5, GL_COMPRESSED_RGBA_S3TC_DXT5_EXT },
        { TextureFormat::BC7, GL_COMPRESSED_RGBA_BPTC_UNORM },
        { TextureFormat::ASTC4x4, GL_COMPRESSED_RGBA_ASTC_4x4_KHR }
    };

    bool Texture::PlatformCreate()
    {
        auto prevTextureHandle = o2Render.mCurrentDrawTexture ? o2Render.mCurrentDrawTexture->mHandle : 0;

        glGenTextures(1, &mHandle);
        glBindTexture(GL_TEXTURE_2D, mHandle);
        GL_CHECK_ERROR();

        GLint texFormat = formatMap[format];
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
            glGenFramebuffersEXT(1, &mFrameBuffer);
            glBindFramebufferEXT(GL_FRAMEBUFFER, mFrameBuffer);

            glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, mHandle, 0);

            GLenum DrawBuffers[2] = { GL_COLOR_ATTACHMENT0 };
            glDrawBuffers(1, DrawBuffers);

            if (glCheckFramebufferStatusEXT(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            {
                GLenum glError = glGetError();

                o2Render.mLog->Error("Failed to create GL frame buffer object! GL Error %i %cs", glError,
                                     GetGLErrorDesc(glError));

                glBindTexture(GL_TEXTURE_2D, prevTextureHandle);

                return false;
            }

            glBindFramebufferEXT(GL_FRAMEBUFFER, 0);
            GL_CHECK_ERROR();
        }

        glBindTexture(GL_TEXTURE_2D, prevTextureHandle);

        return true;
    }

    void Texture::PlatformDestroy()
    {
        if (mUsage == Usage::RenderTarget)
            glDeleteFramebuffersEXT(1, &mFrameBuffer);

        glDeleteTextures(1, &mHandle);
    }

    void Texture::PlatformUploadData(const Vec2I& size, Byte* data, TextureFormat format)
    {
        auto prevTextureHandle = o2Render.mCurrentDrawTexture ? o2Render.mCurrentDrawTexture->mHandle : 0;

        glBindTexture(GL_TEXTURE_2D, mHandle);

        GLint texFormat = formatMap[format];

        if (Texture::IsFormatCompressed(format))
        {
            int blockSize = Texture::FormatBlockSize(format);
            int dataSize = ((size.x + 3) / 4) * ((size.y + 3) / 4) * blockSize;

            glCompressedTexImage2D(GL_TEXTURE_2D, 0, texFormat, size.x, size.y, 0, dataSize, data);
        }
        else
        {
            glTexImage2D(GL_TEXTURE_2D, 0, texFormat, (GLsizei)size.x, (GLsizei)size.y, 0, texFormat, GL_UNSIGNED_BYTE, data);
        }

        GL_CHECK_ERROR();

        glBindTexture(GL_TEXTURE_2D, prevTextureHandle);
    }

    void Texture::PlatformUploadRegionData(const Vec2I& offset, const Vec2I& size, Byte* data, TextureFormat format)
    {
        auto prevTextureHandle = o2Render.mCurrentDrawTexture ? o2Render.mCurrentDrawTexture->mHandle : 0;
        glBindTexture(GL_TEXTURE_2D, mHandle);

        GLint texFormat = formatMap[format];

        glTexSubImage2D(GL_TEXTURE_2D, 0, offset.x, offset.y, size.x, size.y, texFormat, GL_UNSIGNED_BYTE,
                        data);

        GL_CHECK_ERROR();

        glBindTexture(GL_TEXTURE_2D, prevTextureHandle);
    }

    void Texture::Copy(const Texture& from, const RectI& rect)
    {
        auto prevTextureHandle = o2Render.mCurrentDrawTexture ? o2Render.mCurrentDrawTexture->mHandle : 0;
        glBindTexture(GL_TEXTURE_2D, from.mHandle);

        GLint texFormat = formatMap[format];

        glCopyTexImage2D(GL_TEXTURE_2D, 0, texFormat, rect.left, rect.top, rect.Width(), rect.Height(), 0);
        glBindTexture(GL_TEXTURE_2D, prevTextureHandle);
    }

    void Texture::PlatformGetData(Byte* data)
    {
        auto prevTextureHandle = o2Render.mCurrentDrawTexture ? o2Render.mCurrentDrawTexture->mHandle : 0;
        glBindTexture(GL_TEXTURE_2D, mHandle);
        glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        glBindTexture(GL_TEXTURE_2D, prevTextureHandle);
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

#endif //PLATFORM_WINDOWS
