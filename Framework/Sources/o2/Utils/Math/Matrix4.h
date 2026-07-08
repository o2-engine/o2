#pragma once

#include "o2/Utils/Math/Math.h"
#include "o2/Utils/Math/Quaternion.h"
#include "o2/Utils/Math/Vector3.h"

namespace o2
{
    // Column-major 4x4 matrix, same layout as Math::OrthoProjMatrix and Math::mtxMultiply
    class Mat4
    {
    public:
        float m[16];

    public:
        inline Mat4();

        inline bool operator==(const Mat4& v) const;
        inline bool operator!=(const Mat4& v) const;

        inline Mat4 operator*(const Mat4& v) const;
        inline Mat4 operator*=(const Mat4& v);

        inline float& At(int row, int col);
        inline float At(int row, int col) const;

        inline Vec3F TransformPoint(const Vec3F& v) const;
        inline Vec3F TransformDirection(const Vec3F& v) const;

        inline Mat4 Transposed() const;
        inline Mat4 Inverted() const;

        inline void Decompose(Vec3F& pos, Quat& rot, Vec3F& scale) const;

        inline const float* Data() const;

        static inline Mat4 Identity();
        static inline Mat4 Translation(const Vec3F& v);
        static inline Mat4 Rotation(const Quat& q);
        static inline Mat4 Scaling(const Vec3F& v);
        static inline Mat4 TRS(const Vec3F& pos, const Quat& rot, const Vec3F& scale);
        static inline Mat4 Ortho(float left, float right, float bottom, float top, float nearz, float farz);
        static inline Mat4 Perspective(float fovYRad, float aspect, float nearz, float farz);
        static inline Mat4 LookAt(const Vec3F& eye, const Vec3F& target, const Vec3F& up = Vec3F::YAxis());
    };

    Mat4::Mat4()
    {
        for (int i = 0; i < 16; i++)
            m[i] = 0.0f;

        m[0] = m[5] = m[10] = m[15] = 1.0f;
    }

    bool Mat4::operator==(const Mat4& v) const
    {
        for (int i = 0; i < 16; i++)
        {
            if (Math::Abs(m[i] - v.m[i]) > 0.001f)
                return false;
        }

        return true;
    }

    bool Mat4::operator!=(const Mat4& v) const
    {
        return !(*this == v);
    }

    Mat4 Mat4::operator*(const Mat4& v) const
    {
        Mat4 res;

        for (int c = 0; c < 4; c++)
        {
            for (int r = 0; r < 4; r++)
            {
                res.m[c*4 + r] = m[r]*v.m[c*4] + m[4 + r]*v.m[c*4 + 1] +
                                 m[8 + r]*v.m[c*4 + 2] + m[12 + r]*v.m[c*4 + 3];
            }
        }

        return res;
    }

    Mat4 Mat4::operator*=(const Mat4& v)
    {
        *this = *this*v;
        return *this;
    }

    float& Mat4::At(int row, int col)
    {
        return m[col*4 + row];
    }

    float Mat4::At(int row, int col) const
    {
        return m[col*4 + row];
    }

    Vec3F Mat4::TransformPoint(const Vec3F& v) const
    {
        Vec3F res(m[0]*v.x + m[4]*v.y + m[8]*v.z + m[12],
                  m[1]*v.x + m[5]*v.y + m[9]*v.z + m[13],
                  m[2]*v.x + m[6]*v.y + m[10]*v.z + m[14]);

        float w = m[3]*v.x + m[7]*v.y + m[11]*v.z + m[15];
        if (Math::Abs(w) > FLT_EPSILON && Math::Abs(w - 1.0f) > FLT_EPSILON)
            res /= w;

        return res;
    }

    Vec3F Mat4::TransformDirection(const Vec3F& v) const
    {
        return Vec3F(m[0]*v.x + m[4]*v.y + m[8]*v.z,
                     m[1]*v.x + m[5]*v.y + m[9]*v.z,
                     m[2]*v.x + m[6]*v.y + m[10]*v.z);
    }

    Mat4 Mat4::Transposed() const
    {
        Mat4 res;

        for (int c = 0; c < 4; c++)
        {
            for (int r = 0; r < 4; r++)
                res.m[c*4 + r] = m[r*4 + c];
        }

        return res;
    }

    Mat4 Mat4::Inverted() const
    {
        Mat4 res;
        float inv[16];

        inv[0] = m[5]*m[10]*m[15] - m[5]*m[11]*m[14] - m[9]*m[6]*m[15] +
                 m[9]*m[7]*m[14] + m[13]*m[6]*m[11] - m[13]*m[7]*m[10];

        inv[4] = -m[4]*m[10]*m[15] + m[4]*m[11]*m[14] + m[8]*m[6]*m[15] -
                 m[8]*m[7]*m[14] - m[12]*m[6]*m[11] + m[12]*m[7]*m[10];

        inv[8] = m[4]*m[9]*m[15] - m[4]*m[11]*m[13] - m[8]*m[5]*m[15] +
                 m[8]*m[7]*m[13] + m[12]*m[5]*m[11] - m[12]*m[7]*m[9];

        inv[12] = -m[4]*m[9]*m[14] + m[4]*m[10]*m[13] + m[8]*m[5]*m[14] -
                  m[8]*m[6]*m[13] - m[12]*m[5]*m[10] + m[12]*m[6]*m[9];

        inv[1] = -m[1]*m[10]*m[15] + m[1]*m[11]*m[14] + m[9]*m[2]*m[15] -
                 m[9]*m[3]*m[14] - m[13]*m[2]*m[11] + m[13]*m[3]*m[10];

        inv[5] = m[0]*m[10]*m[15] - m[0]*m[11]*m[14] - m[8]*m[2]*m[15] +
                 m[8]*m[3]*m[14] + m[12]*m[2]*m[11] - m[12]*m[3]*m[10];

        inv[9] = -m[0]*m[9]*m[15] + m[0]*m[11]*m[13] + m[8]*m[1]*m[15] -
                 m[8]*m[3]*m[13] - m[12]*m[1]*m[11] + m[12]*m[3]*m[9];

        inv[13] = m[0]*m[9]*m[14] - m[0]*m[10]*m[13] - m[8]*m[1]*m[14] +
                  m[8]*m[2]*m[13] + m[12]*m[1]*m[10] - m[12]*m[2]*m[9];

        inv[2] = m[1]*m[6]*m[15] - m[1]*m[7]*m[14] - m[5]*m[2]*m[15] +
                 m[5]*m[3]*m[14] + m[13]*m[2]*m[7] - m[13]*m[3]*m[6];

        inv[6] = -m[0]*m[6]*m[15] + m[0]*m[7]*m[14] + m[4]*m[2]*m[15] -
                 m[4]*m[3]*m[14] - m[12]*m[2]*m[7] + m[12]*m[3]*m[6];

        inv[10] = m[0]*m[5]*m[15] - m[0]*m[7]*m[13] - m[4]*m[1]*m[15] +
                  m[4]*m[3]*m[13] + m[12]*m[1]*m[7] - m[12]*m[3]*m[5];

        inv[14] = -m[0]*m[5]*m[14] + m[0]*m[6]*m[13] + m[4]*m[1]*m[14] -
                  m[4]*m[2]*m[13] - m[12]*m[1]*m[6] + m[12]*m[2]*m[5];

        inv[3] = -m[1]*m[6]*m[11] + m[1]*m[7]*m[10] + m[5]*m[2]*m[11] -
                 m[5]*m[3]*m[10] - m[9]*m[2]*m[7] + m[9]*m[3]*m[6];

        inv[7] = m[0]*m[6]*m[11] - m[0]*m[7]*m[10] - m[4]*m[2]*m[11] +
                 m[4]*m[3]*m[10] + m[8]*m[2]*m[7] - m[8]*m[3]*m[6];

        inv[11] = -m[0]*m[5]*m[11] + m[0]*m[7]*m[9] + m[4]*m[1]*m[11] -
                  m[4]*m[3]*m[9] - m[8]*m[1]*m[7] + m[8]*m[3]*m[5];

        inv[15] = m[0]*m[5]*m[10] - m[0]*m[6]*m[9] - m[4]*m[1]*m[10] +
                  m[4]*m[2]*m[9] + m[8]*m[1]*m[6] - m[8]*m[2]*m[5];

        float det = m[0]*inv[0] + m[1]*inv[4] + m[2]*inv[8] + m[3]*inv[12];
        if (Math::Abs(det) < FLT_EPSILON)
            return Mat4();

        float t = 1.0f/det;
        for (int i = 0; i < 16; i++)
            res.m[i] = inv[i]*t;

        return res;
    }

    void Mat4::Decompose(Vec3F& pos, Quat& rot, Vec3F& scale) const
    {
        pos = Vec3F(m[12], m[13], m[14]);

        Vec3F xaxis(m[0], m[1], m[2]);
        Vec3F yaxis(m[4], m[5], m[6]);
        Vec3F zaxis(m[8], m[9], m[10]);

        scale = Vec3F(xaxis.Length(), yaxis.Length(), zaxis.Length());

        if (xaxis.Cross(yaxis).Dot(zaxis) < 0.0f)
            scale.x = -scale.x;

        if (Math::Abs(scale.x) > FLT_EPSILON) xaxis /= scale.x;
        if (Math::Abs(scale.y) > FLT_EPSILON) yaxis /= scale.y;
        if (Math::Abs(scale.z) > FLT_EPSILON) zaxis /= scale.z;

        float trace = xaxis.x + yaxis.y + zaxis.z;

        if (trace > 0.0f)
        {
            float s = Math::Sqrt(trace + 1.0f)*2.0f;
            rot = Quat((yaxis.z - zaxis.y)/s, (zaxis.x - xaxis.z)/s, (xaxis.y - yaxis.x)/s, s*0.25f);
        }
        else if (xaxis.x > yaxis.y && xaxis.x > zaxis.z)
        {
            float s = Math::Sqrt(1.0f + xaxis.x - yaxis.y - zaxis.z)*2.0f;
            rot = Quat(s*0.25f, (yaxis.x + xaxis.y)/s, (zaxis.x + xaxis.z)/s, (yaxis.z - zaxis.y)/s);
        }
        else if (yaxis.y > zaxis.z)
        {
            float s = Math::Sqrt(1.0f + yaxis.y - xaxis.x - zaxis.z)*2.0f;
            rot = Quat((yaxis.x + xaxis.y)/s, s*0.25f, (zaxis.y + yaxis.z)/s, (zaxis.x - xaxis.z)/s);
        }
        else
        {
            float s = Math::Sqrt(1.0f + zaxis.z - xaxis.x - yaxis.y)*2.0f;
            rot = Quat((zaxis.x + xaxis.z)/s, (zaxis.y + yaxis.z)/s, s*0.25f, (xaxis.y - yaxis.x)/s);
        }

        rot.Normalize();
    }

    const float* Mat4::Data() const
    {
        return m;
    }

    Mat4 Mat4::Identity()
    {
        return Mat4();
    }

    Mat4 Mat4::Translation(const Vec3F& v)
    {
        Mat4 res;
        res.m[12] = v.x;
        res.m[13] = v.y;
        res.m[14] = v.z;
        return res;
    }

    Mat4 Mat4::Rotation(const Quat& q)
    {
        Mat4 res;

        float xx = q.x*q.x, yy = q.y*q.y, zz = q.z*q.z;
        float xy = q.x*q.y, xz = q.x*q.z, yz = q.y*q.z;
        float wx = q.w*q.x, wy = q.w*q.y, wz = q.w*q.z;

        res.m[0] = 1.0f - 2.0f*(yy + zz);
        res.m[1] = 2.0f*(xy + wz);
        res.m[2] = 2.0f*(xz - wy);

        res.m[4] = 2.0f*(xy - wz);
        res.m[5] = 1.0f - 2.0f*(xx + zz);
        res.m[6] = 2.0f*(yz + wx);

        res.m[8] = 2.0f*(xz + wy);
        res.m[9] = 2.0f*(yz - wx);
        res.m[10] = 1.0f - 2.0f*(xx + yy);

        return res;
    }

    Mat4 Mat4::Scaling(const Vec3F& v)
    {
        Mat4 res;
        res.m[0] = v.x;
        res.m[5] = v.y;
        res.m[10] = v.z;
        return res;
    }

    Mat4 Mat4::TRS(const Vec3F& pos, const Quat& rot, const Vec3F& scale)
    {
        return Translation(pos)*Rotation(rot)*Scaling(scale);
    }

    Mat4 Mat4::Ortho(float left, float right, float bottom, float top, float nearz, float farz)
    {
        Mat4 res;
        Math::OrthoProjMatrix(res.m, left, right, bottom, top, nearz, farz);
        return res;
    }

    Mat4 Mat4::Perspective(float fovYRad, float aspect, float nearz, float farz)
    {
        Mat4 res;
        Math::PerspectiveProjMatrix(res.m, fovYRad, aspect, nearz, farz);
        return res;
    }

    Mat4 Mat4::LookAt(const Vec3F& eye, const Vec3F& target, const Vec3F& up /*= Vec3F::YAxis()*/)
    {
        Vec3F f = (target - eye).Normalized();
        Vec3F s = f.Cross(up).Normalized();
        Vec3F u = s.Cross(f);

        Mat4 res;

        res.m[0] = s.x; res.m[4] = s.y; res.m[8] = s.z;   res.m[12] = -s.Dot(eye);
        res.m[1] = u.x; res.m[5] = u.y; res.m[9] = u.z;   res.m[13] = -u.Dot(eye);
        res.m[2] = -f.x; res.m[6] = -f.y; res.m[10] = -f.z; res.m[14] = f.Dot(eye);
        res.m[3] = 0.0f; res.m[7] = 0.0f; res.m[11] = 0.0f; res.m[15] = 1.0f;

        return res;
    }
}
