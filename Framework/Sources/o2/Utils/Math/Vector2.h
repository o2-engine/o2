#pragma once

#include "o2/Utils/Math/Math.h"
#include "Box2D/Common/b2Math.h"

namespace o2
{
    class DataValue;

    template<typename T>
    class Vec2
    {
    public:
        T x;
        T y;

    public:
        inline Vec2();
        inline Vec2(T vx, T vy);

        template<typename RT>
        inline operator Vec2<RT>();

        inline Vec2(const b2Vec2& v);
        inline operator b2Vec2() const;

        inline bool operator==(const Vec2& v) const;
        inline bool operator!=(const Vec2& v) const;

        inline Vec2 operator+(const Vec2& v) const;
        inline Vec2 operator+=(const Vec2& v);

        inline Vec2 operator-(const Vec2& v) const;
        inline Vec2 operator-=(const Vec2& v);

        inline Vec2 operator*(const Vec2& v) const;
        inline Vec2 operator*=(const Vec2& v);

        inline Vec2 operator*(T v) const;
        inline Vec2 operator*=(T v);

        inline Vec2 operator/(const Vec2& v) const;
        inline Vec2 operator/=(const Vec2& v);

        inline Vec2 operator/(T v) const;
        inline Vec2 operator/=(T v);

        inline Vec2 operator^(T s) const;
        inline T    operator^(const Vec2& v) const;

        inline T&   operator[](int idx);

        inline Vec2 Scale(T scale) const;
        inline Vec2 Scale(const Vec2& scaleVec) const;
        inline Vec2 Scale(const Vec2& scaleVec, const Vec2& origin) const;
        inline Vec2 InvScale(const Vec2& scale) const;

        inline T&   Get(int i);
        inline void Set(T vx, T vy);

        inline T Dot(const Vec2& v) const;

        inline T    Cross(const Vec2& v) const;
        inline Vec2 Cross(float v) const;

        inline T Length() const;
        inline T SqrLength() const;

        inline Vec2 Normalized() const;
        inline void Normalize();

        inline Vec2 Perpendicular() const;

        inline Vec2 Rotate(float rad) const;
        inline Vec2 Rotate(float cs, float sn) const;
        inline void Rotate(float rad, const Vec2& origin);

        inline Vec2 Inverted(bool bx = true, bool by = true) const;
        inline Vec2 InvertedX() const;
        inline Vec2 InvertedY() const;

        inline float Angle(const Vec2& other) const;
        inline float SignedAngle(const Vec2& other) const;
        inline Vec2 Project(const Vec2& other) const;
        inline Vec2 ClampLength(T newLength) const;

        inline bool IsParallel(const Vec2& other) const;

        static inline Vec2<T> Rotated(float rad);
        static inline Vec2<T> Up();
        static inline Vec2<T> Down();
        static inline Vec2<T> Left();
        static inline Vec2<T> Right();
        static inline Vec2<T> One();

        static inline T Length(const Vec2& a, const Vec2& b);
        static inline T SqrLength(const Vec2& a, const Vec2& b);
        static inline T Angle(const Vec2& a, const Vec2& b);
        static inline T SignedAngle(const Vec2& a, const Vec2& b);
    };

    typedef Vec2<float> Vec2F;
    typedef Vec2<int>   Vec2I;

    template<typename T>
    Vec2<T>::Vec2():
        x(0), y(0)
    {}

    template<typename T>
    Vec2<T>::Vec2(T vx, T vy) :
        x(vx), y(vy)
    {}

    template<typename T>
    template<typename RT>
    Vec2<T>::operator Vec2<RT>()
    {
        return Vec2<RT>((RT)x, (RT)y);
    }

    template<typename T>
    Vec2<T>::Vec2(const b2Vec2& v):
        x(v.x), y(v.y)
    {}

    template<typename T>
    Vec2<T>::operator b2Vec2() const
    {
        return b2Vec2((float32)x, (float32)y);
    }

    template<typename T>
    Vec2<T> Vec2<T>::operator+(const Vec2<T>& v) const
    {
        return Vec2(x + v.x, y + v.y);
    }

    template<typename T>
    Vec2<T> Vec2<T>::operator+=(const Vec2<T>& v)
    {
        x += v.x; y += v.y;
        return *this;
    }

    template<typename T>
    Vec2<T> Vec2<T>::operator-(const Vec2<T>& v) const
    {
        return Vec2(x - v.x, y - v.y);
    }

    template<typename T>
    Vec2<T> Vec2<T>::operator-=(const Vec2<T>& v)
    {
        x -= v.x; y -= v.y;
        return *this;
    }

    template<typename T>
    Vec2<T> Vec2<T>::operator*(T v) const
    {
        return Vec2(x*v, y*v);
    }

    template<typename T>
    Vec2<T> Vec2<T>::operator*=(T v)
    {
        x *= v; y *= v;
        return *this;
    }

    template<typename T>
    Vec2<T> Vec2<T>::operator/(T v) const
    {
        float t = 1.0f/v;
        return Vec2((T)x*t, (T)y*t);
    }

    template<typename T>
    Vec2<T> Vec2<T>::operator/(const Vec2<T>& v) const
    {
        return Vec2(x/v.x, y/v.y);
    }

    template<typename T>
    Vec2<T> Vec2<T>::operator/=(const Vec2& v)
    {
        x /= v.x; y /= v.y;
    }

    template<typename T>
    Vec2<T> Vec2<T>::operator/=(T v)
    {
        *this = *this/v;
        return *this;
    }

    template<typename T>
    Vec2<T> Vec2<T>::operator*(const Vec2<T>& v) const
    {
        return Vec2(x*v.x, y*v.y);
    }

    template<typename T>
    Vec2<T> Vec2<T>::operator*=(const Vec2<T>& v)
    {
        x *= v.x; y *= v.y;
        return *this;
    }

    template<typename T>
    bool Vec2<T>::operator==(const Vec2<T>& v) const
    {
        return Math::Abs(x - v.x) < 0.001f && Math::Abs(y - v.y) < 0.001f;
    }

    template<typename T>
    bool Vec2<T>::operator!=(const Vec2<T>& v) const
    {
        return Math::Abs(x - v.x) > 0.001f || Math::Abs(y - v.y) > 0.001f;
    }

    template<typename T>
    Vec2<T> Vec2<T>::operator^(T s) const
    {
        return Vec2(-s*y, s*x);
    }

    template<typename T>
    T Vec2<T>::operator^(const Vec2<T>& v) const
    {
        return x*v.y - y*v.x;
    }

    template<typename T>
    T& Vec2<T>::operator[](int idx)
    {
        if (idx == 0)
            return x;

        return y;
    }

    template<typename T>
    Vec2<T> Vec2<T>::Scale(T scale) const
    {
        return Vec2(x*scale, y*scale);
    }

    template<typename T>
    Vec2<T> Vec2<T>::Scale(const Vec2<T>& scaleVec) const
    {
        return Vec2(x*scaleVec.x, y*scaleVec.y);
    }

    template<typename T>
    Vec2<T> Vec2<T>::Scale(const Vec2<T>& scaleVec, const Vec2<T>& origin) const
    {
        return (*this - origin).Scale(scaleVec) + origin;
    }

    template<typename T>
    Vec2<T> Vec2<T>::InvScale(const Vec2<T>& v) const
    {
        return Vec2(x/v.x, y/v.y);
    }

    template<typename T>
    T& Vec2<T>::Get(int i)
    {
        if (i == 0) return x;
        return y;
    }

    template<typename T>
    void Vec2<T>::Set(T vx, T vy)
    {
        x = vx; y = vy;
    }

    template<typename T>
    T Vec2<T>::Dot(const Vec2<T>& v) const
    {
        return x*v.x + y*v.y;
    }

    template<typename T>
    Vec2<T> Vec2<T>::Cross(float v) const
    {
        return Vec2(-v*y, v*x);
    }

    template<typename T>
    T Vec2<T>::Cross(const Vec2& v) const
    {
        return x*v.y - y*v.x;
    }

    template<typename T>
    T Vec2<T>::Length() const
    {
        return Math::Sqrt(x*x + y*y);
    }

    template<typename T>
    T Vec2<T>::SqrLength() const
    {
        return x*x + y*y;
    }

    template<typename T>
    Vec2<T> Vec2<T>::Normalized() const
    {
        T ln = Length();
        if (ln > 0) return *this/ln;
        return Vec2(0, 0);
    }

    template<typename T>
    void Vec2<T>::Normalize()
    {
        *this = this->Normalized();
    }

    template<typename T>
    Vec2<T> Vec2<T>::Perpendicular() const
    {
        return Vec2(-y, x);
    }

    template<typename T>
    Vec2<T> Vec2<T>::Rotate(float rad) const
    {
        float cs = Math::Cos(rad);
        float sn = Math::Sin(rad);

        return Vec2((T)(cs*x - sn*y), (T)(cs*y + sn*x));
    }

    template<typename T>
    Vec2<T> Vec2<T>::Rotate(float cs, float sn) const
    {
        return Vec2((T)(cs*x - sn*y), (T)(cs*y + sn*x));
    }

    template<typename T>
    void Vec2<T>::Rotate(float rad, const Vec2& c)
    {
        *this -= c;
        Rotate(rad);
        *this += c;
    }

    template<typename T>
    Vec2<T> Vec2<T>::Inverted(bool bx /*= true*/, bool by /*= true*/)  const
    {
        Vec2 r = *this;
        if (bx) r.x = -r.x;
        if (by) r.y = -r.y;
        return r;
    }

    template<typename T>
    Vec2<T> Vec2<T>::InvertedX() const
    {
        Vec2 r = *this;
        r.x = -r.x;
        return r;
    }

    template<typename T>
    Vec2<T> Vec2<T>::InvertedY() const
    {
        Vec2 r = *this;
        r.y = -r.y;
        return r;
    }

    template<typename T>
    Vec2<T> Vec2<T>::ClampLength(T newLength) const
    {
        float sc = newLength/Length();
        return Scale(sc);
    }

    template<typename T>
    bool Vec2<T>::IsParallel(const Vec2<T>& other) const
    {
        return Math::Abs<T>(Perpendicular().Normalized().Dot(other)) < (T)0.01f;
    }

    template<typename T>
    Vec2<T> Vec2<T>::Project(const Vec2& other) const
    {
        Vec2<T> n = other.Normalized();
        return n*(n.Dot(*this));
    }

    template<typename T>
    float Vec2<T>::Angle(const Vec2& other) const
    {
        float rad = Math::Atan2F(Cross(other) + FLT_EPSILON, Dot(other));
        if (rad < 0)
            return Math::PI()*2.0f + rad;

        return rad;
    }

    template<typename T>
    float Vec2<T>::SignedAngle(const Vec2& other) const
    {
        return Math::Atan2F(Cross(other) + FLT_EPSILON, Dot(other));;
    }

    template<typename T>
    Vec2<T> Vec2<T>::Rotated(float rad)
    {
        return Vec2<T>(Math::Cos(rad), Math::Sin(rad));
    }

    template<typename T>
    Vec2<T> Vec2<T>::Up()
    {
        return Vec2<T>(0, 1.0f);
    }

    template<typename T>
    Vec2<T> Vec2<T>::Down()
    {
        return Vec2<T>(0, -1.0f);
    }

    template<typename T>
    Vec2<T> Vec2<T>::Left()
    {
        return Vec2<T>(-1.0f, 0);
    }

    template<typename T>
    Vec2<T> Vec2<T>::Right()
    {
        return Vec2<T>(1.0f, 0);
    }

    template<typename T>
    Vec2<T> Vec2<T>::One()
    {
        return Vec2<T>(1.0f, 1.0f);
    }

    template<typename T>
    T Vec2<T>::Angle(const Vec2& a, const Vec2& b)
    {
        return a.Angle(b);
    }

    template<typename T>
    T Vec2<T>::SignedAngle(const Vec2& a, const Vec2& b)
    {
        return a.SignedAngle(b);
    }

    template<typename T>
    T Vec2<T>::SqrLength(const Vec2& a, const Vec2& b)
    {
        return (b - a).SqrLength();
    }

    template<typename T>
    T Vec2<T>::Length(const Vec2& a, const Vec2& b)
    {
        return (b - a).Length();
    }
}
