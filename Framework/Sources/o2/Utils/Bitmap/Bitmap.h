#pragma once

#include "o2/Utils/Math/Vector2.h"
#include "o2/Utils/Property.h"
#include "o2/Utils/Types/CommonTypes.h"
#include "o2/Utils/Types/Ref.h"
#include "o2/Utils/Types/String.h"

namespace o2
{
    // ------------------------------------------------------
    // Image. Containing formatted pixel data, size, filename
    // ------------------------------------------------------
    class Bitmap: public RefCounterable
    {
    public:
        enum class ImageType { Auto = 0, Png };

    public:
        PROPERTIES(Bitmap);
        GETTER(UInt8*, data, GetData);          // Data getter
        GETTER(Vec2I, size, GetSize);           // Image size getter
        GETTER(PixelFormat, format, GetFormat); // Data format getter

    public:
        // Default constructor
        Bitmap();

        // Constructor with format and size
        Bitmap(PixelFormat format, const Vec2I& size);

        // Constructor that loads from file
        Bitmap(const String& fileName, ImageType type = ImageType::Auto);

        // Copy-constructor
        Bitmap(const Bitmap& other);

        // Virtual destructor
        virtual ~Bitmap();

        // Assign operator
        Bitmap& operator=(const Bitmap& other);

        // Returns copy of this
        Bitmap* Clone() const;

        // Creates image with specified format
        void Create(PixelFormat format, const Vec2I& size);

        // Loading image from file
        bool Load(const String& fileName, ImageType type = ImageType::Auto);

        // Loading image from a memory buffer, e.g. a downloaded response body
        bool LoadFromMemory(const UInt8* data, UInt size, ImageType type = ImageType::Auto);

        // Saving image to file
        bool Save(const String& fileName, ImageType type) const;

        // Clearing image with color
        void Clear(const Color4& color);

        // Returns data
        UInt8* GetData();

        // Returns const data
        const UInt8* GetData() const;

        // Returns size of image
        Vec2I GetSize() const;

        // Returns pixel format
        PixelFormat GetFormat() const;

        // Return file name
        const String& GetFilename() const;

        // Copy image to position
        void CopyImage(const Bitmap& img, const Vec2I& position = Vec2I(), const RectI& imgSrc = RectI());

        // Blends images by alpha
        void BlendImage(const Bitmap& img, const Vec2I& position = Vec2I(), const RectI& imgSrc = RectI());

        // Sets images pixels colors
        void Colorise(const Color4& color);

        // Sets image pixels by gradient
        void GradientByAlpha(const Color4& color1, const Color4& color4, float angle = 0, float size = 0,
                             Vec2F origin = Vec2F());

        // Fill image with color
        void Fill(const Color4& color);

        // Fills rect with color
        void FillRect(int rtLeft, int rtTop, int rtRight, int rtBottom, const Color4& color);

        // Apply blur effect
        void Blur(float radius);

        // Apply outline effect
        void Outline(float radius, const Color4& color, int threshold = 100);

        // Returns a copy scaled to newSize with bilinear filtering
        Ref<Bitmap> Resized(const Vec2I& newSize) const;

    protected:
        PixelFormat mFormat;   // Image format
        UInt8*      mData;     // Data array
        Vec2I       mSize;     // Size of image, in pixels
        String      mFilename; // File name. Empty if no file
    };
}
// --- META ---

PRE_ENUM_META(o2::Bitmap::ImageType);
// --- END META ---
