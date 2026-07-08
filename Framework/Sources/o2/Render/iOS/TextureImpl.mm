#include "o2/stdafx.h"

#ifdef PLATFORM_IOS
#import <MetalKit/MetalKit.h>

#include "o2/Render/Texture.h"
#include "o2/Utils/Debug/Log/LogStream.h"
#include "o2/Render/iOS/MetalWrappers.h"

namespace o2
{
    namespace
    {
        // Builds an MTLSamplerState from o2 wrap+filter modes
        id<MTLSamplerState> BuildSamplerState(Texture::Wrap wrap, Texture::Filter filter)
        {
            MTLSamplerAddressMode address = wrap == Texture::Wrap::Repeat
                ? MTLSamplerAddressModeRepeat
                : MTLSamplerAddressModeClampToEdge;

            MTLSamplerMinMagFilter minMag = filter == Texture::Filter::Nearest
                ? MTLSamplerMinMagFilterNearest
                : MTLSamplerMinMagFilterLinear;

            MTLSamplerDescriptor* desc = [[MTLSamplerDescriptor alloc] init];
            desc.sAddressMode = address;
            desc.tAddressMode = address;
            desc.minFilter = minMag;
            desc.magFilter = minMag;

            return [RenderDevice::device newSamplerStateWithDescriptor:desc];
        }
    }

    TextureBase::TextureBase():
        mImpl(mnew MTLTextureImpl())
    {}

    // TODO: TextureBase destructor to destroy mImpl

    bool Texture::PlatformCreate()
    {
        if (!mImpl)
            mImpl = mnew MTLTextureImpl();

        MTLTextureDescriptor *textureDescriptor = [[MTLTextureDescriptor alloc] init];
        textureDescriptor.pixelFormat = mUsage == Usage::RenderTarget ? RenderDevice::view.colorPixelFormat : MTLPixelFormatRGBA8Unorm;
        textureDescriptor.width = mSize.x;
        textureDescriptor.height = mSize.y;

        if (mUsage == Usage::RenderTarget)
            textureDescriptor.usage = MTLTextureUsageRenderTarget|MTLTextureUsageShaderRead;

        mImpl->texture = [RenderDevice::device newTextureWithDescriptor:textureDescriptor];
        mImpl->samplerState = BuildSamplerState(mWrap, mFilter);

        return mImpl->texture != nil;
    }

    void Texture::PlatformDestroy()
    {
        if (mImpl)
        {
            mImpl->texture = nil;
            mImpl->samplerState = nil;
            mImpl->depthTexture = nil;
        }
    }

    void Texture::PlatformUploadData(const Vec2I& size, Byte* data, TextureFormat format)
    {
        NSUInteger bytesPerRow = 4 * mSize.x;
        MTLRegion region = { { 0, 0, 0 }, { (UInt)size.x, (UInt)size.y, 1 } };
        [mImpl->texture replaceRegion:region
                          mipmapLevel:0
                            withBytes:data
                          bytesPerRow:bytesPerRow];
    }

    void Texture::PlatformUploadRegionData(const Vec2I& offset, const Vec2I& size, Byte* data, TextureFormat format)
    {
        NSUInteger bytesPerRow = 4 * size.x;
        MTLRegion region = { { (UInt)offset.x, (UInt)offset.y, 0 }, { (UInt)size.x, (UInt)size.y, 1 } };
        [mImpl->texture replaceRegion:region
                          mipmapLevel:0
                            withBytes:data
                          bytesPerRow:bytesPerRow];
    }

    void Texture::Copy(const Texture& from, const RectI& rect)
    {}

    void Texture::PlatformGetData(Byte* data)
    {}

    void Texture::PlatformSetFilter()
    {
        if (mImpl)
            mImpl->samplerState = BuildSamplerState(mWrap, mFilter);
    }

    void Texture::PlatformSetWrap()
    {
        if (mImpl)
            mImpl->samplerState = BuildSamplerState(mWrap, mFilter);
    }
}

#endif //PLATFORM_IOS
