#include "o2/stdafx.h"
#include "Curve.h"

#include "o2/Utils/Math/Interpolation.h"
#include "o2/Utils/Tools/KeySearch.h"

namespace o2
{
    Curve::Curve()
    { }

    Curve::Curve(float beginCoef, float beginCoefPosition, float endCoef, float endCoefPosition)
    {
        mKeys.Add(Key(0.0f, 0.0f, 0.0f, 0.0f, beginCoef, beginCoefPosition));
        mKeys.Add(Key(1.0f, 1.0f, endCoef - 1.0f, -endCoefPosition, 0.0f, 0.0f));
        UpdateApproximation();
    }

    Curve::Curve(const Vector<Vec2F>& values, bool smooth /*= true*/)
    {
        AppendKeys(values, smooth);
    }

    Curve::Curve(const Curve& other) :
        mKeys(other.mKeys), keys(this), length(this)
    { }

    bool Curve::operator!=(const Curve& other) const
    {
        return !(*this == other);
    }

    bool Curve::operator==(const Curve& other) const
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

    Curve& Curve::operator=(const Curve& other)
    {
        mKeys = other.mKeys;

        UpdateApproximation();

        onKeysChanged();

        return *this;
    }

    Curve& Curve::operator+=(const Curve& other)
    {
        AppendCurve(other);
        return *this;
    }

    float Curve::Evaluate(float position) const
    {
        int cacheKey = 0, cacheKeyApprox = 0;
        return Evaluate(position, true, cacheKey, cacheKeyApprox);
    }

    float Curve::Evaluate(float position, bool direction, int& cacheKey, int& cacheKeyApprox) const
    {
        int count = mKeys.Count();

        if (count == 1)
            return mKeys[0].value;
        else if (count == 0)
            return 0.0f;

        int prevCacheKey = cacheKey;
        int keyLeftIdx = -1, keyRightIdx = -1;
        SearchKey(mKeys, count, position, keyLeftIdx, keyRightIdx, direction, cacheKey);

        if (keyLeftIdx < 0)
            return 0.0f;

        const Key& rightKey = mKeys[keyRightIdx];

        int segLeftIdx = 0;
        int segRightIdx = 1;

        if (keyLeftIdx != prevCacheKey)
            cacheKeyApprox = 0;

        SearchKey(rightKey.mApproxValues, Key::mApproxValuesCount, position, segLeftIdx, segRightIdx, direction, cacheKeyApprox);

        const ApproximationValue& segLeft = rightKey.mApproxValues[segLeftIdx];
        const ApproximationValue& segRight = rightKey.mApproxValues[segRightIdx];

        float dist = segRight.position - segLeft.position;
        float coef = (position - segLeft.position) / dist;

        return Math::Lerp(segLeft.value, segRight.value, coef);
    }

    void Curve::BeginKeysBatchChange()
    {
        mBatchChange = true;
    }

    void Curve::CompleteKeysBatchingChange()
    {
        UpdateApproximation();
        mBatchChange = false;
        mChangedKeys = false;
    }

    void Curve::MoveKeys(float offset)
    {
        for (auto& key : mKeys)
            key.position += offset;

        if (mBatchChange)
            mChangedKeys = true;
        else
            UpdateApproximation();
    }

    void Curve::MoveKeysFrom(float begin, float offset)
    {
        for (auto& key : mKeys)
        {
            if (key.position >= begin)
                key.position += offset;
        }

        if (mBatchChange)
            mChangedKeys = true;
        else
            UpdateApproximation();
    }

    void Curve::AppendCurve(const Curve& curve)
    {
        AppendKeys(curve.mKeys);
    }

    void Curve::PrependCurve(const Curve& curve)
    {
        PrependKeys(curve.mKeys);
    }

    void Curve::InsertCurve(const Curve& curve, float position)
    {
        InsertKeys(curve.mKeys, position);
    }

    void Curve::AppendKeys(const Vector<Vec2F>& values, bool smooth /*= true*/)
    {
        AppendKeys(values.Convert<Key>([=](const Vec2F& v) {
            Key res(v.x, v.y, 0, 0, 0, 0);
            res.supportsType = smooth ? Key::Type::Smooth : Key::Type::Broken;
            return res;
        }));
    }

    void Curve::AppendKeys(const Vector<Key>& keys)
    {
        if (keys.IsEmpty())
            return;

        float offset = (mKeys.IsEmpty() ? 0.0f : mKeys.Last().position) - keys[0].position;

        for (auto key : keys)
        {
            key.position += offset;
            mKeys.Add(key);
        }

        CheckSmoothKeys();

        if (mBatchChange)
            mChangedKeys = true;
        else
            UpdateApproximation();
    }

    void Curve::PrependKeys(const Vector<Vec2F>& values, bool smooth /*= true*/)
    {
        PrependKeys(values.Convert<Key>([=](const Vec2F& v) {
            Key res(v.x, v.y, 0, 0, 0, 0);
            res.supportsType = smooth ? Key::Type::Smooth : Key::Type::Broken;
            return res;
        }));
    }

    void Curve::PrependKeys(const Vector<Key>& keys)
    {
        if (keys.IsEmpty())
            return;

        float currentBegin = mKeys.IsEmpty() ? 0.0f : mKeys[0].position;
        float offset = keys.Last().position - Math::Min(currentBegin, 0.0f);

        for (int i = 0; i < mKeys.Count(); i++)
            mKeys[i].position += offset;

        mKeys.Insert(keys, 0);

        CheckSmoothKeys();

        if (mBatchChange)
            mChangedKeys = true;
        else
            UpdateApproximation();
    }

    void Curve::InsertKeys(const Vector<Vec2F>& values, float position, bool smooth /*= true*/)
    {
        InsertKeys(values.Convert<Key>([=](const Vec2F& v) {
            Key res(v.x, v.y, 0, 0, 0, 0);
            res.supportsType = smooth ? Key::Type::Smooth : Key::Type::Broken;
            return res;
        }), position);
    }

    void Curve::InsertKeys(const Vector<Key>& keys, float position)
    {
        if (keys.IsEmpty())
            return;

        int pos = mKeys.Count();
        for (int i = 0; i < mKeys.Count(); i++)
        {
            if (mKeys[i].position > position)
            {
                pos = i;
                break;
            }
        }

        mKeys.Insert(keys, pos);

        float insertOffset = position - keys[0].position;
        for (int i = 0; i < keys.Count(); i++)
            mKeys[i + pos].position += insertOffset;

        float offset = keys.Last().position - keys[0].position;
        for (int i = pos + keys.Count(); i < mKeys.Count(); i++)
            mKeys[i].position += offset;

        CheckSmoothKeys();

        if (mBatchChange)
            mChangedKeys = true;
        else
            UpdateApproximation();
    }

    int Curve::InsertKey(const Key& key)
    {
        int pos = mKeys.Count();
        for (int i = 0; i < mKeys.Count(); i++)
        {
            if (mKeys[i].position > key.position)
            {
                pos = i;
                break;
            }
        }

        pos = Math::Clamp(pos, 0, mKeys.Count());
        mKeys.Insert(key, pos);

        CheckSmoothKeys();

        if (mBatchChange)
            mChangedKeys = true;
        else
            UpdateApproximation();

        return pos;
    }

    int Curve::InsertKey(float position, float value, float leftCoef, float leftCoefPosition,
                         float rightCoef, float rightCoefPosition)
    {
        return InsertKey(Key(position, value, leftCoef, leftCoefPosition, rightCoef, rightCoefPosition));
    }

    int Curve::InsertKey(float position, float value, float smooth /*= 1.0f*/)
    {
        int pos = mKeys.Count();
        for (int i = 0; i < mKeys.Count(); i++)
        {
            if (mKeys[i].position > position)
            {
                pos = i;
                break;
            }
        }

        pos = Math::Clamp(pos, 0, mKeys.Count());
        mKeys.Insert(Key(value, position), pos);
        SmoothKey(position, smooth);

        if (mBatchChange)
            mChangedKeys = true;
        else
            UpdateApproximation();

        return pos;
    }

    int Curve::InsertFlatKey(float position, float value)
    {
        int pos = mKeys.Count();
        for (int i = 0; i < mKeys.Count(); i++)
        {
            if (mKeys[i].position > position)
            {
                pos = i;
                break;
            }
        }

        pos = Math::Clamp(pos, 0, mKeys.Count());
        float leftSupport = 0, rightSupport = 0;
        const float supportCoef = 0.4f;

        if (pos > 0)
            leftSupport = (position - mKeys[pos - 1].position)*supportCoef;

        if (pos < mKeys.Count() - 1)
            rightSupport = (mKeys[pos + 1] - position)*supportCoef;

        mKeys.Insert(Key(position, value, value, leftSupport, value, rightSupport), pos);

        CheckSmoothKeys();

        if (mBatchChange)
            mChangedKeys = true;
        else
            UpdateApproximation();

        return pos;
    }

    int Curve::AppendKey(float offset, float value, float leftCoef, float leftCoefPosition,
                         float rightCoef, float rightCoefPosition)
    {
        Key newKey((mKeys.IsEmpty() ? 0.0f : mKeys.Last().position) + offset, value, leftCoef, leftCoefPosition,
                   rightCoef, rightCoefPosition);

        newKey.supportsType = Key::Type::Broken;

        mKeys.Add(newKey);
        CheckSmoothKeys();

        if (mBatchChange)
            mChangedKeys = true;
        else
            UpdateApproximation();

        return mKeys.Count() - 1;
    }

    int Curve::AppendKey(float offset, float value, float smoothCoef /*= 1.0f*/)
    {
        Key newKey((mKeys.IsEmpty() ? 0.0f : mKeys.Last().position) + offset, value, value, 0, value, 0);
        newKey.supportsType = Key::Type::Smooth;

        mKeys.Add(newKey);
        CheckSmoothKeys();

        if (mBatchChange)
            mChangedKeys = true;
        else
            UpdateApproximation();

        return mKeys.Count() - 1;
    }

    int Curve::AppendKey(float offset, float value)
    {
        Key newKey((mKeys.IsEmpty() ? 0.0f : mKeys.Last().position) + offset, value, value, 0, value, 0);
        newKey.supportsType = Key::Type::Broken;

        mKeys.Add(newKey);
        CheckSmoothKeys();

        if (mBatchChange)
            mChangedKeys = true;
        else
            UpdateApproximation();

        return mKeys.Count() - 1;
    }

    int Curve::PrependKey(float offset, float value, float leftCoef, float leftCoefPosition,
                          float rightCoef, float rightCoefPosition)
    {
        float begin = mKeys.IsEmpty() ? 0.0f : mKeys[0].position;

        for (auto& key : mKeys)
            key.position += offset;

        Key newKey(begin, value, leftCoef, leftCoefPosition,
                   rightCoef, rightCoefPosition);

        newKey.supportsType = Key::Type::Broken;

        mKeys.Add(newKey);
        CheckSmoothKeys();

        if (mBatchChange)
            mChangedKeys = true;
        else
            UpdateApproximation();

        return 0;
    }

    int Curve::PrependKey(float offset, float value, float smoothCoef /*= 1.0f*/)
    {
        float begin = mKeys.IsEmpty() ? 0.0f : mKeys[0].position;

        for (auto& key : mKeys)
            key.position += offset;

        Key newKey(begin, value, value, 0, value, 0);
        newKey.supportsType = Key::Type::Smooth;

        mKeys.Add(newKey);
        CheckSmoothKeys();

        if (mBatchChange)
            mChangedKeys = true;
        else
            UpdateApproximation();

        return 0;
    }

    int Curve::PrependKey(float offset, float value)
    {
        float begin = mKeys.IsEmpty() ? 0.0f : mKeys[0].position;

        for (auto& key : mKeys)
            key.position += offset;

        Key newKey(begin, value, value, 0, value, 0);
        newKey.supportsType = Key::Type::Broken;

        mKeys.Add(newKey);
        CheckSmoothKeys();

        if (mBatchChange)
            mChangedKeys = true;
        else
            UpdateApproximation();

        return 0;
    }

    Curve::Key Curve::GetKey(float position) const
    {
        for (auto& key : mKeys)
        {
            if (Math::Equals(key.position, position))
                return key;
        }

        return Key();
    }

    Curve::Key Curve::GetKeyAt(int idx) const
    {
        if (idx < 0 || idx > mKeys.Count() - 1)
            return Key();

        return mKeys[idx];
    }

    Curve::Key Curve::FindKey(UInt64 uid) const
    {
        for (auto& key : mKeys)
        {
            if (key.uid == uid)
                return key;
        }

        return Key();
    }

    int Curve::FindKeyIdx(UInt64 uid) const
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

    bool Curve::RemoveKey(float position)
    {
        for (int i = 0; i < mKeys.Count(); i++)
        {
            if (Math::Equals(mKeys[i].position, position))
            {
                mKeys.RemoveAt(i);

                if (mBatchChange)
                    mChangedKeys = true;
                else
                    UpdateApproximation();

                return true;
            }
        }

        return false;
    }

    bool Curve::RemoveKeyAt(int idx)
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

    void Curve::RemoveAllKeys()
    {
        mKeys.Clear();
        onKeysChanged();
    }

    bool Curve::ContainsKey(float position) const
    {
        for (auto& key : mKeys)
        {
            if (Math::Equals(key.position, position))
                return true;
        }

        return false;
    }

    const Vector<Curve::Key>& Curve::GetKeys() const
    {
        return mKeys;
    }

    void Curve::SetKeys(const Vector<Key>& keys)
    {
        mKeys = keys;
        CheckSmoothKeys();

        if (mBatchChange)
            mChangedKeys = true;
        else
            UpdateApproximation();
    }

    void Curve::SetKey(const Key& key, int idx)
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

    void Curve::SmoothKey(float position, float smoothCoef)
    {
        int pos = 0;

        for (int i = 0; i < mKeys.Count(); i++)
        {
            if (Math::Equals(mKeys[i].position, position))
            {
                pos = i;
                break;
            }
        }

        SmoothKeyAt(pos, smoothCoef);
    }

    void Curve::SmoothKeyAt(int idx, float smoothCoef)
    {
        InternalSmoothKeyAt(idx, smoothCoef);

        if (mBatchChange)
            mChangedKeys = true;
        else
            UpdateApproximation();
    }

    float Curve::Length() const
    {
        if (mKeys.Count() == 0)
            return 0.0f;

        return mKeys.Last().position;
    }

    bool Curve::IsEmpty() const
    {
        return mKeys.Count() == 0;
    }

    RectF Curve::GetRect() const
    {
        RectF res;

        if (mKeys.Count() < 2)
            return res;

        res.left = mKeys[1].mApproxValues[0].position; res.right = mKeys[1].mApproxValues[0].position;
        res.top = mKeys[1].mApproxValues[0].value; res.bottom = mKeys[1].mApproxValues[0].value;

        for (int k = 1; k < mKeys.Count(); k++)
        {
            auto& key = mKeys[k];
            for (int i = 0; i < Key::mApproxValuesCount; i++)
            {
                res.left = Math::Min(key.mApproxValues[i].position, res.left);
                res.right = Math::Max(key.mApproxValues[i].position, res.right);
                res.top = Math::Max(key.mApproxValues[i].value, res.top);
                res.bottom = Math::Min(key.mApproxValues[i].value, res.bottom);
            }
        }

        return res;
    }

    Curve::Key Curve::operator[](float position) const
    {
        for (auto& key : mKeys)
        {
            if (Math::Equals(key.position, position))
                return key;
        }

        return Key();
    }

    Curve Curve::Parametric(float begin, float end, float duration, 
                            float beginCoef, float beginCoefPosition, float endCoef, float endCoefPosition)
    {
        Curve res;
        res.InsertKey(0.0f, begin, 0.0f, 0.0f, Math::Lerp(begin, end, beginCoef) - begin, beginCoefPosition*duration);
        res.InsertKey(duration, end, Math::Lerp(begin, end, endCoef) - end, -endCoefPosition*duration, 0.0f, 0.0f);
        return res;
    }

    Curve Curve::EaseIn(float begin /*= 0.0f*/, float end /*= 1.0f*/, float duration /*= 1.0f*/, float strongness /*= 0.4f*/)
    {
        return Parametric(begin, end, duration, 0.0f, strongness, 1.0f, 1.0f);
    }

    Curve Curve::EaseOut(float begin /*= 0.0f*/, float end /*= 1.0f*/, float duration /*= 1.0f*/, float strongness /*= 0.4f*/)
    {
        return Parametric(begin, end, duration, 0.0f, 0.0f, 1.0f, strongness);
    }

    Curve Curve::EaseInOut(float begin /*= 0.0f*/, float end /*= 1.0f*/, float duration /*= 1.0f*/, float strongness /*= 0.4f*/)
    {
        return Parametric(begin, end, duration, 0.0f, strongness, 1.0f, 1.0f);
    }

    Curve Curve::Linear(float begin /*= 0.0f*/, float end /*= 1.0f*/, float duration /*= 1.0f*/)
    {
        return Parametric(begin, end, duration, 0.0f, 0.0f, 1.0f, 1.0f);
    }

    void Curve::CheckSmoothKeys()
    {
        for (int i = 0; i < mKeys.Count(); i++)
        {
            if (mKeys[i].supportsType == Key::Type::Smooth)
                InternalSmoothKeyAt(i);
        }
    }

    void Curve::UpdateApproximation()
    {
        for (int i = 1; i < mKeys.Count(); i++)
        {
            Key& beginKey = mKeys[i - 1];
            Key& endKey = mKeys[i];

            Vec2F rightSupport(beginKey.rightSupportPosition, beginKey.rightSupportValue);
            Vec2F leftSupport(endKey.leftSupportPosition, endKey.leftSupportValue);

            if (rightSupport.x < 0.0f)
                rightSupport.x = 0;

            if (rightSupport.x > endKey.position - beginKey.position && rightSupport.x != 0.0f)
                rightSupport *= (endKey.position - beginKey.position) / rightSupport.x;

            if (leftSupport.x > 0.0f)
                leftSupport.x = 0;

            if (leftSupport.x < beginKey.position - endKey.position && leftSupport.x != 0.0f)
                leftSupport *= (beginKey.position - endKey.position) / leftSupport.x;

            Vec2F a(beginKey.position, beginKey.value);
            Vec2F d(endKey.position, endKey.value);
            Vec2F b = a + rightSupport;
            Vec2F c = d + leftSupport;

            endKey.mApproxValuesBounds.Set(a, a);
            for (int j = 0; j < Key::mApproxValuesCount; j++)
            {
                float coef = (float)j / (float)(Key::mApproxValuesCount - 1);
                Vec2F p = Bezier(a, b, c, d, coef);
                endKey.mApproxValues[j] = p;
                endKey.mApproxValuesBounds.left = Math::Min(endKey.mApproxValuesBounds.left, p.x);
                endKey.mApproxValuesBounds.right = Math::Max(endKey.mApproxValuesBounds.right, p.x);
                endKey.mApproxValuesBounds.top = Math::Max(endKey.mApproxValuesBounds.top, p.y);
                endKey.mApproxValuesBounds.bottom = Math::Min(endKey.mApproxValuesBounds.bottom, p.y);
            }
        }

        onKeysChanged();
    }

    Vector<Curve::Key> Curve::GetKeysNonContant()
    {
        return mKeys;
    }

    void Curve::OnDeserialized(const DataValue& node)
    {
        UpdateApproximation();
    }

    void Curve::InternalSmoothKeyAt(int idx, float smoothCoef /*= 1.0f*/)
    {
        const float baseSmoothCoef = 0.5f;

        Key& key = mKeys[idx];
        Vec2F thisKeyPoint(key.position, key.value);

        if (idx == 0)
        {
            Key& nextKey = mKeys[Math::Min(idx + 1, mKeys.Count() - 1)];

            Vec2F nextKeyPoint(nextKey.position, nextKey.value);
            Vec2F supportVec = (nextKeyPoint - thisKeyPoint)*baseSmoothCoef*smoothCoef;

            key.rightSupportPosition = supportVec.x;
            key.rightSupportValue = supportVec.y;
        }
        else if (idx == mKeys.Count() - 1)
        {
            Key& lastKey = mKeys[Math::Max(0, idx - 1)];

            Vec2F lastKeyPoint(lastKey.position, lastKey.value);
            Vec2F supportVec = (lastKeyPoint - thisKeyPoint)*baseSmoothCoef*smoothCoef;

            key.leftSupportPosition = supportVec.x;
            key.leftSupportValue = supportVec.y;
        }
        else
        {
            Key& lastKey = mKeys[Math::Max(0, idx - 1)];
            Key& nextKey = mKeys[Math::Min(idx + 1, mKeys.Count())];

            Vec2F lastKeyPoint(lastKey.position, lastKey.value);
            Vec2F thisKeyPoint(key.position, key.value);
            Vec2F nextKeyPoint(nextKey.position, nextKey.value);

            Vec2F thisToLast = lastKeyPoint - thisKeyPoint;
            Vec2F thisToNext = nextKeyPoint - thisKeyPoint;

            float supportLength = Math::Min(thisToNext.Length(), thisToLast.Length())*baseSmoothCoef*smoothCoef;
            Vec2F supportVec = Math::CalculateEllipseTangent(lastKeyPoint, thisKeyPoint, nextKeyPoint)*supportLength;

            key.leftSupportPosition = -supportVec.x; key.leftSupportValue = -supportVec.y;
            key.rightSupportPosition = supportVec.x; key.rightSupportValue = supportVec.y;
        }
    }

    void Curve::InternalSmoothKey(float position, float smoothCoef /*= 1.0f*/)
    {
        int pos = 0;

        for (int i = 0; i < mKeys.Count(); i++)
        {
            if (Math::Equals(mKeys[i].position, position))
            {
                pos = i;
                break;
            }
        }

        InternalSmoothKeyAt(pos, smoothCoef);
    }

    Curve::Key::Key() :
        uid(Math::Random()), value(0), position(0), leftSupportValue(0), leftSupportPosition(0), rightSupportValue(0), rightSupportPosition(0),
        supportsType(Type::Smooth)
    { }

    Curve::Key::Key(float position, float value, float leftSupportValue, float leftSupportPosition,
                    float rightSupportValue, float rightSupportPosition) :
        uid(Math::Random()), value(value), position(position), leftSupportValue(leftSupportValue), leftSupportPosition(leftSupportPosition),
        rightSupportValue(rightSupportValue), rightSupportPosition(rightSupportPosition), supportsType(Type::Broken)
    { }

    Curve::Key::Key(float value, float position) :
        uid(Math::Random()), value(value), position(position), leftSupportValue(0), leftSupportPosition(0), rightSupportValue(0),
        rightSupportPosition(0), supportsType(Type::Smooth)
    { }

    Curve::Key::Key(const Key& other) :
        uid(other.uid), value(other.value), position(other.position), leftSupportValue(other.leftSupportValue),
        leftSupportPosition(other.leftSupportPosition), rightSupportValue(other.rightSupportValue),
        rightSupportPosition(other.rightSupportPosition), supportsType(other.supportsType), mApproxValuesBounds(other.mApproxValuesBounds)
    {
        memcpy(mApproxValues, other.mApproxValues, mApproxValuesCount * sizeof(ApproximationValue));
    }

    Curve::Key::operator float() const
    {
        return value;
    }

    const ApproximationValue* Curve::Key::GetApproximatedPoints() const
    {
        return mApproxValues;
    }

    int Curve::Key::GetApproximatedPointsCount() const
    {
        return mApproxValuesCount;
    }

    const RectF& Curve::Key::GetGetApproximatedPointsBounds() const
    {
        return mApproxValuesBounds;
    }

    Curve::Key& Curve::Key::operator=(float value)
    {
        this->value = value;
        return *this;
    }

    Curve::Key& Curve::Key::operator=(const Key& other)
    {
        uid = other.uid;
        value = other.value;
        position = other.position;
        leftSupportValue = other.leftSupportValue;
        leftSupportPosition = other.leftSupportPosition;
        rightSupportValue = other.rightSupportValue;
        rightSupportPosition = other.rightSupportPosition;
        supportsType = other.supportsType;
        mApproxValuesBounds = other.mApproxValuesBounds;
        memcpy(mApproxValues, other.mApproxValues, mApproxValuesCount * sizeof(Vec2F));

        return *this;
    }

    bool Curve::Key::operator!=(const Key& other) const
    {
        return !(*this == other);
    }

    bool Curve::Key::operator==(const Key& other) const
    {
        return Math::Equals(position, other.position) && Math::Equals(value, other.value) &&
            Math::Equals(leftSupportPosition, other.leftSupportPosition) &&
            Math::Equals(leftSupportValue, other.leftSupportValue) &&
            Math::Equals(rightSupportPosition, other.rightSupportPosition) &&
            Math::Equals(rightSupportValue, other.rightSupportValue) &&
            supportsType == other.supportsType;
    }

}
// --- META ---

ENUM_META(o2::Curve::Key::Type)
{
    ENUM_ENTRY(Broken);
    ENUM_ENTRY(Discrete);
    ENUM_ENTRY(Flat);
    ENUM_ENTRY(Free);
    ENUM_ENTRY(Linear);
    ENUM_ENTRY(Smooth);
}
END_ENUM_META;

DECLARE_CLASS(o2::Curve);

DECLARE_CLASS(o2::Curve::Key);
// --- END META ---
