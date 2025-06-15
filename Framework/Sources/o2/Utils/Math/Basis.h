#pragma once

#include "o2/Utils/Math/Vector2.h"
#include "o2/Utils/Math/Rect.h"

namespace o2
{
    // -------------------------
    // Basis. Just as matrix 2x3
    // -------------------------
    struct Basis
    {
        Vec2F xv, yv, origin;

        inline Basis();
        inline Basis(const Vec2F& origin, const Vec2F& xvec = Vec2F(1, 0), const Vec2F& yvec = Vec2F(0, 1));
        inline Basis(const Vec2F& origin, float angle);
        inline Basis(const RectF& rect);

        inline bool operator==(const Basis& cbasis) const;
        inline bool operator!=(const Basis& cbasis) const;

        inline Basis& operator=(const RectF& other);

        inline Basis operator*(const Basis& cbasis) const;
        inline Vec2F operator*(const Vec2F& vec) const;
        inline Vec2F operator/(const Vec2F& vec) const;

        inline void Set(const Vec2F& origin = Vec2F(0, 0), const Vec2F& xvec = Vec2F(1, 0), const Vec2F& yvec = Vec2F(0, 1));
        inline void Set(const Vec2F& origin, float angle);

        inline float GetAngle() const;
        inline Vec2F GetScale() const;
        inline float GetShift() const;
        inline float GetShiftFast(const Vec2F& scale) const;

        inline void Decompose(Vec2F* origin, float* angle, Vec2F* scale, float* shift) const;

        inline Basis Inverted() const;
        inline void  Inverse();

        inline void Translate(const Vec2F& voffs);
        inline void Scale(const Vec2F& scalev);
        inline void Rotate(float angle);

        inline void  Transform(float& x, float& y) const;
        inline Vec2F Transform(const Vec2F& vec) const;

        inline RectF AABB() const;

        inline bool IsPointInside(const Vec2F& point) const;

        inline static Basis Identity();
        inline static Basis Scaled(const Vec2F& scale);
        inline static Basis Translated(const Vec2F& voffs);
        inline static Basis Rotated(float angle);
        inline static Basis Build(const Vec2F& position, const Vec2F& scale, float angle, float shift);
    };

    struct BasisDef
    {
        Vec2F mPosition;
        Vec2F mScale;
        float mAngle;
        float mShift;

    public:
        inline BasisDef(const Vec2F& position = Vec2F(), const Vec2F& scale = Vec2F(1, 1), float angle = 0, float shift = 0);
        inline BasisDef(const Basis& bas);

        inline operator Basis() const;

        inline Basis Build() const;
    };

    inline BasisDef Basis2Def(const Basis& bas);
    inline Basis    Def2Basis(const BasisDef& def);

    inline Vec2F operator*(const Vec2F& vec, const Basis& basis)
    {
        return basis.Transform(vec);
    }

    Basis::Basis() :
        xv(1, 0), yv(0, 1), origin()
    {}

    Basis::Basis(const Vec2F& origin, const Vec2F& xvec /*= vec2f(1, 0)*/, const Vec2F& yvec /*= vec2f(0, 1)*/) :
        xv(xvec), yv(yvec), origin(origin)
    {}

    Basis::Basis(const Vec2F& origin, float angle) :
        origin(origin)
    {
        float cs = cosf(angle), sn = sinf(angle);
        xv.Set(cs, sn);
        yv.Set(-sn, cs);
    }

    Basis::Basis(const RectF& rect) :
        origin(rect.LeftBottom()), xv(rect.Width(), 0.0f), yv(0.0f, rect.Height())
    {}

    bool Basis::operator==(const Basis& cbasis) const
    {
        return xv == cbasis.xv && yv == cbasis.yv && origin == cbasis.origin;
    }

    bool Basis::operator!=(const Basis& cbasis) const
    {
        return xv != cbasis.xv || yv != cbasis.yv || origin != cbasis.origin;
    }

    Basis& Basis::operator=(const RectF& other)
    {
        origin = other.LeftBottom();
        xv.Set(other.Width(), 0.0f);
        yv.Set(0.0f, other.Height());
        return *this;
    }

    Basis Basis::operator*(const Basis& cbasis) const
    {
        Basis res;
        res.xv.x = xv.x*cbasis.xv.x + xv.y*cbasis.yv.x;                               res.xv.y = xv.x*cbasis.xv.y + xv.y*cbasis.yv.y;
        res.yv.x = yv.x*cbasis.xv.x + yv.y*cbasis.yv.x;                               res.yv.y = yv.x*cbasis.xv.y + yv.y*cbasis.yv.y;
        res.origin.x = origin.x*cbasis.xv.x + origin.y*cbasis.yv.x + cbasis.origin.x; res.origin.y = origin.x*cbasis.xv.y + origin.y*cbasis.yv.y + cbasis.origin.y;
        return res;
    }

    Vec2F Basis::operator*(const Vec2F& vec) const
    {
        Vec2F ret;
        ret.x = xv.x*vec.x + yv.x*vec.y + origin.x;
        ret.y = xv.y*vec.x + yv.y*vec.y + origin.y;
        return ret;
    }

    Vec2F Basis::operator/(const Vec2F& vec) const
    {
        return operator*(vec.Inverted());
    }

    void Basis::Set(const Vec2F& origin /*= vec2f(0, 0)*/, const Vec2F& xvec /*= vec2f(1, 0)*/, const Vec2F& yvec /*= vec2f(0, 1)*/)
    {
        xv = xvec; yv = yvec;
        this->origin = origin;
    }

    void Basis::Set(const Vec2F& origin, float angle)
    {
        this->origin = origin;
        float cs = cosf(angle), sn = sinf(angle);
        xv.Set(cs, sn);
        yv.Set(-sn, cs);
    }

    float Basis::GetAngle() const
    {
        float angle = -atan2f(-xv.y, xv.x);
        if (angle < 0)
            return 6.283185307f + angle;

        return angle;
    }

    Vec2F Basis::GetScale() const
    {
        return Vec2F(xv.Length(), yv.Length());
    }

    float Basis::GetShift() const
    {
        Vec2F scale = GetScale();
        return GetShiftFast(scale);
    }

    float Basis::GetShiftFast(const Vec2F& scale) const
    {
        if (Math::Abs(scale.x) < Math::Epsilon || Math::Abs(scale.y) < Math::Epsilon)
            return 0.0f;

        float proj = yv.Dot(xv/scale.x);
        return proj/scale.y;
    }

    void Basis::Decompose(Vec2F* origin, float* angle, Vec2F* scale, float* shift) const
    {
        *origin = this->origin;
        *angle = GetAngle();
        *scale = GetScale();
        *shift = GetShiftFast(*scale);
    }

    Basis Basis::Inverted() const
    {
        float invdet = 1.0f/(xv.x*yv.y - yv.x*xv.y);
        Basis res;

        res.xv.x = yv.y*invdet;
        res.yv.x = -yv.x*invdet;
        res.origin.x = (yv.x*origin.y - origin.x*yv.y)*invdet;

        res.xv.y = -xv.y*invdet;
        res.yv.y = xv.x*invdet;
        res.origin.y = -(xv.x*origin.y - origin.x*xv.y)*invdet;

        return res;
    }

    void Basis::Inverse()
    {
        *this = Inverted();
    }

    void Basis::Translate(const Vec2F& voffs)
    {
        origin += voffs;
    }

    void Basis::Scale(const Vec2F& scalev)
    {
        xv *= scalev.x;
        yv *= scalev.y;
    }

    void Basis::Rotate(float angle)
    {
        float cs = cosf(angle), sn = sinf(angle);

        Vec2F nxv(cs*xv.x - sn*xv.y, sn*xv.x + cs*xv.y);
        Vec2F nyv(cs*yv.x - sn*yv.y, sn*yv.x + cs*yv.y);

        xv = nxv; yv = nyv;
    }

    void Basis::Transform(float& x, float& y) const
    {
        float lx = x, ly = y;
        x = xv.x*lx + yv.x*ly + origin.x;
        y = xv.y*lx + yv.y*ly + origin.y;
    }

    Vec2F Basis::Transform(const Vec2F& vec) const
    {
        return Vec2F(xv.x*vec.x + yv.x*vec.y + origin.x, xv.y*vec.x + yv.y*vec.y + origin.y);
    }

    RectF Basis::AABB() const
    {
        Vec2F points[4] =
        {
            origin, origin + xv, origin + yv, origin + xv + yv
        };

        return RectF::Bound(points, 4);
    }

    bool Basis::IsPointInside(const Vec2F& point) const
    {
        Vec2F rs(xv.Length(), yv.Length());
        Vec2F nx = xv / rs.x, ny = yv / rs.y;
        Vec2F lp = point - origin;

        float dx = lp.Dot(nx);
        float dy = lp.Dot(ny);

        return dx >= 0.0f && dx <= rs.x && dy >= 0.0f && dy < rs.y;
    }

    Basis Basis::Identity()
    {
        return Basis(Vec2F(0, 0), Vec2F(1, 0), Vec2F(0, 1));
    }

    Basis Basis::Scaled(const Vec2F& scale)
    {
        return Basis(Vec2F(0, 0), Vec2F(scale.x, 0), Vec2F(0, scale.y));
    }

    Basis Basis::Translated(const Vec2F& voffs)
    {
        return Basis(voffs, Vec2F(1, 0), Vec2F(0, 1));
    }

    Basis Basis::Rotated(float angle)
    {
        float cs = cosf(angle), sn = sinf(angle);
        return Basis(Vec2F(0, 0), Vec2F(cs, sn), Vec2F(-sn, cs));
    }

    Basis Basis::Build(const Vec2F& position, const Vec2F& scale, float angle, float shift)
    {
        float sn = sinf(angle), cs = cosf(angle), sshift = Math::Sqrt(1.0f - shift*shift);
        Vec2F x(scale.x*cs, sn*scale.x), y(-sn*scale.y, cs*scale.y);
        y = y.Rotate(sshift, -shift);
        return Basis(position, x, y);
    }


    BasisDef::BasisDef(const Vec2F& position /*= vec2f()*/, const Vec2F& scale /*= vec2f(1, 1)*/, float angle /*= 0*/,
                       float shift /*= 0*/) :
        mPosition(position), mScale(scale), mAngle(angle), mShift(shift)
    {}

    BasisDef::BasisDef(const Basis& bas)
    {
        bas.Decompose(&mPosition, &mAngle, &mScale, &mShift);
    }

    BasisDef::operator Basis() const
    {
        return Build();
    }

    Basis BasisDef::Build() const
    {
        return Basis::Build(mPosition, mScale, mAngle, mShift);
    }


    BasisDef Basis2Def(const Basis& bas)
    {
        return BasisDef(bas);
    }

    Basis Def2Basis(const BasisDef& def)
    {
        return def.Build();
    }
}
