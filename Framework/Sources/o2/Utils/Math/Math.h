#pragma once

#include <float.h>
#include <math.h>
#include <random>
#include <stdlib.h>
#include "o2/Utils/Types/CommonTypes.h"

namespace o2
{
    template<typename T> class Rect;
    template<typename T> class Vec2;
    typedef Vec2<float> Vec2F;
    typedef Rect<float> RectF;

    namespace Math
    {
		const float Epsilon = FLT_EPSILON;

        float Floor(float value);
        int FloorToInt(float value);

        float Ceil(float value);
        int CeilToInt(float value);

        float Round(float value);
        Vec2F Round(const Vec2F& value);
        RectF Round(const RectF& value);
        int RoundToInt(float value);

        float Mod(float val, float x);

        float Pow(float value, float s);

        template<typename T>
        T Sign(const T& value);

        template<typename T>
        T Clamp(const T& value, const T& minv, const T& maxv);

        float Clamp01(float value);

        template<typename T>
        T Min(const T& v1, const T& v2);

        template<typename T>
        T Max(const T& v1, const T& v2);

        template<typename T>
        void Swap(T& v1, T& v2);

        template<typename T>
        T Abs(const T& value);

        template<typename T>
        T Random(const T& minValue = 0, const T& maxValue = 1);
        
        UInt64 Random();

        // Random value in 0..1 from the current source: the scoped generator if any, else rand()
        float RandomUnit();

        // Routes Math::Random through its own deterministic generator while alive; the global rand() stays
        // untouched. Nested scopes restore the previous one
        class RandomScope
        {
        public:
            explicit RandomScope(UInt seed);
            ~RandomScope();

            float Next();

        private:
            UInt         mState;
            RandomScope* mPrevious;
        };

        template<typename T>
        T Lerp(const T& a, const T& b, float coef);

        template<typename T>
        T Lerpc(const T& a, const T& b, float coef);

        RectF Lerp(const RectF& a, const RectF& b, float coef);
        bool Lerp(bool a, bool b, float coef);
        int Lerp(int a, int b, float coef);

        bool Equals(float a, float b, float range = FLT_EPSILON);

        template<typename T>
        bool Equals(const T& a, const T& b);

        template<typename T>
        bool Fewer(const T& a, const T& b);

        template<typename T>
        bool Greater(const T& a, const T& b);

        float Sqrt(float value);

        template<typename T>
        T Sqr(T value);

        float PI();
        float Deg2rad(const float& value);
        float Rad2deg(const float& value);

        float Sin(float rad);
        float Cos(float rad);
        float ASin(float value);
        float ACos(float value);
        float Atan2F(float x, float y);

        // Wraps angle in radians to (-pi, pi]
        float WrapAngle(float rad);

        // Builds a orthographic projection matrix
        void OrthoProjMatrix(float* mat, float left, float right, float bottom, float top, float nearz, float farz);

        // Builds a perspective projection matrix
        void PerspectiveProjMatrix(float* mat, float fovYRad, float aspect, float nearz, float farz);

        // Calculates tangent vector on ellipse that located on axis begin-end at point middle
        Vec2F CalculateEllipseTangent(const Vec2F& begin, const Vec2F& middle, const Vec2F& end);
        
        // Multiplicates matrixes 4x4
        void mtxMultiply(float* ret, const float* lhs, const float* rhs);
        
        template<typename T>
        T Sign(const T& value)
        {
            if (value < 0)
                return (T)(-1);

            return (T)1;
        }

        template<typename T>
        T Clamp(const T& value, const T& minv, const T& maxv)
        {
            if (value < minv)
                return minv;

            if (value > maxv)
                return maxv;

            return value;
        }

        template<typename T>
        T Min(const T& v1, const T& v2)
        {
            return std::min(v1, v2);
        }

        template<typename T>
        T Max(const T& v1, const T& v2)
        {
            return std::max(v1, v2);
        }

        template<typename T>
        void Swap(T& v1, T& v2)
        {
            std::swap(v1, v2);
        }
        
        template<typename T>
        T Abs(const T& value)
        {
            return std::abs(value);
        }

        template<typename T>
        T Random(const T& minValue /*= 0*/, const T& maxValue /*= 1*/)
        {
            return (T)(RandomUnit()*(float)(maxValue - minValue) + (float)minValue);
        }

        template<typename T>
        T Lerp(const T& a, const T& b, float coef)
        {
            return (b - a)*coef + a;
        }

        template<typename T>
        T Lerpc(const T& a, const T& b, float coef)
        {
            return (b - a)*Clamp01(coef) + a;
        }

        template<typename T>
        bool Equals(const T& a, const T& b)
        {
            return a == b;
        }

        template<typename T>
        bool Fewer(const T& a, const T& b)
        {
            return a < b;
        }

        template<typename T>
        bool Greater(const T& a, const T& b)
        {
            return a > b;
        }

        template<typename T>
        T Sqr(T value)
        {
            return value*value;
        }
    }

} //namespace o2
