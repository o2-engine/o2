#include "o2/stdafx.h"

#ifdef PLATFORM_MAC
#import <Cocoa/Cocoa.h>
#import <MetalKit/MetalKit.h>

#include "o2/Render/Texture.h"
#include "o2/Utils/Debug/Log/LogStream.h"
#include "o2/Render/Mac/MetalWrappers.h"

namespace o2
{
    TextureBase::TextureBase():
        mImpl(mnew MTLTextureImpl())
    {}
    
    // TODO: TextureBase destructor to destroy mImpl
    
    bool Texture::PlatformCreate()
    {
        MTLTextureDescriptor *textureDescriptor = [[MTLTextureDescriptor alloc] init];
        textureDescriptor.pixelFormat = MTLPixelFormatBGRA8Unorm;
        textureDescriptor.width = mSize.x;
        textureDescriptor.height = mSize.y;
        
        if (usage == Usage::RenderTarget)
            textureDescriptor.usage = MTLTextureUsageRenderTarget|MTLTextureUsageShaderRead;
        
        mImpl->texture = [RenderDevice::device newTextureWithDescriptor:textureDescriptor];

        return true;
    }

    void Texture::PlatformDestroy()
    {
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
        // TODO
    }
}

#endif //PLATFORM_MAC
