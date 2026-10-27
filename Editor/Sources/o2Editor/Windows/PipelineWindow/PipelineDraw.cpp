#include "o2Editor/stdafx.h"
#include "PipelineDraw.h"

#include "o2/Render/Mesh.h"
#include "o2/Render/Render.h"
#include "o2/Render/Texture.h"
#include "o2/Utils/Bitmap/Bitmap.h"

#include <map>
#include <tuple>

namespace Editor
{
    namespace PipelineDraw
    {
        Gradient::Gradient(const Color4& flat)
        {
            stops.Add({ 0.0f, flat });
        }

        Gradient::Gradient(const Color4& top, const Color4& bottom)
        {
            stops.Add({ 0.0f, top });
            stops.Add({ 1.0f, bottom });
        }

        Gradient::Gradient(const Color4& top, float midAt, const Color4& mid, const Color4& bottom)
        {
            stops.Add({ 0.0f, top });
            stops.Add({ midAt, mid });
            stops.Add({ 1.0f, bottom });
        }

        Color4 Gradient::At(float t) const
        {
            if (stops.IsEmpty())
                return Color4::White();

            if (stops.Count() == 1 || t <= stops[0].first)
                return stops[0].second;

            for (int i = 1; i < stops.Count(); i++)
            {
                if (t <= stops[i].first)
                {
                    float k = (t - stops[i - 1].first) / Math::Max(0.0001f, stops[i].first - stops[i - 1].first);
                    return Math::Lerp(stops[i - 1].second, stops[i].second, k);
                }
            }
            return stops.Last().second;
        }

        namespace
        {
            struct Batch
            {
                Vector<Vertex>      vertices;
                Vector<VertexIndex> indices;

                int Add(const Vec2F& p, const Color4& color)
                {
                    vertices.Add(Vertex(p, color.ABGR(), 0.0f, 0.0f));
                    return vertices.Count() - 1;
                }

                void Triangle(int a, int b, int c)
                {
                    indices.Add((VertexIndex)a);
                    indices.Add((VertexIndex)b);
                    indices.Add((VertexIndex)c);
                }

                void Draw()
                {
                    if (indices.IsEmpty())
                        return;

                    static Mesh mesh(TextureRef(), 1024, 1024);
                    UInt polyCount = indices.Count() / 3;
                    if (mesh.GetMaxVertexCount() < (UInt)vertices.Count() || mesh.GetMaxPolyCount() < polyCount)
                        mesh.Resize(Math::Max((UInt)vertices.Count(), mesh.GetMaxVertexCount()), Math::Max(polyCount, mesh.GetMaxPolyCount()));

                    memcpy(mesh.GetVertices<Vertex>(), vertices.Data(), sizeof(Vertex) * vertices.Count());
                    memcpy(mesh.GetIndexes(), indices.Data(), sizeof(VertexIndex) * indices.Count());
                    mesh.vertexCount = vertices.Count();
                    mesh.polyCount = polyCount;
                    mesh.Draw();
                }
            };

            // Rings reused across calls: a card draws dozens of shapes a frame, each would allocate its own otherwise
            Vector<Vec2F>& RingBuffer(int index)
            {
                static Vector<Vec2F> buffers[5];
                return buffers[index];
            }

            Batch& SharedBatch()
            {
                static Batch batch;
                batch.vertices.Clear();
                batch.indices.Clear();
                return batch;
            }

            int SegmentsFor(const Corners& c)
            {
                float r = Math::Max(Math::Max(c.lb, c.rb), Math::Max(c.rt, c.lt));
                float px = r / Math::Max(0.0001f, Pixel());
                return Math::Clamp((int)Math::Ceil(px * 0.5f), 2, 24);
            }

            // Outline of the rectangle grown by d (negative shrinks), the radii following; same point count for any d
            void Ring(const RectF& rect, const Corners& c, float d, int segments, Vector<Vec2F>& out)
            {
                out.Clear();
                out.Reserve(4 * (segments + 1));
                float cx = (rect.left + rect.right) * 0.5f, cy = (rect.top + rect.bottom) * 0.5f;
                float l = Math::Min(rect.left - d, cx), r = Math::Max(rect.right + d, cx);
                float b = Math::Min(rect.bottom - d, cy), t = Math::Max(rect.top + d, cy);
                float maxR = Math::Min(r - l, t - b) * 0.5f;
                auto radius = [&](float base) { return base > 0.0f ? Math::Clamp(base + d, 0.0f, maxR) : 0.0f; };

                const float pi = Math::PI();
                auto corner = [&](float rad, float ccx, float ccy, float from)
                {
                    for (int i = 0; i <= segments; i++)
                    {
                        float a = from + pi * 0.5f * i / segments;
                        out.Add(Vec2F(ccx + Math::Cos(a) * rad, ccy + Math::Sin(a) * rad));
                    }
                };

                float rlb = radius(c.lb), rrb = radius(c.rb), rrt = radius(c.rt), rlt = radius(c.lt);
                corner(rlb, l + rlb, b + rlb, pi);
                corner(rrb, r - rrb, b + rrb, pi * 1.5f);
                corner(rrt, r - rrt, t - rrt, 0.0f);
                corner(rlt, l + rlt, t - rlt, pi * 0.5f);
            }

            Color4 Faded(const Color4& color, float k)
            {
                return Color4(color.r, color.g, color.b, (int)Math::Clamp(color.a * k, 0.0f, 255.0f));
            }

            // Strip between two rings of the same point count; alphas scale the gradient colours
            void Strip(Batch& batch, const Vector<Vec2F>& a, float alphaA, const Vector<Vec2F>& b, float alphaB,
                       const Gradient& gradient, float top, float height)
            {
                int n = a.Count();
                int baseA = batch.vertices.Count();
                for (auto& p : a)
                    batch.Add(p, Faded(gradient.At((top - p.y) / height), alphaA));
                int baseB = batch.vertices.Count();
                for (auto& p : b)
                    batch.Add(p, Faded(gradient.At((top - p.y) / height), alphaB));

                for (int i = 0; i < n; i++)
                {
                    int j = (i + 1) % n;
                    batch.Triangle(baseA + i, baseB + i, baseB + j);
                    batch.Triangle(baseA + i, baseB + j, baseA + j);
                }
            }

            void Fan(Batch& batch, const Vec2F& center, const Vector<Vec2F>& ring, const Gradient& gradient, float top, float height)
            {
                int c = batch.Add(center, gradient.At((top - center.y) / height));
                int base = batch.vertices.Count();
                for (auto& p : ring)
                    batch.Add(p, gradient.At((top - p.y) / height));

                int n = ring.Count();
                for (int i = 0; i < n; i++)
                    batch.Triangle(c, base + i, base + (i + 1) % n);
            }
        }

        float Pixel()
        {
            Vec2F scale = o2Render.GetViewPixelScale();
            return 1.0f / Math::Max(0.0001f, scale.x);
        }

        void Fill(const RectF& rect, const Corners& corners, const Color4& color)
        {
            Fill(rect, corners, Gradient(color));
        }

        void Fill(const RectF& rect, const Corners& corners, const Gradient& gradient)
        {
            if (rect.Width() <= 0.0f || rect.Height() <= 0.0f)
                return;

            float a = Pixel();
            int segments = SegmentsFor(corners);
            float top = rect.top, height = Math::Max(0.0001f, rect.top - rect.bottom);

            auto& inner = RingBuffer(0);
            auto& outer = RingBuffer(1);
            Ring(rect, corners, -a * 0.5f, segments, inner);
            Ring(rect, corners, a * 0.5f, segments, outer);

            auto& batch = SharedBatch();
            Fan(batch, rect.Center(), inner, gradient, top, height);
            Strip(batch, inner, 1.0f, outer, 0.0f, gradient, top, height);
            batch.Draw();
        }

        // A 2 x 2 texel tile of the two colours, repeated by the sampler; one per colour pair
        static TextureRef CheckerTexture(const Color4& first, const Color4& second, bool secondTopLeft)
        {
            static std::map<std::tuple<ULong, ULong, bool>, TextureRef> cache;
            auto key = std::make_tuple(first.ABGR(), second.ABGR(), secondTopLeft);
            auto it = cache.find(key);
            if (it != cache.end())
                return it->second;

            Bitmap bitmap(PixelFormat::R8G8B8A8, Vec2I(2, 2));
            UInt8* data = bitmap.GetData();
            const Color4* cells[4] = { secondTopLeft ? &second : &first, secondTopLeft ? &first : &second,
                                       secondTopLeft ? &first : &second, secondTopLeft ? &second : &first };
            for (int i = 0; i < 4; i++)
            {
                data[i * 4 + 0] = (UInt8)cells[i]->r;
                data[i * 4 + 1] = (UInt8)cells[i]->g;
                data[i * 4 + 2] = (UInt8)cells[i]->b;
                data[i * 4 + 3] = (UInt8)cells[i]->a;
            }

            TextureRef texture(bitmap);
            texture->SetFilter(Texture::Filter::Nearest);
            texture->SetWrap(Texture::Wrap::Repeat);
            cache[key] = texture;
            return texture;
        }

        void Checker(const RectF& rect, const Corners& corners, float cell, const Color4& first, const Color4& second,
                     bool secondTopLeft)
        {
            if (rect.Width() <= 0.0f || rect.Height() <= 0.0f || cell <= 0.0f)
                return;

            float a = Pixel();
            int segments = SegmentsFor(corners);
            auto& inner = RingBuffer(0);
            auto& outer = RingBuffer(1);
            Ring(rect, corners, -a * 0.5f, segments, inner);
            Ring(rect, corners, a * 0.5f, segments, outer);

            // Texture rows run down from the top-left corner, two cells per repeat
            float k = 0.5f / cell;
            ULong opaque = Color4::White().ABGR(), clear = Color4(255, 255, 255, 0).ABGR();
            static Vector<Vertex> vertices;
            static Vector<VertexIndex> indices;
            vertices.Clear();
            indices.Clear();
            auto add = [&](const Vec2F& p, ULong color)
            {
                vertices.Add(Vertex(p, color, (p.x - rect.left) * k, (rect.top - p.y) * k));
                return (VertexIndex)(vertices.Count() - 1);
            };

            VertexIndex center = add(rect.Center(), opaque);
            VertexIndex innerBase = (VertexIndex)vertices.Count();
            for (auto& p : inner)
                add(p, opaque);
            VertexIndex outerBase = (VertexIndex)vertices.Count();
            for (auto& p : outer)
                add(p, clear);

            int n = inner.Count();
            for (int i = 0; i < n; i++)
            {
                int j = (i + 1) % n;
                indices.Add(center); indices.Add(innerBase + i); indices.Add(innerBase + j);
                indices.Add(innerBase + i); indices.Add(outerBase + i); indices.Add(outerBase + j);
                indices.Add(innerBase + i); indices.Add(outerBase + j); indices.Add(innerBase + j);
            }

            o2Render.DrawBuffer(PrimitiveType::Polygon, vertices.Data(), vertices.Count(), indices.Data(), indices.Count() / 3,
                                o2Render.GetDefaultMaterial(), CheckerTexture(first, second, secondTopLeft));
        }

        void Stroke(const RectF& rect, const Corners& corners, float from, float to, const Gradient& gradient)
        {
            if (to <= from || rect.Width() <= 0.0f || rect.Height() <= 0.0f)
                return;

            float a = Pixel();
            int segments = SegmentsFor(corners);
            float top = rect.top, height = Math::Max(0.0001f, rect.top - rect.bottom);
            float d0 = -from, d1 = -to;

            auto& batch = SharedBatch();
            auto& r0 = RingBuffer(0);
            auto& r1 = RingBuffer(1);
            auto& r2 = RingBuffer(2);
            auto& r3 = RingBuffer(3);
            if (to - from >= a)
            {
                Ring(rect, corners, d0 + a * 0.5f, segments, r0);
                Ring(rect, corners, d0 - a * 0.5f, segments, r1);
                Ring(rect, corners, d1 + a * 0.5f, segments, r2);
                Ring(rect, corners, d1 - a * 0.5f, segments, r3);
                Strip(batch, r0, 0.0f, r1, 1.0f, gradient, top, height);
                Strip(batch, r1, 1.0f, r2, 1.0f, gradient, top, height);
                Strip(batch, r2, 1.0f, r3, 0.0f, gradient, top, height);
            }
            else
            {
                // A hairline keeps its total ink: a pixel-wide ridge of proportional strength
                float mid = (d0 + d1) * 0.5f, k = (to - from) / a;
                Ring(rect, corners, mid + a, segments, r0);
                Ring(rect, corners, mid, segments, r1);
                Ring(rect, corners, mid - a, segments, r2);
                Strip(batch, r0, 0.0f, r1, k, gradient, top, height);
                Strip(batch, r1, k, r2, 0.0f, gradient, top, height);
            }
            batch.Draw();
        }

        void Frame(const RectF& rect, const Corners& corners, float width, const Color4& color)
        {
            Stroke(rect, corners, -width * 0.5f, width * 0.5f, Gradient(color));
        }

        void Shadow(const RectF& rect, const Corners& corners, const Color4& color, float blur)
        {
            if (color.a == 0 || rect.Width() <= 0.0f || rect.Height() <= 0.0f)
                return;

            // Rings at -2σ..2σ around the edge carry the Gaussian falloff of the CSS shadow
            float sigma = Math::Max(0.5f, blur * 0.5f);
            const float offsets[] = { -2.0f, -1.0f, 0.0f, 1.0f, 2.0f };
            const float alphas[] = { 1.0f, 0.84f, 0.5f, 0.16f, 0.0f };
            Corners grown = corners;
            int segments = Math::Max(SegmentsFor(corners), Math::Clamp((int)(sigma / Math::Max(0.0001f, Pixel()) * 0.3f), 4, 24));

            float top = rect.top, height = Math::Max(0.0001f, rect.top - rect.bottom);
            Gradient flat(color);
            auto& batch = SharedBatch();
            for (int i = 0; i < 5; i++)
                Ring(rect, grown, offsets[i] * sigma, segments, RingBuffer(i));

            auto& first = RingBuffer(0);
            int c = batch.Add(rect.Center(), color);
            int base = batch.vertices.Count();
            for (auto& p : first)
                batch.Add(p, color);
            for (int i = 0; i < first.Count(); i++)
                batch.Triangle(c, base + i, base + (i + 1) % first.Count());

            for (int i = 0; i < 4; i++)
                Strip(batch, RingBuffer(i), alphas[i], RingBuffer(i + 1), alphas[i + 1], flat, top, height);
            batch.Draw();
        }

        void Circle(const Vec2F& center, float radius, const Color4& color)
        {
            RectF rect(center.x - radius, center.y + radius, center.x + radius, center.y - radius);
            Fill(rect, Corners(radius), color);
        }

        void Ring(const Vec2F& center, float radius, float width, const Color4& color)
        {
            RectF rect(center.x - radius, center.y + radius, center.x + radius, center.y - radius);
            Stroke(rect, Corners(radius), -width * 0.5f, width * 0.5f, Gradient(color));
        }

        void Polygon(const Vector<Vec2F>& points, const Color4& color)
        {
            int n = points.Count();
            if (n < 3)
                return;

            float a = Pixel();
            auto& inner = RingBuffer(0);
            auto& outer = RingBuffer(1);
            inner.Clear();
            outer.Clear();
            Vec2F center;
            for (auto& p : points)
                center += p;
            center /= (float)n;

            for (int i = 0; i < n; i++)
            {
                Vec2F prev = points[(i + n - 1) % n], cur = points[i], next = points[(i + 1) % n];
                Vec2F n0 = (cur - prev).Normalized().Perpendicular() * -1.0f;
                Vec2F n1 = (next - cur).Normalized().Perpendicular() * -1.0f;
                Vec2F normal = (n0 + n1).Normalized();
                float k = Math::Max(0.25f, normal.Dot(n1));
                Vec2F offset = normal * (a * 0.5f / k);
                inner.Add(cur - offset);
                outer.Add(cur + offset);
            }

            Gradient flat(color);
            auto& batch = SharedBatch();
            Fan(batch, center, inner, flat, 0.0f, 1.0f);
            Strip(batch, inner, 1.0f, outer, 0.0f, flat, 0.0f, 1.0f);
            batch.Draw();
        }

        void Line(const Vec2F& a, const Vec2F& b, float width, const Color4& color)
        {
            Vec2F dir = (b - a).Normalized();
            if (dir.SqrLength() < 0.5f)
                return;

            Vec2F side = dir.Perpendicular() * (width * 0.5f);
            Vector<Vec2F> quad = { a - side, b - side, b + side, a + side };
            // Counter-clockwise for the outward normals
            if ((quad[1] - quad[0]).Cross(quad[2] - quad[1]) < 0.0f)
                quad = { quad[3], quad[2], quad[1], quad[0] };
            Polygon(quad, color);
        }
    }
}
