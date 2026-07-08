#pragma once

#include "o2/Utils/Math/Math.h"
#include "o2/Utils/Math/Vector2.h"

namespace o2
{
    template<typename T>
    class Vec3
    {
    public:
        T x;
        T y;
        T z;

    public:
        inline Vec3();
        inline Vec3(T vx, T vy, T vz);
        inline Vec3(const Vec2<T>& v, T vz = 0);

        template<typename RT>
        inline operator Vec3<RT>() const;

        inline bool operator==(const Vec3& v) const;
        inline bool operator!=(const Vec3& v) const;

        inline Vec3 operator-() const;

        inline Vec3 operator+(const Vec3& v) const;
        inline Vec3 operator+=(const Vec3& v);

        inline Vec3 operator-(const Vec3& v) const;
        inline Vec3 operator-=(const Vec3& v);

        inline Vec3 operator*(const Vec3& v) const;
        inline Vec3 operator*=(const Vec3& v);

        inline Vec3 operator*(T v) const;
        inline Vec3 operator*=(T v);

        inline Vec3 operator/(const Vec3& v) const;
        inline Vec3 operator/=(const Vec3& v);

        inline Vec3 operator/(T v) const;
        inline Vec3 operator/=(T v);

        inline T& operator[](int idx);

        inline T&   Get(int i);
        inline void Set(T vx, T vy, T vz);

        inline T Dot(const Vec3& v) const;

        inline Vec3 Cross(const Vec3& v) const;

        inline T Length() const;
        inline T SqrLength() const;

        inline Vec3 Normalized() const;
        inline void Normalize();

        inline Vec3 Inverted(bool bx = true, bool by = true, bool bz = true) const;
        inline Vec3 InvertedX() const;
        inline Vec3 InvertedY() const;
        inline Vec3 InvertedZ() const;

        inline Vec2<T> XY() const;

        static inline Vec3 Lerp(const Vec3& a, const Vec3& b, float coef);

        static inline Vec3 Zero();
        static inline Vec3 One();
        static inline Vec3 XAxis();
        static inline Vec3 YAxis();
        static inline Vec3 ZAxis();

        // Returns unit axis by index: 0 - X, 1 - Y, any other - Z
        static inline Vec3 Axis(int index);

        static inline T Length(const Vec3& a, const Vec3& b);
        static inline T SqrLength(const Vec3& a, const Vec3& b);
    };

    typedef Vec3<float> Vec3F;
    typedef Vec3<int>   Vec3I;

    template<typename T>
    Vec3<T>::Vec3():
        x(0), y(0), z(0)
    {}

    template<typename T>
    Vec3<T>::Vec3(T vx, T vy, T vz):
        x(vx), y(vy), z(vz)
    {}

    template<typename T>
    Vec3<T>::Vec3(const Vec2<T>& v, T vz /*= 0*/):
        x(v.x), y(v.y), z(vz)
    {}

    template<typename T>
    template<typename RT>
    Vec3<T>::operator Vec3<RT>() const
    {
        return Vec3<RT>((RT)x, (RT)y, (RT)z);
    }

    template<typename T>
    bool Vec3<T>::operator==(const Vec3<T>& v) const
    {
        return Math::Abs(x - v.x) < 0.001f && Math::Abs(y - v.y) < 0.001f && Math::Abs(z - v.z) < 0.001f;
    }

    template<typename T>
    bool Vec3<T>::operator!=(const Vec3<T>& v) const
    {
        return !(*this == v);
    }

    template<typename T>
    Vec3<T> Vec3<T>::operator-() const
    {
        return Vec3(-x, -y, -z);
    }

    template<typename T>
    Vec3<T> Vec3<T>::operator+(const Vec3<T>& v) const
    {
        return Vec3(x + v.x, y + v.y, z + v.z);
    }

    template<typename T>
    Vec3<T> Vec3<T>::operator+=(const Vec3<T>& v)
    {
        x += v.x; y += v.y; z += v.z;
        return *this;
    }

    template<typename T>
    Vec3<T> Vec3<T>::operator-(const Vec3<T>& v) const
    {
        return Vec3(x - v.x, y - v.y, z - v.z);
    }

    template<typename T>
    Vec3<T> Vec3<T>::operator-=(const Vec3<T>& v)
    {
        x -= v.x; y -= v.y; z -= v.z;
        return *this;
    }

    template<typename T>
    Vec3<T> Vec3<T>::operator*(const Vec3<T>& v) const
    {
        return Vec3(x*v.x, y*v.y, z*v.z);
    }

    template<typename T>
    Vec3<T> Vec3<T>::operator*=(const Vec3<T>& v)
    {
        x *= v.x; y *= v.y; z *= v.z;
        return *this;
    }

    template<typename T>
    Vec3<T> Vec3<T>::operator*(T v) const
    {
        return Vec3(x*v, y*v, z*v);
    }

    template<typename T>
    Vec3<T> Vec3<T>::operator*=(T v)
    {
        x *= v; y *= v; z *= v;
        return *this;
    }

    template<typename T>
    Vec3<T> Vec3<T>::operator/(const Vec3<T>& v) const
    {
        return Vec3(x/v.x, y/v.y, z/v.z);
    }

    template<typename T>
    Vec3<T> Vec3<T>::operator/=(const Vec3<T>& v)
    {
        x /= v.x; y /= v.y; z /= v.z;
        return *this;
    }

    template<typename T>
    Vec3<T> Vec3<T>::operator/(T v) const
    {
        float t = 1.0f/v;
        return Vec3((T)(x*t), (T)(y*t), (T)(z*t));
    }

    template<typename T>
    Vec3<T> Vec3<T>::operator/=(T v)
    {
        *this = *this/v;
        return *this;
    }

    template<typename T>
    T& Vec3<T>::operator[](int idx)
    {
        if (idx == 0)
            return x;

        if (idx == 1)
            return y;

        return z;
    }

    template<typename T>
    T& Vec3<T>::Get(int i)
    {
        if (i == 0) return x;
        if (i == 1) return y;
        return z;
    }

    template<typename T>
    void Vec3<T>::Set(T vx, T vy, T vz)
    {
        x = vx; y = vy; z = vz;
    }

    template<typename T>
    T Vec3<T>::Dot(const Vec3<T>& v) const
    {
        return x*v.x + y*v.y + z*v.z;
    }

    template<typename T>
    Vec3<T> Vec3<T>::Cross(const Vec3<T>& v) const
    {
        return Vec3(y*v.z - z*v.y, z*v.x - x*v.z, x*v.y - y*v.x);
    }

    template<typename T>
    T Vec3<T>::Length() const
    {
        return Math::Sqrt(x*x + y*y + z*z);
    }

    template<typename T>
    T Vec3<T>::SqrLength() const
    {
        return x*x + y*y + z*z;
    }

    template<typename T>
    Vec3<T> Vec3<T>::Normalized() const
    {
        T ln = Length();
        if (ln > 0) return *this/ln;
        return Vec3(0, 0, 0);
    }

    template<typename T>
    void Vec3<T>::Normalize()
    {
        *this = this->Normalized();
    }

    template<typename T>
    Vec3<T> Vec3<T>::Inverted(bool bx /*= true*/, bool by /*= true*/, bool bz /*= true*/) const
    {
        Vec3 r = *this;
        if (bx) r.x = -r.x;
        if (by) r.y = -r.y;
        if (bz) r.z = -r.z;
        return r;
    }

    template<typename T>
    Vec3<T> Vec3<T>::InvertedX() const
    {
        Vec3 r = *this;
        r.x = -r.x;
        return r;
    }

    template<typename T>
    Vec3<T> Vec3<T>::InvertedY() const
    {
        Vec3 r = *this;
        r.y = -r.y;
        return r;
    }

    template<typename T>
    Vec3<T> Vec3<T>::InvertedZ() const
    {
        Vec3 r = *this;
        r.z = -r.z;
        return r;
    }

    template<typename T>
    Vec2<T> Vec3<T>::XY() const
    {
        return Vec2<T>(x, y);
    }

    template<typename T>
    Vec3<T> Vec3<T>::Lerp(const Vec3<T>& a, const Vec3<T>& b, float coef)
    {
        return (b - a)*coef + a;
    }

    template<typename T>
    Vec3<T> Vec3<T>::Zero()
    {
        return Vec3<T>(0, 0, 0);
    }

    template<typename T>
    Vec3<T> Vec3<T>::One()
    {
        return Vec3<T>(1, 1, 1);
    }

    template<typename T>
    Vec3<T> Vec3<T>::XAxis()
    {
        return Vec3<T>(1, 0, 0);
    }

    template<typename T>
    Vec3<T> Vec3<T>::YAxis()
    {
        return Vec3<T>(0, 1, 0);
    }

    template<typename T>
    Vec3<T> Vec3<T>::ZAxis()
    {
        return Vec3<T>(0, 0, 1);
    }

    template<typename T>
    Vec3<T> Vec3<T>::Axis(int index)
    {
        switch (index)
        {
            case 0: return XAxis();
            case 1: return YAxis();
            default: return ZAxis();
        }
    }

    template<typename T>
    T Vec3<T>::Length(const Vec3& a, const Vec3& b)
    {
        return (b - a).Length();
    }

    template<typename T>
    T Vec3<T>::SqrLength(const Vec3& a, const Vec3& b)
    {
        return (b - a).SqrLength();
    }
}
