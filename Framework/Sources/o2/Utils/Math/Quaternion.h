#pragma once

#include "o2/Utils/Math/Math.h"
#include "o2/Utils/Math/Vector3.h"

namespace o2
{
    class Quat
    {
    public:
        float x;
        float y;
        float z;
        float w;

    public:
        inline Quat();
        inline Quat(float vx, float vy, float vz, float vw);

        inline bool operator==(const Quat& v) const;
        inline bool operator!=(const Quat& v) const;

        inline Quat operator*(const Quat& v) const;
        inline Quat operator*=(const Quat& v);

        inline Vec3F operator*(const Vec3F& v) const;

        inline float Dot(const Quat& v) const;

        inline float Length() const;
        inline float SqrLength() const;

        inline Quat Normalized() const;
        inline void Normalize();

        inline Quat Inverted() const;

        // Euler angles order: rotation around X, then Y, then Z (v' = Rz*Ry*Rx*v)
        inline Vec3F ToEuler() const;

        static inline Quat Identity();
        static inline Quat FromAxisAngle(const Vec3F& axis, float rad);
        static inline Quat FromEuler(const Vec3F& radians);

        // Returns the shortest arc rotation mapping direction from to direction to
        static inline Quat FromToRotation(const Vec3F& from, const Vec3F& to);

        static inline Quat LookRotation(const Vec3F& forward, const Vec3F& up = Vec3F::YAxis());
        static inline Quat Slerp(const Quat& a, const Quat& b, float coef);
    };

    Quat::Quat():
        x(0), y(0), z(0), w(1.0f)
    {}

    Quat::Quat(float vx, float vy, float vz, float vw):
        x(vx), y(vy), z(vz), w(vw)
    {}

    bool Quat::operator==(const Quat& v) const
    {
        return Math::Abs(x - v.x) < 0.001f && Math::Abs(y - v.y) < 0.001f &&
               Math::Abs(z - v.z) < 0.001f && Math::Abs(w - v.w) < 0.001f;
    }

    bool Quat::operator!=(const Quat& v) const
    {
        return !(*this == v);
    }

    Quat Quat::operator*(const Quat& v) const
    {
        return Quat(w*v.x + x*v.w + y*v.z - z*v.y,
                    w*v.y + y*v.w + z*v.x - x*v.z,
                    w*v.z + z*v.w + x*v.y - y*v.x,
                    w*v.w - x*v.x - y*v.y - z*v.z);
    }

    Quat Quat::operator*=(const Quat& v)
    {
        *this = *this*v;
        return *this;
    }

    Vec3F Quat::operator*(const Vec3F& v) const
    {
        Vec3F qv(x, y, z);
        Vec3F t = qv.Cross(v)*2.0f;
        return v + t*w + qv.Cross(t);
    }

    float Quat::Dot(const Quat& v) const
    {
        return x*v.x + y*v.y + z*v.z + w*v.w;
    }

    float Quat::Length() const
    {
        return Math::Sqrt(x*x + y*y + z*z + w*w);
    }

    float Quat::SqrLength() const
    {
        return x*x + y*y + z*z + w*w;
    }

    Quat Quat::Normalized() const
    {
        float ln = Length();
        if (ln > 0)
        {
            float t = 1.0f/ln;
            return Quat(x*t, y*t, z*t, w*t);
        }

        return Quat();
    }

    void Quat::Normalize()
    {
        *this = this->Normalized();
    }

    Quat Quat::Inverted() const
    {
        return Quat(-x, -y, -z, w);
    }

    Vec3F Quat::ToEuler() const
    {
        Vec3F res;

        float sinPitch = 2.0f*(w*y - z*x);
        if (Math::Abs(sinPitch) >= 1.0f - 1e-6f)
        {
            // Gimbal lock: at pitch +-90 degrees roll and yaw collapse into one angle, kept in z
            res.x = 0.0f;
            res.y = sinPitch > 0.0f ? Math::PI()*0.5f : -Math::PI()*0.5f;
            res.z = 2.0f*Math::Atan2F(z, w);
            return res;
        }

        res.x = Math::Atan2F(2.0f*(w*x + y*z), 1.0f - 2.0f*(x*x + y*y));
        res.y = Math::ASin(sinPitch);
        res.z = Math::Atan2F(2.0f*(w*z + x*y), 1.0f - 2.0f*(y*y + z*z));

        return res;
    }

    Quat Quat::Identity()
    {
        return Quat(0, 0, 0, 1.0f);
    }

    Quat Quat::FromAxisAngle(const Vec3F& axis, float rad)
    {
        Vec3F n = axis.Normalized();
        float sn = Math::Sin(rad*0.5f);
        return Quat(n.x*sn, n.y*sn, n.z*sn, Math::Cos(rad*0.5f));
    }

    Quat Quat::FromEuler(const Vec3F& radians)
    {
        float cx = Math::Cos(radians.x*0.5f), sx = Math::Sin(radians.x*0.5f);
        float cy = Math::Cos(radians.y*0.5f), sy = Math::Sin(radians.y*0.5f);
        float cz = Math::Cos(radians.z*0.5f), sz = Math::Sin(radians.z*0.5f);

        return Quat(cz*cy*sx - sz*sy*cx,
                    cz*sy*cx + sz*cy*sx,
                    sz*cy*cx - cz*sy*sx,
                    cz*cy*cx + sz*sy*sx);
    }

    Quat Quat::FromToRotation(const Vec3F& from, const Vec3F& to)
    {
        Vec3F f = from.Normalized();
        Vec3F t = to.Normalized();

        float dot = f.Dot(t);
        if (dot >= 1.0f - 1e-6f)
            return Identity();

        if (dot <= -1.0f + 1e-6f)
        {
            // Opposite directions: half turn around any perpendicular axis
            Vec3F axis = f.Cross(Vec3F::XAxis());
            if (axis.SqrLength() < 1e-6f)
                axis = f.Cross(Vec3F::YAxis());

            return FromAxisAngle(axis.Normalized(), Math::PI());
        }

        Vec3F cross = f.Cross(t);
        return Quat(cross.x, cross.y, cross.z, 1.0f + dot).Normalized();
    }

    Quat Quat::LookRotation(const Vec3F& forward, const Vec3F& up /*= Vec3F::YAxis()*/)
    {
        Vec3F zaxis = forward.Normalized();
        Vec3F xaxis = up.Cross(zaxis).Normalized();
        Vec3F yaxis = zaxis.Cross(xaxis);

        float trace = xaxis.x + yaxis.y + zaxis.z;
        Quat res;

        if (trace > 0.0f)
        {
            float s = Math::Sqrt(trace + 1.0f)*2.0f;
            res = Quat((yaxis.z - zaxis.y)/s, (zaxis.x - xaxis.z)/s, (xaxis.y - yaxis.x)/s, s*0.25f);
        }
        else if (xaxis.x > yaxis.y && xaxis.x > zaxis.z)
        {
            float s = Math::Sqrt(1.0f + xaxis.x - yaxis.y - zaxis.z)*2.0f;
            res = Quat(s*0.25f, (yaxis.x + xaxis.y)/s, (zaxis.x + xaxis.z)/s, (yaxis.z - zaxis.y)/s);
        }
        else if (yaxis.y > zaxis.z)
        {
            float s = Math::Sqrt(1.0f + yaxis.y - xaxis.x - zaxis.z)*2.0f;
            res = Quat((yaxis.x + xaxis.y)/s, s*0.25f, (zaxis.y + yaxis.z)/s, (zaxis.x - xaxis.z)/s);
        }
        else
        {
            float s = Math::Sqrt(1.0f + zaxis.z - xaxis.x - yaxis.y)*2.0f;
            res = Quat((zaxis.x + xaxis.z)/s, (zaxis.y + yaxis.z)/s, s*0.25f, (xaxis.y - yaxis.x)/s);
        }

        return res.Normalized();
    }

    Quat Quat::Slerp(const Quat& a, const Quat& b, float coef)
    {
        Quat end = b;
        float dot = a.Dot(b);

        if (dot < 0.0f)
        {
            dot = -dot;
            end = Quat(-b.x, -b.y, -b.z, -b.w);
        }

        if (dot > 0.9995f)
        {
            return Quat(Math::Lerp(a.x, end.x, coef), Math::Lerp(a.y, end.y, coef),
                        Math::Lerp(a.z, end.z, coef), Math::Lerp(a.w, end.w, coef)).Normalized();
        }

        float angle = Math::ACos(dot);
        float sn = Math::Sin(angle);
        float ca = Math::Sin((1.0f - coef)*angle)/sn;
        float cb = Math::Sin(coef*angle)/sn;

        return Quat(a.x*ca + end.x*cb, a.y*ca + end.y*cb, a.z*ca + end.z*cb, a.w*ca + end.w*cb);
    }
}
