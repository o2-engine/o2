#pragma once

#include "o2/Utils/Editor/Attributes/ScriptableAttribute.h"
#include "o2/Utils/Math/ApproximationValue.h"
#include "o2/Utils/Math/Math.h"
#include "o2/Utils/Math/Vector2.h"
#include "o2/Utils/Property.h"
#include "o2/Utils/Serialization/Serializable.h"
#include "o2/Utils/Types/Containers/Vector.h"

namespace o2
{
    // --------------
    // Color gradient
    // --------------
    class ColorGradient: public ISerializable, public RefCounterable
    {
    public:
        class Key;

    public:
        PROPERTIES(ColorGradient);
        PROPERTY(Vector<Key>, keys, SetKeys, GetKeys); // Keys property
        GETTER(float, length, Length);                 // Max position getter

    public:
        Function<void()> onKeysChanged; // Keys changed event

    public:
        // Default constructor @SCRIPTABLE
        ColorGradient();

        // Constructor by two colors
        ColorGradient(const Color4& beginColor, const Color4& endColor);

        // Constructor from colors list
        explicit ColorGradient(const Vector<Pair<float, Color4>>& values);

        // Copy-constructor
        ColorGradient(const ColorGradient& other);

        // Is equals operator
        bool operator==(const ColorGradient& other) const;

        // Is not equals operator
        bool operator!=(const ColorGradient& other) const;

        // Assign operator
        ColorGradient& operator=(const ColorGradient& other);

        // Append other gradient at end
        ColorGradient& operator+=(const ColorGradient& other);

        // Returns value by position @SCRIPTABLE
        Color4 Evaluate(float position) const;

        // Returns value by position
        Color4 Evaluate(float position, bool direction, int& cacheKey) const;

        // Moves all keys positions to offset
        void MoveKeys(float offset);

        // Moves keys from begin position positions to offset
        void MoveKeysFrom(float begin, float offset);

        // Appends gradient at end
        void AppendCurve(const ColorGradient& gradient);

        // Prepends gradient at end
        void PrependCurve(const ColorGradient& gradient);

        // Inserts gradient at position
        void InsertCurve(const ColorGradient& gradient, float position);

        // Adds keys at end
        void AppendKeys(const Vector<Pair<float, Color4>>& values);

        // Adds keys at end
        void AppendKeys(const Vector<Key>& keys);

        // Adds keys at beginning
        void PrependKeys(const Vector<Pair<float, Color4>>& values);

        // Adds keys at beginning
        void PrependKeys(const Vector<Key>& keys);

        // Inserts keys at position
        void InsertKeys(const Vector<Pair<float, Color4>>& values, float position);

        // Inserts keys at position
        void InsertKeys(const Vector<Key>& keys, float position);

        // Inserts key and returns index
        int InsertKey(const Key& key);

        // Inserts key and returns index @SCRIPTABLE
        int InsertKey(float position, const Color4& value);

        // Appends key at end with offset and returns index
        int AppendKey(float offset, const Color4& value);

        // Prepends key at beginning with offset and returns index
        int PrependKey(float offset, const Color4& value);

        // Removes key at position
        bool RemoveKey(float position);

        // Removes key at index
        bool RemoveKeyAt(int idx);

        // Removes all keys
        void RemoveAllKeys();

        // Returns true if contains key at position
        bool ContainsKey(float position) const;

        // Returns keys array
        const Vector<Key>& GetKeys() const;

        // Returns key at position
        Key GetKey(float position) const;

        // Returns key at index
        Key GetKeyAt(int idx) const;

        // Returns key by uid
        Key FindKey(UInt64 uid) const;

        // Returns key index by uid
        int FindKeyIdx(UInt64 uid) const;

        // Sets keys
        void SetKeys(const Vector<Key>& keys);

        // Sets key data at position
        void SetKey(const Key& key, int idx);

        // Returns ColorGradient max position @SCRIPTABLE
        float Length() const;

        // Returns true if ColorGradient is empty @SCRIPTABLE
        bool IsEmpty() const;

        // Key access operator by position
        Key operator[](float position) const;

        SERIALIZABLE(ColorGradient);

    public:
        // ---------
        // Color key
        // ---------
        class Key: public ISerializable
        {
        public:
            UInt64 uid;             // Random unique id @SERIALIZABLE
            Color4 color;           // Value @SERIALIZABLE
            float  position = 0.0f; // Position @SERIALIZABLE

        public:
            //Default constructor
            Key();

            // Constructor from value and position
            Key(const Color4& value, float position);

            // Copy-constructor
            Key(const Key& other);

            // Copy operator
            Key& operator=(const Key& other);

            // Assign from value operator
            Key& operator=(const Color4& value);

            // Color4 cast operator
            operator Color4() const;

            // Check equals operator
            bool operator==(const Key& other) const;

            // Check not equals operator
            bool operator!=(const Key& other) const;

            SERIALIZABLE(Key);

            friend class ColorGradient;
        };

    protected:
        Vector<Key> mKeys; // ColorGradient keys @SERIALIZABLE

    protected:
        // Sort keys by position
        void SortKeys(); 
    };
}
// --- META ---

CLASS_BASES_META(o2::ColorGradient)
{
    BASE_CLASS(o2::ISerializable);
    BASE_CLASS(o2::RefCounterable);
}
END_META;
CLASS_FIELDS_META(o2::ColorGradient)
{
    FIELD().PUBLIC().NAME(keys);
    FIELD().PUBLIC().NAME(length);
    FIELD().PUBLIC().NAME(onKeysChanged);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().NAME(mKeys);
}
END_META;
CLASS_METHODS_META(o2::ColorGradient)
{

    typedef const Vector<Pair<float, Color4>>& _tmp1;
    typedef const Vector<Pair<float, Color4>>& _tmp2;
    typedef const Vector<Pair<float, Color4>>& _tmp3;
    typedef const Vector<Pair<float, Color4>>& _tmp4;

    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(const Color4&, const Color4&);
    FUNCTION().PUBLIC().CONSTRUCTOR(_tmp1);
    FUNCTION().PUBLIC().CONSTRUCTOR(const ColorGradient&);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(Color4, Evaluate, float);
    FUNCTION().PUBLIC().SIGNATURE(Color4, Evaluate, float, bool, int&);
    FUNCTION().PUBLIC().SIGNATURE(void, MoveKeys, float);
    FUNCTION().PUBLIC().SIGNATURE(void, MoveKeysFrom, float, float);
    FUNCTION().PUBLIC().SIGNATURE(void, AppendCurve, const ColorGradient&);
    FUNCTION().PUBLIC().SIGNATURE(void, PrependCurve, const ColorGradient&);
    FUNCTION().PUBLIC().SIGNATURE(void, InsertCurve, const ColorGradient&, float);
    FUNCTION().PUBLIC().SIGNATURE(void, AppendKeys, _tmp2);
    FUNCTION().PUBLIC().SIGNATURE(void, AppendKeys, const Vector<Key>&);
    FUNCTION().PUBLIC().SIGNATURE(void, PrependKeys, _tmp3);
    FUNCTION().PUBLIC().SIGNATURE(void, PrependKeys, const Vector<Key>&);
    FUNCTION().PUBLIC().SIGNATURE(void, InsertKeys, _tmp4, float);
    FUNCTION().PUBLIC().SIGNATURE(void, InsertKeys, const Vector<Key>&, float);
    FUNCTION().PUBLIC().SIGNATURE(int, InsertKey, const Key&);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(int, InsertKey, float, const Color4&);
    FUNCTION().PUBLIC().SIGNATURE(int, AppendKey, float, const Color4&);
    FUNCTION().PUBLIC().SIGNATURE(int, PrependKey, float, const Color4&);
    FUNCTION().PUBLIC().SIGNATURE(bool, RemoveKey, float);
    FUNCTION().PUBLIC().SIGNATURE(bool, RemoveKeyAt, int);
    FUNCTION().PUBLIC().SIGNATURE(void, RemoveAllKeys);
    FUNCTION().PUBLIC().SIGNATURE(bool, ContainsKey, float);
    FUNCTION().PUBLIC().SIGNATURE(const Vector<Key>&, GetKeys);
    FUNCTION().PUBLIC().SIGNATURE(Key, GetKey, float);
    FUNCTION().PUBLIC().SIGNATURE(Key, GetKeyAt, int);
    FUNCTION().PUBLIC().SIGNATURE(Key, FindKey, UInt64);
    FUNCTION().PUBLIC().SIGNATURE(int, FindKeyIdx, UInt64);
    FUNCTION().PUBLIC().SIGNATURE(void, SetKeys, const Vector<Key>&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetKey, const Key&, int);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(float, Length);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(bool, IsEmpty);
    FUNCTION().PROTECTED().SIGNATURE(void, SortKeys);
}
END_META;

CLASS_BASES_META(o2::ColorGradient::Key)
{
    BASE_CLASS(o2::ISerializable);
}
END_META;
CLASS_FIELDS_META(o2::ColorGradient::Key)
{
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().NAME(uid);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().NAME(color);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(0.0f).NAME(position);
}
END_META;
CLASS_METHODS_META(o2::ColorGradient::Key)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(const Color4&, float);
    FUNCTION().PUBLIC().CONSTRUCTOR(const Key&);
}
END_META;
// --- END META ---
