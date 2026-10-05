#include "o2/stdafx.h"
#include "VectorMesh.h"

#include "o2/Utils/Math/Math.h"
#include "o2/Utils/Types/Containers/Map.h"
#include "o2/Utils/Types/Containers/Pair.h"

namespace o2
{
    namespace
    {
        float MapSlicedCoordinate(float value, float imageSize, float nearBorder, float farBorder, float targetSize)
        {
            float excess = nearBorder + farBorder - targetSize;
            if (excess > 0.0f)
            {
                nearBorder -= excess*0.5f;
                farBorder -= excess*0.5f;
            }

            if (value <= nearBorder)
                return value;

            if (value >= imageSize - farBorder)
                return targetSize - (imageSize - value);

            float sourceCenter = imageSize - nearBorder - farBorder;
            if (sourceCenter <= 0.0f)
                return nearBorder;

            return nearBorder + (value - nearBorder)*(targetSize - nearBorder - farBorder)/sourceCenter;
        }

        Color32Bit MultiplyColor(Color32Bit color, const Color4& multiplier)
        {
            const int channels[4] = { multiplier.r, multiplier.g, multiplier.b, multiplier.a };

            Color32Bit res = 0;
            for (int i = 0; i < 4; i++)
                res |= (Color32Bit)Math::Clamp((int)((color >> (i*8)) & 0xFF)*channels[i]/255, 0, 255) << (i*8);

            return res;
        }
    }

    void VectorMesh::Clear()
    {
        positions.Clear();
        colors.Clear();
        indexes.Clear();
    }

    UInt VectorMesh::GetTrianglesCount() const
    {
        return (UInt)indexes.Count()/3;
    }

    bool VectorMesh::GetBounds(Vec2F& min, Vec2F& max) const
    {
        if (positions.IsEmpty())
            return false;

        min = max = positions[0];
        for (const Vec2F& position : positions)
        {
            min.x = Math::Min(min.x, position.x);
            min.y = Math::Min(min.y, position.y);
            max.x = Math::Max(max.x, position.x);
            max.y = Math::Max(max.y, position.y);
        }

        return true;
    }

    void VectorMesh::SplitByLine(bool vertical, float coordinate)
    {
        const double epsilon = 1e-7;

        auto getValue = [&](VertexIndex idx) { return vertical ? positions[idx].x : positions[idx].y; };
        auto getSide = [&](VertexIndex idx)
        {
            double delta = (double)getValue(idx) - coordinate;
            return delta > epsilon ? 1 : (delta < -epsilon ? -1 : 0);
        };

        Map<Pair<VertexIndex, VertexIndex>, VertexIndex> cuts;

        // Ordered ends make the cut point identical for both triangles sharing the edge
        auto cut = [&](VertexIndex a, VertexIndex b)
        {
            const Vec2F& pa = positions[a];
            const Vec2F& pb = positions[b];
            if (!(pa.x < pb.x || (pa.x == pb.x && pa.y <= pb.y)))
                std::swap(a, b);

            Pair<VertexIndex, VertexIndex> key(a, b);
            auto found = cuts.find(key);
            if (found != cuts.end())
                return found->second;

            Vec2F first = positions[a], second = positions[b];
            double coef = ((double)coordinate - getValue(a))/((double)getValue(b) - getValue(a));

            Vec2F position(vertical ? coordinate : (float)(first.x + (second.x - first.x)*coef),
                           vertical ? (float)(first.y + (second.y - first.y)*coef) : coordinate);

            Color32Bit color = 0;
            for (int channel = 0; channel < 4; channel++)
            {
                double from = (double)((colors[a] >> (channel*8)) & 0xFF);
                double to = (double)((colors[b] >> (channel*8)) & 0xFF);
                int value = Math::Clamp(Math::RoundToInt((float)(from + (to - from)*coef)), 0, 255);
                color |= (Color32Bit)value << (channel*8);
            }

            VertexIndex idx = (VertexIndex)positions.Count();
            positions.Add(position);
            colors.Add(color);
            cuts[key] = idx;

            return idx;
        };

        Vector<VertexIndex> result;
        result.reserve(indexes.Count());

        bool straddles = false;
        for (int i = 0; i + 2 < indexes.Count(); i += 3)
        {
            VertexIndex v[3] = { indexes[i], indexes[i + 1], indexes[i + 2] };
            int s[3] = { getSide(v[0]), getSide(v[1]), getSide(v[2]) };

            bool hasPositive = s[0] > 0 || s[1] > 0 || s[2] > 0;
            bool hasNegative = s[0] < 0 || s[1] < 0 || s[2] < 0;
            if (!hasPositive || !hasNegative)
            {
                result.Add(v[0]);
                result.Add(v[1]);
                result.Add(v[2]);
                continue;
            }

            straddles = true;

            int lone = 0;
            for (int k = 0; k < 3; k++)
            {
                int next = s[(k + 1)%3], prev = s[(k + 2)%3];
                if (s[k] == 0 || (next == prev && next != s[k]))
                    lone = k;
            }

            VertexIndex a = v[lone], b = v[(lone + 1)%3], c = v[(lone + 2)%3];
            if (s[lone] == 0)
            {
                VertexIndex middle = cut(b, c);
                for (VertexIndex idx : { a, b, middle, a, middle, c })
                    result.Add(idx);
            }
            else
            {
                VertexIndex ab = cut(a, b), ac = cut(a, c);
                for (VertexIndex idx : { a, ab, ac, ab, b, c, ab, c, ac })
                    result.Add(idx);
            }
        }

        if (straddles)
            indexes = result;
    }

    void VectorMesh::SplitBySlices(const BorderF& borders)
    {
        if (borders.left > 0.0f)
            SplitByLine(true, borders.left);

        if (borders.right > 0.0f)
            SplitByLine(true, size.x - borders.right);

        if (borders.top > 0.0f)
            SplitByLine(false, borders.top);

        if (borders.bottom > 0.0f)
            SplitByLine(false, size.y - borders.bottom);
    }

    void VectorMesh::FillVertices(Vertex* vertices, const Basis& transform,
                                  const Color4& color /*= Color4::White()*/) const
    {
        FillSlicedVertices(vertices, transform, BorderF(), size, color);
    }

    void VectorMesh::FillSlicedVertices(Vertex* vertices, const Basis& transform, const BorderF& borders,
                                        const Vec2F& targetSize, const Color4& color /*= Color4::White()*/) const
    {
        bool sliced = borders != BorderF() || targetSize != size;
        bool white = color == Color4::White();
        Vec2F xv = transform.xv/targetSize.x, yv = transform.yv/targetSize.y;
        Vec2F origin = transform.origin + transform.yv;

        for (int i = 0; i < positions.Count(); i++)
        {
            Vec2F point = sliced ? MapSlicedPoint(positions[i], size, borders, targetSize) : positions[i];
            Color32Bit vertexColor = white ? colors[i] : MultiplyColor(colors[i], color);
            vertices[i].Set(origin + xv*point.x - yv*point.y, vertexColor, 0.0f, 0.0f);
        }
    }

    Vec2F VectorMesh::MapSlicedPoint(const Vec2F& point, const Vec2F& imageSize, const BorderF& borders,
                                     const Vec2F& targetSize)
    {
        return Vec2F(MapSlicedCoordinate(point.x, imageSize.x, borders.left, borders.right, targetSize.x),
                     MapSlicedCoordinate(point.y, imageSize.y, borders.top, borders.bottom, targetSize.y));
    }
}
