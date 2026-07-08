#include "o2/stdafx.h"
#include "o2/Utils/Math/Math.h"

#include "o2/Utils/Math/Vector2.h"
#include "o2/Utils/Math/Rect.h"

namespace o2
{
    namespace Math
    {

        float Floor(float value)
        {
            return floorf(value);
        }

        float Ceil(float value)
        {
            return ceilf(value);
        }

        Vec2F Round(const Vec2F& value)
        {
            return Vec2F(Round(value.x), Round(value.y));
        }

        RectF Round(const RectF& value)
        {
            return RectF(Round(value.left), Round(value.top), Round(value.right), Round(value.bottom));
        }

        float Round(float value)
        {
            return roundf(value);
        }

        float Pow(float value, float s)
        {
            return powf(value, s);
        }

        int FloorToInt(float value)
        {
            return (int)floorf(value);
        }

        int CeilToInt(float value)
        {
            return (int)ceilf(value);
        }

        float Clamp01(float value)
        {
            return Clamp(value, 0.0f, 1.0f);
        }

        int RoundToInt(float value)
        {
            return (int)roundf(value);
        }

        float Mod(float val, float x)
        {

            return fmodf(val, x);
        }

        RectF Lerp(const RectF& a, const RectF& b, float coef)
        {
            return RectF(Lerp(a.left, b.left, coef), Lerp(a.top, b.top, coef),
                         Lerp(a.right, b.right, coef), Lerp(a.bottom, b.bottom, coef));
        }

        bool Lerp(bool a, bool b, float coef)
        {
            return coef < 0.99f ? a : b;
        }

        int Lerp(int a, int b, float coef)
        {
            return (int)((float)(b - a)*coef) + a;
        }

        bool Equals(float a, float b, float range /*= FLT_EPSILON*/)
        {
            float x = a - b;
            return x*x < range*range;
        }

        float Sqrt(float value)
        {
            return sqrtf(value);
        }

        float PI()
        {
            return 3.1415926535897932384626433832795f;
        }

        float Deg2rad(const float& value)
        {
            return value*(PI() / 180.0f);
        }

        float Rad2deg(const float& value)
        {
            return value*(180.0f / PI());
        }

        float Sin(float rad)
        {
            return sinf(rad);
        }

        float Cos(float rad)
        {
            return cosf(rad);
        }

        float ASin(float value)
        {
            return asinf(value);
        }

        float ACos(float value)
        {
            return acosf(value);
        }

        float Atan2F(float x, float y)
        {
            return atan2f(x, y);
        }

        float WrapAngle(float rad)
        {
            while (rad > PI())
                rad -= 2.0f*PI();

            while (rad <= -PI())
                rad += 2.0f*PI();

            return rad;
        }

        void OrthoProjMatrix(float* mat, float left, float right, float bottom, float top, float nearz, float farz)
        {
            float tx = -(right + left)/(right - left);
            float ty = -(top + bottom)/(top - bottom);
            float tz = -(farz + nearz)/(farz - nearz);

            mat[0] = 2.0f/(right - left); mat[4] = 0.0f;                mat[8] = 0.0f;                  mat[12] = tx;
            mat[1] = 0.0f;                mat[5] = 2.0f/(top - bottom); mat[9] = 0.0f;                  mat[13] = ty;
            mat[2] = 0.0f;                mat[6] = 0.0f;                mat[10] = -2.0f/(farz - nearz); mat[14] = tz;
            mat[3] = 0.0f;                mat[7] = 0.0f;                mat[11] = 0.0f;                 mat[15] = 1.0f;
        }

        void PerspectiveProjMatrix(float* mat, float fovYRad, float aspect, float nearz, float farz)
        {
            float f = 1.0f/tanf(fovYRad*0.5f);
            float tz = (farz + nearz)/(nearz - farz);
            float tw = 2.0f*farz*nearz/(nearz - farz);

            mat[0] = f/aspect; mat[4] = 0.0f; mat[8] = 0.0f;   mat[12] = 0.0f;
            mat[1] = 0.0f;     mat[5] = f;    mat[9] = 0.0f;   mat[13] = 0.0f;
            mat[2] = 0.0f;     mat[6] = 0.0f; mat[10] = tz;    mat[14] = tw;
            mat[3] = 0.0f;     mat[7] = 0.0f; mat[11] = -1.0f; mat[15] = 0.0f;
        }

        Vec2F CalculateEllipseTangent(const Vec2F& begin, const Vec2F& middle, const Vec2F& end)
        {
            Vec2F axis = end - begin;
            float axisLength = axis.Length();
            Vec2F axisNorm = axis/axisLength;
            Vec2F axisNormPerpendicular = axisNorm.Perpendicular();

            Vec2F centerToMiddle = middle - (begin + end)*0.5f;

            float xProjection = axisNorm.Dot(centerToMiddle + (begin + end)*0.5f - begin);
            float xProjectionFromCenter = xProjection - axisLength*0.5f;

            float ssq = axisLength*axisLength*0.25f - xProjectionFromCenter*xProjectionFromCenter;
            float circleYProjection = ssq > 0 ? Math::Sqrt(ssq) : 0.00001f;

            Vec2F circlePoint = axisNorm*xProjection + axisNormPerpendicular*circleYProjection;
            Vec2F centerToCirclePoint = circlePoint - (axis*0.5f);
            Vec2F circleTangent = centerToCirclePoint.Perpendicular().Inverted().Normalized();
            float yc = axisNormPerpendicular.Dot(centerToMiddle);

            Vec2F decomposedCircleTangent(axisNorm.Dot(circleTangent), axisNormPerpendicular.Dot(circleTangent));
            float circleToEllipseScale = (yc/circleYProjection);
            Vec2F ellipseTangent = (axisNorm*decomposedCircleTangent.x + axisNormPerpendicular*decomposedCircleTangent.y*circleToEllipseScale).Normalized();

            return ellipseTangent;
        }

        UInt64 Random()
        {
            static std::default_random_engine generator;
            static std::uniform_int_distribution<unsigned long long> distribution;
            return distribution(generator);
        }

        void mtxMultiply(float* ret, const float* lhs, const float* rhs)
        {
            // [ 0 4  8 12 ]   [ 0 4  8 12 ]
            // [ 1 5  9 13 ] x [ 1 5  9 13 ]
            // [ 2 6 10 14 ]   [ 2 6 10 14 ]
            // [ 3 7 11 15 ]   [ 3 7 11 15 ]
            ret[0] = lhs[0] * rhs[0] + lhs[4] * rhs[1] + lhs[8] * rhs[2] + lhs[12] * rhs[3];
            ret[1] = lhs[1] * rhs[0] + lhs[5] * rhs[1] + lhs[9] * rhs[2] + lhs[13] * rhs[3];
            ret[2] = lhs[2] * rhs[0] + lhs[6] * rhs[1] + lhs[10] * rhs[2] + lhs[14] * rhs[3];
            ret[3] = lhs[3] * rhs[0] + lhs[7] * rhs[1] + lhs[11] * rhs[2] + lhs[15] * rhs[3];

            ret[4] = lhs[0] * rhs[4] + lhs[4] * rhs[5] + lhs[8] * rhs[6] + lhs[12] * rhs[7];
            ret[5] = lhs[1] * rhs[4] + lhs[5] * rhs[5] + lhs[9] * rhs[6] + lhs[13] * rhs[7];
            ret[6] = lhs[2] * rhs[4] + lhs[6] * rhs[5] + lhs[10] * rhs[6] + lhs[14] * rhs[7];
            ret[7] = lhs[3] * rhs[4] + lhs[7] * rhs[5] + lhs[11] * rhs[6] + lhs[15] * rhs[7];

            ret[8] = lhs[0] * rhs[8] + lhs[4] * rhs[9] + lhs[8] * rhs[10] + lhs[12] * rhs[11];
            ret[9] = lhs[1] * rhs[8] + lhs[5] * rhs[9] + lhs[9] * rhs[10] + lhs[13] * rhs[11];
            ret[10] = lhs[2] * rhs[8] + lhs[6] * rhs[9] + lhs[10] * rhs[10] + lhs[14] * rhs[11];
            ret[11] = lhs[3] * rhs[8] + lhs[7] * rhs[9] + lhs[11] * rhs[10] + lhs[15] * rhs[11];

            ret[12] = lhs[0] * rhs[12] + lhs[4] * rhs[13] + lhs[8] * rhs[14] + lhs[12] * rhs[15];
            ret[13] = lhs[1] * rhs[12] + lhs[5] * rhs[13] + lhs[9] * rhs[14] + lhs[13] * rhs[15];
            ret[14] = lhs[2] * rhs[12] + lhs[6] * rhs[13] + lhs[10] * rhs[14] + lhs[14] * rhs[15];
            ret[15] = lhs[3] * rhs[12] + lhs[7] * rhs[13] + lhs[11] * rhs[14] + lhs[15] * rhs[15];
        }
    }
} 
