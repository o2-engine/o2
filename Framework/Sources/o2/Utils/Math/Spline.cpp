#include "o2/stdafx.h"
#include "Spline.h"

#include "o2/Utils/Math/Interpolation.h"
#include "o2/Utils/Tools/KeySearch.h"

namespace o2
{
    Spline::Spline()
    {}

    Spline::Spline(const Vector<Vec2F>& values, bool smooth /*= true*/)
    {
        AppendKeys(values, smooth);
    }

    Spline::Spline(const Spline& other) :
        mKeys(other.mKeys)
    { }

    bool Spline::operator!=(const Spline& other) const
    {
        return !(*this == other);
    }

    bool Spline::operator==(const Spline& other) const
    {
        if (mKeys.Count() != other.mKeys.Count())
            return false;

        for (int i = 0; i < mKeys.Count(); i++)
        {
            if (mKeys[i] != other.mKeys[i])
                return false;
        }

        return true;
    }

    Spline& Spline::operator=(const Spline& other)
    {
        mKeys = other.mKeys;

        UpdateApproximation();

        onKeysChanged();

        return *this;
    }

    Spline& Spline::operator+=(const Spline& other)
    {
        AppendSpline(other);
        return *this;
    }

    Vec2F Spline::Evaluate(float position, float randomRangeCoef /*= 0.0f*/) const
    {
        int cacheKey = 0, cacheKeyApprox = 0;
        return Evaluate(position, randomRangeCoef, true, cacheKey, cacheKeyApprox);
    }

    Vec2F Spline::Evaluate(float position, float randomRangeCoef, bool direction, int& cacheKey, int& cacheKeyApprox) const
    {
        int count = mKeys.Count();

        if (count == 1)
            return mKeys[0].value;
        else if (count == 0)
            return Vec2F();

        int prevCacheKey = cacheKey;
        int keyLeftIdx = -1, keyRightIdx = -1;

        if (mClosed)
            SearchKeyClosed(mKeys, count, position, keyLeftIdx, keyRightIdx, direction, cacheKey);
        else
            SearchKey(mKeys, count, position, keyLeftIdx, keyRightIdx, direction, cacheKey);

        if (keyLeftIdx < 0)
            return Vec2F();

        const Key& rightKey = mKeys[keyRightIdx];

        int segBeginIdx = 0;
        int segEndIdx = 1;

        if (keyLeftIdx != prevCacheKey)
            cacheKeyApprox = 0;

        SearchKey(rightKey.mLeftApproxValues, Key::mApproxValuesCount, position, segBeginIdx, segEndIdx, direction, cacheKeyApprox);

        const ApproximationVec2F& segBeginLeft = rightKey.mLeftApproxValues[segBeginIdx];
        const ApproximationVec2F& segEndLeft = rightKey.mLeftApproxValues[segEndIdx];

        const ApproximationVec2F& segBeginRight = rightKey.mRightApproxValues[segBeginIdx];
        const ApproximationVec2F& segEndRight = rightKey.mRightApproxValues[segEndIdx];

        float dist = segEndLeft.position - segBeginLeft.position;
        float coef = (position - segBeginLeft.position) / dist;

        return Math::Lerp(Math::Lerp(segBeginLeft.value, segEndLeft.value, coef),
                          Math::Lerp(segBeginRight.value, segEndRight.value, coef), randomRangeCoef);
    }

    void Spline::BeginKeysBatchChange()
    {
        mBatchChange = true;
    }

    void Spline::CompleteKeysBatchingChange()
    {
        UpdateApproximation();
        mBatchChange = false;
        mChangedKeys = false;
    }

    void Spline::SetClosed(bool closed)
    {
        mClosed = closed;

        if (mBatchChange)
            mChangedKeys = true;
        else
            UpdateApproximation();
    }

    bool Spline::IsClosed() const
    {
        return mClosed;
    }

    void Spline::AppendSpline(const Spline& Spline)
    {
        AppendKeys(Spline.mKeys);
    }

    void Spline::PrependSpline(const Spline& Spline)
    {
        PrependKeys(Spline.mKeys);
    }

    void Spline::InsertSpline(const Spline& Spline, int idx)
    {
        InsertKeys(Spline.mKeys, idx);
    }

    void Spline::AppendKeys(const Vector<Vec2F>& values, bool smooth /*= true*/)
    {
        AppendKeys(values.Convert<Key>([=](const Vec2F& v) {
            Key res(v, 0.0f, Vec2F(), Vec2F());
            res.supportsType = smooth ? Key::Type::Smooth : Key::Type::Broken;
            return res;
        }));
    }

    void Spline::AppendKeys(const Vector<Key>& keys)
    {
        if (keys.IsEmpty())
            return;

        mKeys.Add(keys);
        CheckSmoothKeys();

        if (mBatchChange)
            mChangedKeys = true;
        else
            UpdateApproximation();
    }

    void Spline::PrependKeys(const Vector<Vec2F>& values, bool smooth /*= true*/)
    {
        PrependKeys(values.Convert<Key>([=](const Vec2F& v) {
            Key res(v, 0.0f, Vec2F(), Vec2F());
            res.supportsType = smooth ? Key::Type::Smooth : Key::Type::Broken;
            return res;
        }));
    }

    void Spline::PrependKeys(const Vector<Key>& keys)
    {
        if (keys.IsEmpty())
            return;

        mKeys.Insert(keys, 0);

        CheckSmoothKeys();

        if (mBatchChange)
            mChangedKeys = true;
        else
            UpdateApproximation();
    }

    void Spline::InsertKeys(const Vector<Vec2F>& values, int idx, bool smooth /*= true*/)
    {
        InsertKeys(values.Convert<Key>([=](const Vec2F& v) {
            Key res(v, 0.0f, Vec2F(), Vec2F());
            res.supportsType = smooth ? Key::Type::Smooth : Key::Type::Broken;
            return res;
        }), idx);
    }

    void Spline::InsertKeys(const Vector<Key>& keys, int idx)
    {
        if (keys.IsEmpty())
            return;

        mKeys.Insert(keys, idx);

        CheckSmoothKeys();

        if (mBatchChange)
            mChangedKeys = true;
        else
            UpdateApproximation();
    }

    void Spline::InsertKey(const Key& key, int idx)
    {
        mKeys.Insert(key, idx);

        CheckSmoothKeys();

        if (mBatchChange)
            mChangedKeys = true;
        else
            UpdateApproximation();
    }

    void Spline::InsertKey(int idx, const Vec2F& position, float rangeValue, const Vec2F& prevSupport, const Vec2F& nextSupport)
    {
        InsertKey(Key(position, rangeValue, prevSupport, nextSupport), idx);
    }

    void Spline::InsertKey(int idx, const Vec2F& position, float rangeValue, float smoothCoef /*= 1.0f*/)
    {
        
        mKeys.Insert(Key(position, rangeValue, Vec2F(), Vec2F()), idx);
        SmoothKey(idx, smoothCoef);

        if (mBatchChange)
            mChangedKeys = true;
        else
            UpdateApproximation();
    }

    int Spline::AppendKey(const Vec2F& position, float rangeValue, const Vec2F& prevSupport, const Vec2F& nextSupport)
    {
        Key newKey(position, rangeValue, prevSupport, nextSupport);

        newKey.supportsType = Key::Type::Broken;

        mKeys.Add(newKey);
        CheckSmoothKeys();

        if (mBatchChange)
            mChangedKeys = true;
        else
            UpdateApproximation();

        return mKeys.Count() - 1;
    }

    int Spline::AppendKey(const Vec2F& position, float rangeValue /*= 0.0f*/, float smoothCoef /*= 1.0f*/)
    {
        Key newKey(position, rangeValue, Vec2F(), Vec2F());
        newKey.supportsType = Key::Type::Smooth;

        mKeys.Add(newKey);
        CheckSmoothKeys();

        if (mBatchChange)
            mChangedKeys = true;
        else
            UpdateApproximation();

        return mKeys.Count() - 1;
    }

    int Spline::PrependKey(const Vec2F& position, float rangeValue, const Vec2F& prevSupport, const Vec2F& nextSupport)
    {
        Key newKey(position, rangeValue, prevSupport, nextSupport);
        newKey.supportsType = Key::Type::Broken;

        mKeys.Add(newKey);
        CheckSmoothKeys();

        if (mBatchChange)
            mChangedKeys = true;
        else
            UpdateApproximation();

        return 0;
    }

    int Spline::PrependKey(const Vec2F& position, float rangeValue /*= 0.0f*/, float smoothCoef /*= 1.0f*/)
    {
        Key newKey(position, rangeValue, Vec2F(), Vec2F());
        newKey.supportsType = Key::Type::Smooth;

        mKeys.Add(newKey);
        CheckSmoothKeys();

        if (mBatchChange)
            mChangedKeys = true;
        else
            UpdateApproximation();

        return 0;
    }

    Spline::Key Spline::GetKey(int idx) const
    {
        if (idx < 0 || idx > mKeys.Count() - 1)
            return Key();

        return mKeys[idx];
    }

    Spline::Key Spline::FindKey(UInt64 uid) const
    {
        for (auto& key : mKeys)
        {
            if (key.uid == uid)
                return key;
        }

        return Key();
    }

    int Spline::GetNextKey(float position) const
    {
        for (int i = 0; i < mKeys.Count(); i++)
        {
            if (mKeys[i].position > position)
                return i;
        }

        return 0;
    }

    int Spline::FindKeyIdx(UInt64 uid) const
    {
        int idx = 0;
        for (auto& key : mKeys)
        {
            if (key.uid == uid)
                return idx;

            idx++;
        }

        return -1;
    }

    bool Spline::RemoveKey(int idx)
    {
        if (idx < 0 || idx >= mKeys.Count())
            return false;

        mKeys.RemoveAt(idx);

        if (mBatchChange)
            mChangedKeys = true;
        else
            UpdateApproximation();

        return true;
    }

    void Spline::RemoveAllKeys()
    {
        mKeys.Clear();
        onKeysChanged();
    }

    const Vector<Spline::Key>& Spline::GetKeys() const
    {
        return mKeys;
    }

    void Spline::SetKeys(const Vector<Key>& keys)
    {
        mKeys = keys;
        CheckSmoothKeys();

        if (mBatchChange)
            mChangedKeys = true;
        else
            UpdateApproximation();
    }

    void Spline::SetKey(const Key& key, int idx)
    {
        if (idx < 0 || idx > mKeys.Count() - 1)
            return;

        mKeys[idx] = key;
        CheckSmoothKeys();

        if (mBatchChange)
            mChangedKeys = true;
        else
            UpdateApproximation();
    }

    void Spline::SmoothKey(int idx, float smoothCoef)
    {
        InternalSmoothKeyAt(idx, smoothCoef);

        if (mBatchChange)
            mChangedKeys = true;
        else
            UpdateApproximation();
    }

    float Spline::Length() const
    {
        if (mKeys.Count() == 0)
            return 0.0f;

        return mClosed ? mKeys[0].position : mKeys.Last().position;
    }

    bool Spline::IsEmpty() const
    {
        return mKeys.Count() == 0;
    }

    RectF Spline::GetRect() const
    {
        RectF res;

        if (mKeys.Count() < 2)
            return res;

        res.left = mKeys[1].mLeftApproxValues[0].value.x; res.right = mKeys[1].mLeftApproxValues[0].value.x;
        res.top = mKeys[1].mLeftApproxValues[0].value.y; res.bottom = mKeys[1].mLeftApproxValues[0].value.y;

        for (int k = 1; k < mKeys.Count(); k++)
        {
            auto& key = mKeys[k];
            for (int i = 0; i < Key::mApproxValuesCount; i++)
            {
                res.left = Math::Min(key.mLeftApproxValues[i].value.x, res.left);
                res.right = Math::Max(key.mLeftApproxValues[i].value.x, res.right);
                res.top = Math::Max(key.mLeftApproxValues[i].value.y, res.top);
                res.bottom = Math::Min(key.mLeftApproxValues[i].value.y, res.bottom);

                res.left = Math::Min(key.mRightApproxValues[i].value.x, res.left);
                res.right = Math::Max(key.mRightApproxValues[i].value.x, res.right);
                res.top = Math::Max(key.mRightApproxValues[i].value.y, res.top);
                res.bottom = Math::Min(key.mRightApproxValues[i].value.y, res.bottom);
            }
        }

        return res;
    }

    const Spline::Key& Spline::operator[](int idx) const
    {
        if (idx >=0 && idx <= mKeys.Count() - 1)
            return mKeys[idx];

        static Key empty;
        return empty;
    }

    Spline::Key& Spline::operator[](int idx)
    {
        if (idx >=0 && idx <= mKeys.Count() - 1)
            return mKeys[idx];

        static Key empty;
        return empty;
    }

    void Spline::CheckSmoothKeys()
    {
        for (int i = 0; i < mKeys.Count(); i++)
        {
            if (mKeys[i].supportsType == Key::Type::Smooth)
                InternalSmoothKeyAt(i);
        }
    }

    void Spline::UpdateApproximation()
    {
        float lengthLeft = 0.0f;
        float lengthRight = 0.0f;

        Vec2F prevLeft;
        Vec2F prevRight;

        auto calculateApproximation = [&](int begin, int end) {
            Key& beginKey = mKeys[begin];
            Key& endKey = mKeys[end];

            Vec2F beginNormal = Key::GetRangeNormal(beginKey.value, begin == 0 ? nullptr : &mKeys[begin - 1].value, begin == mKeys.Count() - 1 ? nullptr : &mKeys[begin + 1].value);
            Vec2F beginRange = beginNormal * beginKey.valueRange;

            Vec2F endNormal = Key::GetRangeNormal(endKey.value, end == 0 ? nullptr : &mKeys[end - 1].value, end == mKeys.Count() - 1 ? nullptr : &mKeys[end + 1].value);
            Vec2F endRange = endNormal * endKey.valueRange;

            Vec2F beginLeft = beginKey.value + beginRange;
            Vec2F beginSupportLeft = beginKey.value + beginRange + beginKey.nextSupport;
            Vec2F endSupportLeft = endKey.value + endRange + endKey.prevSupport;
            Vec2F endLeft = endKey.value + endRange;

            Vec2F beginRight = beginKey.value - beginRange;
            Vec2F beginSupportRight = beginKey.value - beginRange + beginKey.nextSupport;
            Vec2F endSupportRight = endKey.value - endRange + endKey.prevSupport;
            Vec2F endRight = endKey.value - endRange;

            endKey.mApproxValuesBounds.Set(beginLeft, beginLeft);
            for (int j = 0; j < Key::mApproxValuesCount; j++)
            {
                float coef = (float)j / (float)(Key::mApproxValuesCount - 1);
                Vec2F pointLeft = Bezier(beginLeft, beginSupportLeft, endSupportLeft, endLeft, coef);
                Vec2F pointRight = Bezier(beginRight, beginSupportRight, endSupportRight, endRight, coef);

                if (begin == 0 && j == 0)
                {
                    prevLeft = pointLeft;
                    prevRight = pointRight;
                }

                lengthLeft += (pointLeft - prevLeft).Length();
                prevLeft = pointLeft;

                lengthRight += (pointRight - prevRight).Length();
                prevRight = pointRight;

                endKey.mLeftApproxValues[j].value = pointLeft;
                endKey.mLeftApproxValues[j].position = lengthLeft;

                endKey.mRightApproxValues[j].value = pointRight;
                endKey.mRightApproxValues[j].position = lengthRight;

                endKey.mApproxValuesBounds.left = Math::Min(endKey.mApproxValuesBounds.left, pointLeft.x);
                endKey.mApproxValuesBounds.right = Math::Max(endKey.mApproxValuesBounds.right, pointLeft.x);
                endKey.mApproxValuesBounds.top = Math::Max(endKey.mApproxValuesBounds.top, pointLeft.y);
                endKey.mApproxValuesBounds.bottom = Math::Min(endKey.mApproxValuesBounds.bottom, pointLeft.y);

                endKey.mApproxValuesBounds.left = Math::Min(endKey.mApproxValuesBounds.left, pointRight.x);
                endKey.mApproxValuesBounds.right = Math::Max(endKey.mApproxValuesBounds.right, pointRight.x);
                endKey.mApproxValuesBounds.top = Math::Max(endKey.mApproxValuesBounds.top, pointRight.y);
                endKey.mApproxValuesBounds.bottom = Math::Min(endKey.mApproxValuesBounds.bottom, pointRight.y);
            }

            mKeys[end].position = lengthLeft;
        };

        for (int i = 1; i < mKeys.Count(); i++)
            calculateApproximation(i - 1, i);

        if (mClosed && !mKeys.IsEmpty())
            calculateApproximation(mKeys.Count() - 1, 0);

        onKeysChanged();
    }

    Vector<Spline::Key> Spline::GetKeysNonContant()
    {
        return mKeys;
    }

    void Spline::OnDeserialized(const DataValue& node)
    {
        UpdateApproximation();
    }

    void Spline::InternalSmoothKeyAt(int idx, float smoothCoef /*= 1.0f*/)
    {
        const float baseSmoothCoef = 0.5f;

        Key& key = mKeys[idx];
        Vec2F thisKeyPoint = key.value;

        if (idx == 0)
        {
            Key& nextKey = mKeys[Math::Min(idx + 1, mKeys.Count() - 1)];

            Vec2F nextKeyPoint = nextKey.value;
            Vec2F supportVec = (nextKeyPoint - thisKeyPoint)*baseSmoothCoef*smoothCoef;

            key.nextSupport = supportVec;
        }
        else if (idx == mKeys.Count() - 1)
        {
            Key& lastKey = mKeys[Math::Max(0, idx - 1)];

            Vec2F lastKeyPoint = lastKey.value;
            Vec2F supportVec = (lastKeyPoint - thisKeyPoint)*baseSmoothCoef*smoothCoef;

            key.prevSupport = supportVec;
        }
        else
        {
            Key& lastKey = mKeys[Math::Max(0, idx - 1)];
            Key& nextKey = mKeys[Math::Min(idx + 1, mKeys.Count())];

            Vec2F lastKeyPoint = lastKey.value;
            Vec2F nextKeyPoint = nextKey.value;

            Vec2F thisToLast = lastKeyPoint - thisKeyPoint;
            Vec2F thisToNext = nextKeyPoint - thisKeyPoint;

            float supportLength = Math::Min(thisToNext.Length(), thisToLast.Length())*baseSmoothCoef*smoothCoef;
            Vec2F supportVec = Math::CalculateEllipseTangent(lastKeyPoint, thisKeyPoint, nextKeyPoint)*supportLength;

            key.prevSupport = supportVec.Inverted();
            key.nextSupport = supportVec;
        }
    }

    Spline::Key::Key()
    {}

    Spline::Key::Key(const Vec2F& value, float valueRange, const Vec2F& prevSupport, const Vec2F& nextSupport) :
        value(value), valueRange(valueRange), prevSupport(prevSupport), nextSupport(nextSupport), supportsType(Type::Broken)
    { }

    Spline::Key::Key(const Key& other) :
        uid(other.uid), value(other.value), valueRange(other.valueRange), position(other.position), prevSupport(other.prevSupport),
        nextSupport(other.nextSupport), supportsType(other.supportsType), mApproxValuesBounds(other.mApproxValuesBounds)
    {
        memcpy(mLeftApproxValues, other.mLeftApproxValues, mApproxValuesCount * sizeof(ApproximationVec2F));
        memcpy(mRightApproxValues, other.mRightApproxValues, mApproxValuesCount * sizeof(ApproximationVec2F));
    }

    Spline::Key::operator Vec2F() const
    {
        return value;
    }

    const ApproximationVec2F* Spline::Key::GetApproximatedPointsLeft() const
    {
        return mLeftApproxValues;
    }

    const ApproximationVec2F* Spline::Key::GetApproximatedPointsRight() const
    {
        return mRightApproxValues;
    }

    int Spline::Key::GetApproximatedPointsCount() const
    {
        return mApproxValuesCount;
    }

    const RectF& Spline::Key::GetGetApproximatedPointsBounds() const
    {
        return mApproxValuesBounds;
    }

    Vec2F Spline::Key::GetRangeNormal(const Vec2F& position, Vec2F* prev, Vec2F* next)
    {
        if (!prev && !next)
            return Vec2F(0.0f, 1.0f);

        if (!prev)
            return (*next - position).Normalized().Perpendicular();

        if (!next)
            return (position - *prev).Normalized().Perpendicular();

        Vec2F nextDirNorm = (*next - position).Normalized().Perpendicular();
        Vec2F prevDirNorm = (position - *prev).Normalized().Perpendicular();

        return ((nextDirNorm + prevDirNorm)*0.5f).Normalized();
    }

    Spline::Key& Spline::Key::operator=(const Vec2F& value)
    {
        this->value = value;
        return *this;
    }

    Spline::Key& Spline::Key::operator=(const Key& other)
    {
        uid = other.uid;
        value = other.value;
        valueRange = other.valueRange;
        position = other.position;
        prevSupport = other.prevSupport;
        nextSupport = other.nextSupport;
        supportsType = other.supportsType;
        mApproxValuesBounds = other.mApproxValuesBounds;

        memcpy(mLeftApproxValues, other.mLeftApproxValues, mApproxValuesCount * sizeof(Vec2F));
        memcpy(mRightApproxValues, other.mRightApproxValues, mApproxValuesCount * sizeof(Vec2F));

        return *this;
    }

    bool Spline::Key::operator!=(const Key& other) const
    {
        return !(*this == other);
    }

    bool Spline::Key::operator==(const Key& other) const
    {
        return Math::Equals(position, other.position) && Math::Equals(value, other.value) &&
            Math::Equals(prevSupport, other.prevSupport) &&
            Math::Equals(nextSupport, other.nextSupport) &&
            supportsType == other.supportsType;
    }

}
// --- META ---

ENUM_META(o2::Spline::Key::Type, o2__Spline__Key__Type)
{
    ENUM_ENTRY(Broken);
    ENUM_ENTRY(Free);
    ENUM_ENTRY(Smooth);
}
END_ENUM_META;

DECLARE_CLASS(o2::Spline, o2__Spline);

DECLARE_CLASS(o2::Spline::Key, o2__Spline__Key);
// --- END META ---
