#include "o2/stdafx.h"
#include "VectorRasterizer.h"

#include "o2/Render/VectorGraphics/VectorTessellator.h"
#include "o2/Utils/Math/Math.h"

#include <cmath>

namespace o2
{
    namespace
    {
        struct RasterVertex
        {
            double x, y;
            double channels[4];
        };

        // Edge function with ends in fixed order, so both triangles sharing the edge get exactly opposite values
        double EdgeValue(const RasterVertex& a, const RasterVertex& b, double x, double y)
        {
            if (b.x < a.x || (b.x == a.x && b.y < a.y))
                return -((a.x - b.x)*(y - b.y) - (a.y - b.y)*(x - b.x));

            return (b.x - a.x)*(y - a.y) - (b.y - a.y)*(x - a.x);
        }

        bool IsTopLeft(const RasterVertex& a, const RasterVertex& b)
        {
            double dx = b.x - a.x, dy = b.y - a.y;
            return (dy == 0.0 && dx > 0.0) || dy < 0.0;
        }

        UInt8 ToByte(double value)
        {
            return (UInt8)Math::Clamp((int)floor(value*255.0 + 0.5), 0, 255);
        }
    }

    void VectorRasterizer::Rasterize(const VectorMesh& mesh, Bitmap& bitmap, float scale /*= 1.0f*/,
                                     const Vec2F& offset /*= Vec2F()*/)
    {
        if (bitmap.GetFormat() != PixelFormat::R8G8B8A8 || !bitmap.GetData())
            return;

        Vec2I size = bitmap.GetSize();
        UInt8* data = bitmap.GetData();

        for (size_t i = 0; i + 2 < (size_t)mesh.indexes.Count(); i += 3)
        {
            RasterVertex v[3];
            for (int k = 0; k < 3; k++)
            {
                VertexIndex idx = mesh.indexes[i + k];
                Color32Bit color = mesh.colors[idx];

                v[k].x = (double)mesh.positions[idx].x*scale + offset.x;
                v[k].y = (double)mesh.positions[idx].y*scale + offset.y;
                for (int channel = 0; channel < 4; channel++)
                    v[k].channels[channel] = (double)((color >> (channel*8)) & 0xFF)/255.0;
            }

            double area = (v[1].x - v[0].x)*(v[2].y - v[0].y) - (v[1].y - v[0].y)*(v[2].x - v[0].x);
            if (area == 0.0)
                continue;

            if (area < 0.0)
            {
                Math::Swap(v[1], v[2]);
                area = -area;
            }

            int minX = Math::Max((int)floor(Math::Min(v[0].x, Math::Min(v[1].x, v[2].x)) - 0.5), 0);
            int maxX = Math::Min((int)ceil(Math::Max(v[0].x, Math::Max(v[1].x, v[2].x)) - 0.5), size.x - 1);
            int minY = Math::Max((int)floor(Math::Min(v[0].y, Math::Min(v[1].y, v[2].y)) - 0.5), 0);
            int maxY = Math::Min((int)ceil(Math::Max(v[0].y, Math::Max(v[1].y, v[2].y)) - 0.5), size.y - 1);

            bool topLeft[3] = { IsTopLeft(v[1], v[2]), IsTopLeft(v[2], v[0]), IsTopLeft(v[0], v[1]) };

            for (int py = minY; py <= maxY; py++)
            {
                UInt8* row = data + (size_t)(size.y - 1 - py)*size.x*4;
                double y = py + 0.5;

                for (int px = minX; px <= maxX; px++)
                {
                    double x = px + 0.5;
                    double weights[3] = { EdgeValue(v[1], v[2], x, y), EdgeValue(v[2], v[0], x, y),
                                          EdgeValue(v[0], v[1], x, y) };

                    bool covered = true;
                    for (int k = 0; k < 3; k++)
                        covered = covered && (weights[k] > 0.0 || (weights[k] == 0.0 && topLeft[k]));

                    if (!covered)
                        continue;

                    double source[4];
                    for (int channel = 0; channel < 4; channel++)
                    {
                        source[channel] = (weights[0]*v[0].channels[channel] + weights[1]*v[1].channels[channel] +
                                           weights[2]*v[2].channels[channel])/area;
                    }

                    UInt8* pixel = row + px*4;
                    double alpha = source[3];
                    for (int channel = 0; channel < 3; channel++)
                        pixel[channel] = ToByte(source[channel]*alpha + (double)pixel[channel]/255.0*(1.0 - alpha));

                    pixel[3] = ToByte(alpha + (double)pixel[3]/255.0*(1.0 - alpha));
                }
            }
        }
    }

    Ref<Bitmap> VectorRasterizer::Rasterize(const VectorMesh& mesh, float scale /*= 1.0f*/,
                                            const Color4& background /*= Color4(0, 0, 0, 0)*/)
    {
        Vec2I size(Math::Max((int)ceil(mesh.size.x*scale - 1e-3f), 1),
                   Math::Max((int)ceil(mesh.size.y*scale - 1e-3f), 1));

        Ref<Bitmap> bitmap = mmake<Bitmap>(PixelFormat::R8G8B8A8, size);

        UInt8* data = bitmap->GetData();
        for (size_t i = 0; i < (size_t)size.x*size.y; i++)
        {
            data[i*4] = (UInt8)background.r;
            data[i*4 + 1] = (UInt8)background.g;
            data[i*4 + 2] = (UInt8)background.b;
            data[i*4 + 3] = (UInt8)background.a;
        }

        Rasterize(mesh, *bitmap, scale);
        return bitmap;
    }

    Ref<Bitmap> VectorRasterizer::Rasterize(const VectorImage& image, float scale /*= 1.0f*/,
                                            const Color4& background /*= Color4(0, 0, 0, 0)*/,
                                            bool antialiasing /*= true*/)
    {
        VectorTessellationParams params;
        params.pixelScale = Vec2F(scale, scale);
        params.antialiasing = antialiasing;

        VectorMesh mesh;
        VectorTessellator::Tessellate(image, mesh, params);
        return Rasterize(mesh, scale, background);
    }

    void VectorRasterizer::Unpremultiply(Bitmap& bitmap)
    {
        if (bitmap.GetFormat() != PixelFormat::R8G8B8A8 || !bitmap.GetData())
            return;

        Vec2I size = bitmap.GetSize();
        UInt8* data = bitmap.GetData();
        for (size_t i = 0; i < (size_t)size.x*size.y; i++)
        {
            UInt8* pixel = data + i*4;
            if (pixel[3] == 0 || pixel[3] == 255)
                continue;

            for (int channel = 0; channel < 3; channel++)
                pixel[channel] = (UInt8)Math::Min(((int)pixel[channel]*255 + pixel[3]/2)/pixel[3], 255);
        }
    }

    Color4 VectorRasterizer::GetPixel(const Bitmap& bitmap, int x, int y)
    {
        Vec2I size = bitmap.GetSize();
        if (x < 0 || y < 0 || x >= size.x || y >= size.y || bitmap.GetFormat() != PixelFormat::R8G8B8A8)
            return Color4(0, 0, 0, 0);

        const UInt8* pixel = bitmap.GetData() + ((size_t)(size.y - 1 - y)*size.x + x)*4;
        return Color4((int)pixel[0], (int)pixel[1], (int)pixel[2], (int)pixel[3]);
    }
}
