#include "o2/stdafx.h"
#include "ColorGradient.h"

#include "o2/Utils/Math/Interpolation.h"
#include "o2/Utils/Tools/KeySearch.h"

namespace o2
{
    ColorGradient::ColorGradient():
        ColorGradient(Color4::White(), Color4::White())
    { }

    ColorGradient::ColorGradient(const Color4& beginColor, const Color4& endColor)
    {
        mKeys.Add(Key(beginColor, 0.0f));
        mKeys.Add(Key(endColor, 1.0f));
    }

    ColorGradient::ColorGradient(const Vector<Pair<float, Color4>>& values)
    {
        AppendKeys(values);
    }

    ColorGradient::ColorGradient(const ColorGradient& other) :
        mKeys(other.mKeys)
    { }

    bool ColorGradient::operator!=(const ColorGradient& other) const
    {
        return !(*this == other);
    }

    bool ColorGradient::operator==(const ColorGradient& other) const
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

    ColorGradient& ColorGradient::operator=(const ColorGradient& other)
    {
        mKeys = other.mKeys;
        onKeysChanged();

        return *this;
    }

    ColorGradient& ColorGradient::operator+=(const ColorGradient& other)
    {
        AppendCurve(other);
        return *this;
    }

    Color4 ColorGradient::Evaluate(float position) const
    {
        int cacheKey = 0;
        return Evaluate(position, true, cacheKey);
    }

    Color4 ColorGradient::Evaluate(float position, bool direction, int& cacheKey) const
    {
        int count = mKeys.Count();

        if (count == 1)
            return mKeys[0].color;
        else if (count == 0)
            return Color4();

        int prevCacheKey = cacheKey;
        int keyLeftIdx = -1, keyRightIdx = -1;
        SearchKey(mKeys, count, position, keyLeftIdx, keyRightIdx, direction, cacheKey);

        if (keyLeftIdx < 0)
            return Color4();

        const Key& leftKey = mKeys[keyLeftIdx];
        const Key& rightKey = mKeys[keyRightIdx];

        float dist = rightKey.position - leftKey.position;
        float coef = (position - leftKey.position) / dist;

        return Math::Lerp(leftKey.color, rightKey.color, coef);
    }

    void ColorGradient::MoveKeys(float offset)
    {
        for (auto& key : mKeys)
            key.position += offset;

        onKeysChanged();
    }

    void ColorGradient::MoveKeysFrom(float begin, float offset)
    {
        for (auto& key : mKeys)
        {
            if (key.position >= begin)
                key.position += offset;
        }

        onKeysChanged();
    }

    void ColorGradient::AppendCurve(const ColorGradient& gradient)
    {
        AppendKeys(gradient.mKeys);
    }

    void ColorGradient::PrependCurve(const ColorGradient& gradient)
    {
        PrependKeys(gradient.mKeys);
    }

    void ColorGradient::InsertCurve(const ColorGradient& gradient, float position)
    {
        InsertKeys(gradient.mKeys, position);
    }

    void ColorGradient::AppendKeys(const Vector<Pair<float, Color4>>& values)
    {
        AppendKeys(values.Convert<Key>([=](const Pair<float, Color4>& v) { return Key(v.second, v.first); }));
    }

    void ColorGradient::AppendKeys(const Vector<Key>& keys)
    {
        if (keys.IsEmpty())
            return;

        float offset = (mKeys.IsEmpty() ? 0.0f : mKeys.Last().position) - keys[0].position;

        for (auto key : keys)
        {
            key.position += offset;
            mKeys.Add(key);
        }

        onKeysChanged();
    }

    void ColorGradient::PrependKeys(const Vector<Pair<float, Color4>>& values)
    {
        PrependKeys(values.Convert<Key>([=](const Pair<float, Color4>& v) { return Key(v.second, v.first); }));
    }

    void ColorGradient::PrependKeys(const Vector<Key>& keys)
    {
        if (keys.IsEmpty())
            return;

        float currentBegin = mKeys.IsEmpty() ? 0.0f : mKeys[0].position;
        float offset = keys.Last().position - Math::Min(currentBegin, 0.0f);

        for (int i = 0; i < mKeys.Count(); i++)
            mKeys[i].position += offset;

        mKeys.Insert(keys, 0);

        onKeysChanged();
    }

    void ColorGradient::InsertKeys(const Vector<Pair<float, Color4>>& values, float position)
    {
        InsertKeys(values.Convert<Key>([=](const Pair<float, Color4>& v) { return Key(v.second, v.first); }), position);
    }

    void ColorGradient::InsertKeys(const Vector<Key>& keys, float position)
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

        onKeysChanged();
    }

    int ColorGradient::InsertKey(const Key& key)
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

        onKeysChanged();

        return pos;
    }

    int ColorGradient::InsertKey(float position, const Color4& value)
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

        onKeysChanged();

        return pos;
    }

    int ColorGradient::AppendKey(float offset, const Color4& value)
    {
        mKeys.Add(Key(value, (mKeys.IsEmpty() ? 0.0f : mKeys.Last().position) + offset));
        onKeysChanged();
        return mKeys.Count() - 1;
    }

    int ColorGradient::PrependKey(float offset, const Color4& value)
    {
        float begin = mKeys.IsEmpty() ? 0.0f : mKeys[0].position;

        for (auto& key : mKeys)
            key.position += offset;

        mKeys.Add(Key(value, begin));

        onKeysChanged();

        return 0;
    }

    ColorGradient::Key ColorGradient::GetKey(float position) const
    {
        for (auto& key : mKeys)
        {
            if (Math::Equals(key.position, position))
                return key;
        }

        return Key();
    }

    ColorGradient::Key ColorGradient::GetKeyAt(int idx) const
    {
        if (idx < 0 || idx > mKeys.Count() - 1)
            return Key();

        return mKeys[idx];
    }

    ColorGradient::Key ColorGradient::FindKey(UInt64 uid) const
    {
        for (auto& key : mKeys)
        {
            if (key.uid == uid)
                return key;
        }

        return Key();
    }

    int ColorGradient::FindKeyIdx(UInt64 uid) const
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

    bool ColorGradient::RemoveKey(float position)
    {
        for (int i = 0; i < mKeys.Count(); i++)
        {
            if (Math::Equals(mKeys[i].position, position))
            {
                mKeys.RemoveAt(i);
                onKeysChanged();
                return true;
            }
        }

        return false;
    }

    bool ColorGradient::RemoveKeyAt(int idx)
    {
        if (idx < 0 || idx >= mKeys.Count())
            return false;

        mKeys.RemoveAt(idx);
        onKeysChanged();

        return true;
    }

    void ColorGradient::RemoveAllKeys()
    {
        mKeys.Clear();
        onKeysChanged();
    }

    bool ColorGradient::ContainsKey(float position) const
    {
        for (auto& key : mKeys)
        {
            if (Math::Equals(key.position, position))
                return true;
        }

        return false;
    }

    const Vector<ColorGradient::Key>& ColorGradient::GetKeys() const
    {
        return mKeys;
    }

    void ColorGradient::SetKeys(const Vector<Key>& keys)
    {
        mKeys = keys;
        SortKeys();
        onKeysChanged();
    }

    void ColorGradient::SetKey(const Key& key, int idx)
    {
        if (idx < 0 || idx > mKeys.Count() - 1)
            return;

        mKeys[idx] = key;
        SortKeys();
        onKeysChanged();
    }

    float ColorGradient::Length() const
    {
        if (mKeys.Count() == 0)
            return 0.0f;

        return mKeys.Last().position;
    }

    bool ColorGradient::IsEmpty() const
    {
        return mKeys.Count() == 0;
    }

    ColorGradient::Key ColorGradient::operator[](float position) const
    {
        for (auto& key : mKeys)
        {
            if (Math::Equals(key.position, position))
                return key;
        }

        return Key();
    }

    void ColorGradient::SortKeys()
    {
        mKeys.Sort([](const Key& a, const Key& b) { return a.position < b.position; });
    }

    ColorGradient::Key::Key() :
        uid(Math::Random()), color(), position(0)
    { }

    ColorGradient::Key::Key(const Color4& value, float position) :
        uid(Math::Random()), color(value), position(position)
    { }

    ColorGradient::Key::Key(const Key& other) :
        uid(other.uid), color(other.color), position(other.position)
    { }

    ColorGradient::Key::operator Color4() const
    {
        return color;
    }

    ColorGradient::Key& ColorGradient::Key::operator=(const Color4& value)
    {
        this->color = value;
        return *this;
    }

    ColorGradient::Key& ColorGradient::Key::operator=(const Key& other)
    {
        uid = other.uid;
        color = other.color;
        position = other.position;

        return *this;
    }

    bool ColorGradient::Key::operator!=(const Key& other) const
    {
        return !(*this == other);
    }

    bool ColorGradient::Key::operator==(const Key& other) const
    {
        return Math::Equals(position, other.position) && Math::Equals(color, other.color);
    }

}
// --- META ---

DECLARE_CLASS(o2::ColorGradient, o2__ColorGradient);

DECLARE_CLASS(o2::ColorGradient::Key, o2__ColorGradient__Key);
// --- END META ---
