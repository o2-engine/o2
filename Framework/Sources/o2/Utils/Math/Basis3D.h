#pragma once

#include "o2/Utils/Math/AABB.h"
#include "o2/Utils/Math/Basis.h"
#include "o2/Utils/Math/Matrix4.h"
#include "o2/Utils/Math/Quaternion.h"
#include "o2/Utils/Math/Vector3.h"

namespace o2
{
    // ------------------------------------------------------------------------------
    // 3D basis. Affine 3x4 transform: xv, yv, zv are the images of the axes and
    // origin is the translation, same row-vector convention as the 2D Basis
    // ------------------------------------------------------------------------------
    struct Basis3D
    {
        Vec3F xv, yv, zv, origin;

        inline Basis3D();
        inline Basis3D(const Vec3F& origin, const Vec3F& xvec = Vec3F(1, 0, 0), const Vec3F& yvec = Vec3F(0, 1, 0),
                       const Vec3F& zvec = Vec3F(0, 0, 1));
        inline explicit Basis3D(const Mat4& matrix);
        inline explicit Basis3D(const Basis& basis);

        inline bool operator==(const Basis3D& other) const;
        inline bool operator!=(const Basis3D& other) const;

        inline Basis3D operator*(const Basis3D& cbasis) const;
        inline Vec3F operator*(const Vec3F& vec) const;

        inline void Set(const Vec3F& origin = Vec3F(), const Vec3F& xvec = Vec3F(1, 0, 0),
                        const Vec3F& yvec = Vec3F(0, 1, 0), const Vec3F& zvec = Vec3F(0, 0, 1));

        inline Mat4  ToMat4() const;
        inline Basis ToBasis() const;

        inline Vec3F GetScale() const;
        inline Quat  GetRotation() const;
        inline Vec3F GetShear() const;

        inline void Decompose(Vec3F* origin, Quat* rotation, Vec3F* scale) const;

        inline Basis3D Inverted() const;
        inline void    Inverse();

        inline void Translate(const Vec3F& voffs);
        inline void Scale(const Vec3F& scalev);
        inline void Rotate(const Quat& rotation);

        inline Vec3F Transform(const Vec3F& vec) const;
        inline Vec3F TransformDirection(const Vec3F& vec) const;

        inline o2::AABB AABB() const;

        inline static Basis3D Identity();
        inline static Basis3D Translated(const Vec3F& voffs);
        inline static Basis3D Scaled(const Vec3F& scale);
        inline static Basis3D Rotated(const Quat& rotation);
        inline static Basis3D Build(const Vec3F& position, const Vec3F& scale, const Quat& rotation,
                                    const Vec3F& shear = Vec3F());
        inline static Basis3D Build(const Vec3F& position, const Vec3F& scale, const Vec3F& eulerAngles,
                                    const Vec3F& shear = Vec3F());
    };

    inline Vec3F operator*(const Vec3F& vec, const Basis3D& basis)
    {
        return basis.Transform(vec);
    }

    Basis3D::Basis3D():
        xv(1, 0, 0), yv(0, 1, 0), zv(0, 0, 1), origin()
    {}

    Basis3D::Basis3D(const Vec3F& origin, const Vec3F& xvec /*= Vec3F(1, 0, 0)*/, const Vec3F& yvec /*= Vec3F(0, 1, 0)*/,
                     const Vec3F& zvec /*= Vec3F(0, 0, 1)*/):
        xv(xvec), yv(yvec), zv(zvec), origin(origin)
    {}

    Basis3D::Basis3D(const Mat4& matrix):
        xv(matrix.m[0], matrix.m[1], matrix.m[2]),
        yv(matrix.m[4], matrix.m[5], matrix.m[6]),
        zv(matrix.m[8], matrix.m[9], matrix.m[10]),
        origin(matrix.m[12], matrix.m[13], matrix.m[14])
    {}

    Basis3D::Basis3D(const Basis& basis):
        xv(basis.xv, 0.0f), yv(basis.yv, 0.0f), zv(0, 0, 1), origin(basis.origin, 0.0f)
    {}

    bool Basis3D::operator==(const Basis3D& other) const
    {
        return xv == other.xv && yv == other.yv && zv == other.zv && origin == other.origin;
    }

    bool Basis3D::operator!=(const Basis3D& other) const
    {
        return !(*this == other);
    }

    Basis3D Basis3D::operator*(const Basis3D& cbasis) const
    {
        Basis3D res;
        res.xv = cbasis.TransformDirection(xv);
        res.yv = cbasis.TransformDirection(yv);
        res.zv = cbasis.TransformDirection(zv);
        res.origin = cbasis.Transform(origin);
        return res;
    }

    Vec3F Basis3D::operator*(const Vec3F& vec) const
    {
        return Transform(vec);
    }

    void Basis3D::Set(const Vec3F& origin /*= Vec3F()*/, const Vec3F& xvec /*= Vec3F(1, 0, 0)*/,
                      const Vec3F& yvec /*= Vec3F(0, 1, 0)*/, const Vec3F& zvec /*= Vec3F(0, 0, 1)*/)
    {
        xv = xvec; yv = yvec; zv = zvec;
        this->origin = origin;
    }

    Mat4 Basis3D::ToMat4() const
    {
        Mat4 res;

        res.m[0] = xv.x;     res.m[1] = xv.y;     res.m[2] = xv.z;     res.m[3] = 0.0f;
        res.m[4] = yv.x;     res.m[5] = yv.y;     res.m[6] = yv.z;     res.m[7] = 0.0f;
        res.m[8] = zv.x;     res.m[9] = zv.y;     res.m[10] = zv.z;    res.m[11] = 0.0f;
        res.m[12] = origin.x; res.m[13] = origin.y; res.m[14] = origin.z; res.m[15] = 1.0f;

        return res;
    }

    Basis Basis3D::ToBasis() const
    {
        return Basis(origin.XY(), xv.XY(), yv.XY());
    }

    Vec3F Basis3D::GetScale() const
    {
        return Vec3F(xv.Length(), yv.Length(), zv.Length());
    }

    Quat Basis3D::GetRotation() const
    {
        Vec3F pos, scale;
        Quat rot;
        ToMat4().Decompose(pos, rot, scale);
        return rot;
    }

    Vec3F Basis3D::GetShear() const
    {
        Vec3F scale = GetScale();
        if (Math::Abs(scale.x) < Math::Epsilon)
            return Vec3F();

        Vec3F nx = xv/scale.x;
        Vec3F res;

        if (Math::Abs(scale.y) > Math::Epsilon)
            res.x = yv.Dot(nx)/scale.y;

        if (Math::Abs(scale.z) > Math::Epsilon)
        {
            res.y = zv.Dot(nx)/scale.z;

            Vec3F ey = yv - nx*yv.Dot(nx);
            float eyLength = ey.Length();
            if (eyLength > Math::Epsilon)
                res.z = zv.Dot(ey/eyLength)/scale.z;
        }

        return res;
    }

    void Basis3D::Decompose(Vec3F* origin, Quat* rotation, Vec3F* scale) const
    {
        ToMat4().Decompose(*origin, *rotation, *scale);
    }

    Basis3D Basis3D::Inverted() const
    {
        Vec3F yz = yv.Cross(zv);
        Vec3F zx = zv.Cross(xv);
        Vec3F xy = xv.Cross(yv);

        float det = xv.Dot(yz);
        if (Math::Abs(det) < FLT_EPSILON)
            return Basis3D();

        float invDet = 1.0f/det;

        Basis3D res;
        res.xv = Vec3F(yz.x, zx.x, xy.x)*invDet;
        res.yv = Vec3F(yz.y, zx.y, xy.y)*invDet;
        res.zv = Vec3F(yz.z, zx.z, xy.z)*invDet;
        res.origin = -res.TransformDirection(origin);

        return res;
    }

    void Basis3D::Inverse()
    {
        *this = Inverted();
    }

    void Basis3D::Translate(const Vec3F& voffs)
    {
        origin += voffs;
    }

    void Basis3D::Scale(const Vec3F& scalev)
    {
        xv *= scalev.x;
        yv *= scalev.y;
        zv *= scalev.z;
    }

    void Basis3D::Rotate(const Quat& rotation)
    {
        xv = rotation*xv;
        yv = rotation*yv;
        zv = rotation*zv;
    }

    Vec3F Basis3D::Transform(const Vec3F& vec) const
    {
        return xv*vec.x + yv*vec.y + zv*vec.z + origin;
    }

    Vec3F Basis3D::TransformDirection(const Vec3F& vec) const
    {
        return xv*vec.x + yv*vec.y + zv*vec.z;
    }

    o2::AABB Basis3D::AABB() const
    {
        Vec3F points[8] =
        {
            origin, origin + xv, origin + yv, origin + zv,
            origin + xv + yv, origin + xv + zv, origin + yv + zv,
            origin + xv + yv + zv
        };

        return o2::AABB::Bound(points, 8);
    }

    Basis3D Basis3D::Identity()
    {
        return Basis3D();
    }

    Basis3D Basis3D::Translated(const Vec3F& voffs)
    {
        return Basis3D(voffs);
    }

    Basis3D Basis3D::Scaled(const Vec3F& scale)
    {
        return Basis3D(Vec3F(), Vec3F(scale.x, 0, 0), Vec3F(0, scale.y, 0), Vec3F(0, 0, scale.z));
    }

    Basis3D Basis3D::Rotated(const Quat& rotation)
    {
        return Basis3D(Vec3F(), rotation*Vec3F(1, 0, 0), rotation*Vec3F(0, 1, 0), rotation*Vec3F(0, 0, 1));
    }

    Basis3D Basis3D::Build(const Vec3F& position, const Vec3F& scale, const Quat& rotation,
                           const Vec3F& shear /*= Vec3F()*/)
    {
        // Each shear plane tilts the second axis towards the first, keeping unit length,
        // exactly like the 2D Basis::Build does for the XY plane
        float shearYY = Math::Sqrt(Math::Max(0.0f, 1.0f - shear.x*shear.x));
        float shearZZ = Math::Sqrt(Math::Max(0.0f, 1.0f - shear.y*shear.y - shear.z*shear.z));

        Vec3F x = rotation*Vec3F(scale.x, 0.0f, 0.0f);
        Vec3F y = rotation*(Vec3F(shear.x, shearYY, 0.0f)*scale.y);
        Vec3F z = rotation*(Vec3F(shear.y, shear.z, shearZZ)*scale.z);

        return Basis3D(position, x, y, z);
    }

    Basis3D Basis3D::Build(const Vec3F& position, const Vec3F& scale, const Vec3F& eulerAngles,
                           const Vec3F& shear /*= Vec3F()*/)
    {
        // Pure z rotation reproduces the 2D Basis::Build math exactly, keeping 2D content bit-identical
        if (eulerAngles.x == 0.0f && eulerAngles.y == 0.0f)
        {
            float sn = sinf(eulerAngles.z), cs = cosf(eulerAngles.z);
            float sshift = Math::Sqrt(1.0f - shear.x*shear.x);
            Vec2F x2(scale.x*cs, sn*scale.x), y2(-sn*scale.y, cs*scale.y);
            y2 = y2.Rotate(sshift, -shear.x);

            float shearZZ = Math::Sqrt(Math::Max(0.0f, 1.0f - shear.y*shear.y - shear.z*shear.z));
            Vec3F z(Vec2F(shear.y, shear.z).Rotate(cs, sn)*scale.z, shearZZ*scale.z);

            return Basis3D(position, Vec3F(x2, 0.0f), Vec3F(y2, 0.0f), z);
        }

        return Build(position, scale, Quat::FromEuler(eulerAngles), shear);
    }

    inline AABB AABB::Transformed(const Basis3D& basis) const
    {
        Vec3F corners[8];
        for (int i = 0; i < 8; i++)
        {
            Vec3F corner((i & 1) ? max.x : min.x,
                         (i & 2) ? max.y : min.y,
                         (i & 4) ? max.z : min.z);

            corners[i] = basis.Transform(corner);
        }

        return Bound(corners, 8);
    }
}
