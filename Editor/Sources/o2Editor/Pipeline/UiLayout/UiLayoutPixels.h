#pragma once

#include <vector>

#include "o2/Utils/Bitmap/Bitmap.h"
#include "o2/Utils/Types/Containers/Vector.h"
#include "o2/Utils/Types/Ref.h"
#include "o2/Utils/Types/String.h"

using namespace o2;

namespace Editor::UiLayout
{
    // Box in design pixels, top-left origin, y down
    struct Box
    {
        double x = 0, y = 0, w = 0, h = 0;

        Box() = default;
        Box(double x, double y, double w, double h): x(x), y(y), w(w), h(h) {}

        double Right() const { return x + w; }
        double Bottom() const { return y + h; }

        bool operator==(const Box& other) const { return x == other.x && y == other.y && w == other.w && h == other.h; }
    };

    // RGB triple in 0..255 doubles, as the measurement code passes colours around
    struct Rgb
    {
        double r = 0, g = 0, b = 0;

        Rgb() = default;
        Rgb(double r, double g, double b): r(r), g(g), b(b) {}

        double operator[](int i) const { return i == 0 ? r : i == 1 ? g : b; }
        double& operator[](int i) { return i == 0 ? r : i == 1 ? g : b; }
    };

    // JavaScript's Math.round: halves go up
    inline double JsRound(double v) { return std::floor(v + 0.5); }

    // A value stored into a Uint8ClampedArray: clamped, halves to even
    UInt8 Clamp8(double v);

    // --------------------------------------------------------------------------------------------
    // Decoded RGBA image the measurement code works on: rows top-down, 4 bytes per pixel, straight
    // (not premultiplied) alpha
    // --------------------------------------------------------------------------------------------
    class UiImage
    {
    public:
        int w = 0, h = 0;
        std::vector<UInt8> data;

    public:
        UiImage() = default;
        UiImage(int width, int height);

        // Returns an image filled with the colour
        static UiImage Blank(int width, int height, UInt8 r = 0, UInt8 g = 0, UInt8 b = 0, UInt8 a = 0);

        // Returns the pixels of an o2 bitmap (rows stored bottom-up)
        static UiImage FromBitmap(const Bitmap& bitmap);

        // Decodes PNG / JPEG bytes; false when they are not an image
        static bool Decode(const String& bytes, UiImage& out);

        // Returns an o2 bitmap of the pixels
        Ref<Bitmap> ToBitmap() const;

        // Returns the PNG bytes
        String Png() const;

        bool IsEmpty() const { return w <= 0 || h <= 0; }

        int Idx(int x, int y) const { return (y*w + x)*4; }

        const UInt8* At(int x, int y) const { return data.data() + Idx(x, y); }
        UInt8* At(int x, int y) { return data.data() + Idx(x, y); }

        // The pixels of an integer box (outside the image: transparent), at least one pixel
        UiImage Crop(int x, int y, int width, int height) const;

        // Resampled to the size: lanczos3 on premultiplied colours (sharp's default), or nearest
        UiImage Resize(int width, int height, bool nearest = false) const;

        // Draws src over this image at (x, y), source-over
        void Blit(const UiImage& src, int x, int y, double opacity = 1.0);

        // Returns the image composed onto an opaque colour
        UiImage Flatten(UInt8 r, UInt8 g, UInt8 b) const;
    };

    // Integer box covering the box
    Box IntBox(const Box& b);

    // The box grown by dx, dy on every side
    Box Grow(const Box& b, double dx, double dy);
    inline Box Grow(const Box& b, double d) { return Grow(b, d, d); }

    // The box clipped to [0, w] x [0, h]
    Box ClampBox(const Box& b, double w, double h);

    // Area of the intersection
    double Inter(const Box& a, const Box& b);

    // Intersection over union
    double Iou(const Box& a, const Box& b);

    // Share of inner that lies inside outer
    double Coverage(const Box& inner, const Box& outer);

    // True when inner lies inside outer, with slack
    bool Contains(const Box& outer, const Box& inner, double slack = 0.0);

    // #RRGGBB, upper case
    String HexOf(const Rgb& rgb);

    // Parses #RRGGBB / RRGGBB (any case, trimmed); false for anything else
    bool ParseHex(const String& text, Rgb& out);

    // Median of the values, 0 for none
    double Median(std::vector<double> values);

    // Per-channel median of the pixels on a box's one-pixel ring
    Rgb RingMedian(const UiImage& img, const Box& b);

    // Euclidean RGB distance
    double ColorDist(const Rgb& a, const Rgb& b);

    // Colour of a pixel
    inline Rgb RgbAt(const UiImage& img, int x, int y) { const UInt8* p = img.At(x, y); return Rgb(p[0], p[1], p[2]); }

    // Luminance plane 0..255
    std::vector<float> Luma(const UiImage& img);

    // Gradient magnitude of a luminance plane (central differences)
    std::vector<float> Gradient(const std::vector<float>& l, int w, int h);

    // One 4-connected component of a mask
    struct Component
    {
        int area = 0;
        Box box;
        int label = 0;
    };

    // 4-connected components of a mask: a label per pixel (0 = none) and each component's box and area
    void Components(const std::vector<UInt8>& mask, int w, int h, std::vector<int>& labels, std::vector<Component>& comps);

    // Box of the pixels with alpha at least min, false when there is none
    bool AlphaBounds(const UiImage& img, int min, Box& out);
}
