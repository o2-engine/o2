#pragma once

#include <float.h>
#include "o2/Utils/Math/Math.h"
#include "o2/Utils/Math/Rect.h"
#include "o2/Utils/Math/Vector3.h"

namespace o2
{
    struct Basis3D;

    // ---------------------------------------------------------------
    // 3D axis aligned bounding box, min and max corners, 3D analog of
    // the 2D Rect
    // ---------------------------------------------------------------
    struct AABB
    {
        Vec3F min, max;

        inline AABB();
        inline AABB(const Vec3F& min, const Vec3F& max);

        inline bool operator==(const AABB& other) const;
        inline bool operator!=(const AABB& other) const;

        inline AABB operator+(const Vec3F& offset) const;
        inline AABB operator+=(const Vec3F& offset);

        inline AABB operator-(const Vec3F& offset) const;
        inline AABB operator-=(const Vec3F& offset);

        inline AABB operator+(const AABB& other) const;
        inline AABB operator+=(const AABB& other);

        inline Vec3F GetCenter() const;
        inline Vec3F GetSize() const;

        inline AABB Expand(const AABB& other) const;

        inline void Include(const Vec3F& point);
        inline void Include(const AABB& box);

        inline bool Intersects(const AABB& other) const;
        inline bool IsInside(const Vec3F& point) const;

        // Slab method ray intersection; distance is the nearest hit along the ray, 0 when origin is inside
        inline bool IntersectsRay(const Vec3F& origin, const Vec3F& direction, float& distance) const;

        // Returns axis aligned bound of the box corners transformed by basis; defined in Basis3D.h
        AABB Transformed(const Basis3D& basis) const;

        // Returns XY projection without min/max normalization
        inline RectF ToRect() const;

        inline static AABB FromCenterSize(const Vec3F& center, const Vec3F& size);
        inline static AABB FromRect(const RectF& rect, float zMin = 0.0f, float zMax = 0.0f);
        inline static AABB Bound(const Vec3F* points, int count);
    };

    AABB::AABB():
        min(), max()
    {}

    AABB::AABB(const Vec3F& min, const Vec3F& max):
        min(Math::Min(min.x, max.x), Math::Min(min.y, max.y), Math::Min(min.z, max.z)),
        max(Math::Max(min.x, max.x), Math::Max(min.y, max.y), Math::Max(min.z, max.z))
    {}

    bool AABB::operator==(const AABB& other) const
    {
        return Math::Abs(min.x - other.min.x) <= FLT_EPSILON && Math::Abs(min.y - other.min.y) <= FLT_EPSILON &&
            Math::Abs(min.z - other.min.z) <= FLT_EPSILON && Math::Abs(max.x - other.max.x) <= FLT_EPSILON &&
            Math::Abs(max.y - other.max.y) <= FLT_EPSILON && Math::Abs(max.z - other.max.z) <= FLT_EPSILON;
    }

    bool AABB::operator!=(const AABB& other) const
    {
        return !(*this == other);
    }

    AABB AABB::operator+(const Vec3F& offset) const
    {
        AABB res;
        res.min = min + offset;
        res.max = max + offset;
        return res;
    }

    AABB AABB::operator+=(const Vec3F& offset)
    {
        min += offset;
        max += offset;
        return *this;
    }

    AABB AABB::operator-(const Vec3F& offset) const
    {
        AABB res;
        res.min = min - offset;
        res.max = max - offset;
        return res;
    }

    AABB AABB::operator-=(const Vec3F& offset)
    {
        min -= offset;
        max -= offset;
        return *this;
    }

    AABB AABB::operator+(const AABB& other) const
    {
        return Expand(other);
    }

    AABB AABB::operator+=(const AABB& other)
    {
        Include(other);
        return *this;
    }

    Vec3F AABB::GetCenter() const
    {
        return (min + max)*0.5f;
    }

    Vec3F AABB::GetSize() const
    {
        return max - min;
    }

    AABB AABB::Expand(const AABB& other) const
    {
        AABB res = *this;
        res.Include(other);
        return res;
    }

    void AABB::Include(const Vec3F& point)
    {
        min.x = Math::Min(min.x, point.x);
        min.y = Math::Min(min.y, point.y);
        min.z = Math::Min(min.z, point.z);
        max.x = Math::Max(max.x, point.x);
        max.y = Math::Max(max.y, point.y);
        max.z = Math::Max(max.z, point.z);
    }

    void AABB::Include(const AABB& box)
    {
        Include(box.min);
        Include(box.max);
    }

    bool AABB::Intersects(const AABB& other) const
    {
        return !(max.x < other.min.x || min.x > other.max.x ||
                 max.y < other.min.y || min.y > other.max.y ||
                 max.z < other.min.z || min.z > other.max.z);
    }

    bool AABB::IsInside(const Vec3F& point) const
    {
        return point.x > min.x && point.x < max.x &&
            point.y > min.y && point.y < max.y &&
            point.z > min.z && point.z < max.z;
    }

    bool AABB::IntersectsRay(const Vec3F& origin, const Vec3F& direction, float& distance) const
    {
        const float* o = &origin.x;
        const float* d = &direction.x;
        const float* boundsMin = &min.x;
        const float* boundsMax = &max.x;

        float tmin = 0.0f, tmax = FLT_MAX;
        for (int i = 0; i < 3; i++)
        {
            if (Math::Abs(d[i]) < 1e-8f)
            {
                if (o[i] < boundsMin[i] || o[i] > boundsMax[i])
                    return false;

                continue;
            }

            float inv = 1.0f/d[i];
            float t1 = (boundsMin[i] - o[i])*inv;
            float t2 = (boundsMax[i] - o[i])*inv;
            if (t1 > t2)
                Math::Swap(t1, t2);

            tmin = Math::Max(tmin, t1);
            tmax = Math::Min(tmax, t2);
            if (tmin > tmax)
                return false;
        }

        distance = tmin;
        return true;
    }

    RectF AABB::ToRect() const
    {
        RectF res;
        res.left = min.x; res.bottom = min.y;
        res.right = max.x; res.top = max.y;
        return res;
    }

    AABB AABB::FromCenterSize(const Vec3F& center, const Vec3F& size)
    {
        Vec3F half = size*0.5f;
        return AABB(center - half, center + half);
    }

    AABB AABB::FromRect(const RectF& rect, float zMin /*= 0.0f*/, float zMax /*= 0.0f*/)
    {
        return AABB(Vec3F(rect.left, rect.bottom, zMin), Vec3F(rect.right, rect.top, zMax));
    }

    AABB AABB::Bound(const Vec3F* points, int count)
    {
        AABB res;
        res.min = points[0];
        res.max = points[0];

        for (int i = 1; i < count; i++)
            res.Include(points[i]);

        return res;
    }
}
