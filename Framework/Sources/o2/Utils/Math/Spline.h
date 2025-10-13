#pragma once

#include "o2/Utils/Math/ApproximationValue.h"
#include "o2/Utils/Math/Math.h"
#include "o2/Utils/Math/Vector2.h"
#include "o2/Utils/Property.h"
#include "o2/Utils/Serialization/Serializable.h"
#include "o2/Utils/Types/Containers/Vector.h"

namespace o2
{
    // -------------
    // Bezier spline
    // -------------
    class Spline: public ISerializable, public RefCounterable
    {
    public:
        class Key;

    public:
        PROPERTIES(Spline);
        PROPERTY(Vector<Key>, keys, SetKeys, GetKeys); // Keys property
        GETTER(float, length, Length);                 // Length of spline

    public:
        Function<void()> onKeysChanged; // Keys changed event

    public:
        // Default constructor
        Spline();

        // Constructor from keys with smoothing
        Spline(const Vector<Vec2F>& values, bool smooth = true);

        // Copy-constructor
        Spline(const Spline& other);

        // Is equals operator
        bool operator==(const Spline& other) const;

        // Is not equals operator
        bool operator!=(const Spline& other) const;

        // Assign operator
        Spline& operator=(const Spline& other);

        // Append other Spline at end
        Spline& operator+=(const Spline& other);

        // Returns value by position
        Vec2F Evaluate(float position, float randomRangeCoef = 0.0f) const;

        // Returns value by position
        Vec2F Evaluate(float position, float randomRangeCoef, bool direction, int& cacheKey, int& cacheKeyApprox) const;

        // Called when beginning keys batch change. After this call all keys modifications will not be update approximation
        // Used for optimizing many keys change
        void BeginKeysBatchChange();

        // Called when keys batch change completed. Updates approximation
        void CompleteKeysBatchingChange();

        // Sets spline closed or not
        void SetClosed(bool closed);

        // Returns is spline closed
        bool IsClosed() const;

        // Appends Spline at end
        void AppendSpline(const Spline& Spline);

        // Prepends Spline at end
        void PrependSpline(const Spline& Spline);

        // Inserts Spline at position
        void InsertSpline(const Spline& Spline, int idx);

        // Adds keys with smoothing at end
        void AppendKeys(const Vector<Vec2F>& values, bool smooth = true);

        // Adds keys at end
        void AppendKeys(const Vector<Key>& keys);

        // Adds keys with smoothing at beginning
        void PrependKeys(const Vector<Vec2F>& values, bool smooth = true);

        // Adds keys at beginning
        void PrependKeys(const Vector<Key>& keys);

        // Inserts keys with smoothing at position
        void InsertKeys(const Vector<Vec2F>& values, int idx, bool smooth = true);

        // Inserts keys at position
        void InsertKeys(const Vector<Key>& keys, int idx);

        // Inserts key and returns index
        void InsertKey(const Key& key, int idx);

        // Inserts key and returns index
        void InsertKey(int idx, const Vec2F& position, float rangeValue, const Vec2F& prevSupport, const Vec2F& nextSupport);

        // Inserts value at position with smoothing and returns index
        void InsertKey(int idx, const Vec2F&, float rangeValue = 0.0f, float smoothCoef = 1.0f);

        // Appends key at end with offset and returns index
        int AppendKey(const Vec2F&, float rangeValue, const Vec2F& prevSupport, const Vec2F& nextSupport);

        // Appends key at end with offset and smoothing and returns index
        int AppendKey(const Vec2F&, float rangeValue = 0.0f, float smoothCoef = 1.0f);

        // Prepends key at beginning with offset and returns index
        int PrependKey(const Vec2F&, float rangeValue, const Vec2F& prevSupport, const Vec2F& nextSupport);

         // Prepends key at beginning with offset and smoothing and returns index
        int PrependKey(const Vec2F&, float rangeValue = 0.0f, float smoothCoef = 1.0f);

        // Removes key at index
        bool RemoveKey(int idx);

        // Removes all keys
        void RemoveAllKeys();

        // Returns keys array
        const Vector<Key>& GetKeys() const;

        // Returns key at index
        Key GetKey(int idx) const;

        // Returns key by uid
        Key FindKey(UInt64 uid) const;

        // Returns next key at position
        int GetNextKey(float position) const;

        // Returns key index by uid
        int FindKeyIdx(UInt64 uid) const;

        // Sets keys
        void SetKeys(const Vector<Key>& keys);

        // Sets key data at position
        void SetKey(const Key& key, int idx);

        // Smooths key at position
        void SmoothKey(int idx, float smoothCoef = 1.0f);

        // Returns length of spline
        float Length() const;

        // Returns true if Spline is empty
        bool IsEmpty() const;

        // Returns bounding approximated keys rectangle
        RectF GetRect() const;

        // Key access operator by index
        const Key& operator[](int idx) const;

        // Key access operator by index
        Key& operator[](int idx);

        SERIALIZABLE(Spline);

    public:
        // ---------
        // Spline key
        // ---------
        class Key: public ISerializable
        {
        public:
            enum class Type { Smooth, Free, Broken };

        public:
            UInt64 uid = Math::Random();        // Random unique id @SERIALIZABLE
            float  position = 0.0f;             // Position from start, or length @SERIALIZABLE
            Vec2F  value;                       // Main position @SERIALIZABLE
            float  valueRange = 0.0f;           // Value range @SERIALIZABLE
            Vec2F  prevSupport;                 // Previous support point @SERIALIZABLE
            Vec2F  nextSupport;                 // Next support point @SERIALIZABLE
            Type   supportsType = Type::Smooth; // Type of support points @SERIALIZABLE

        public:
            //Default constructor
            Key();

            // Constructor
            Key(const Vec2F& value, float valueRange, const Vec2F& prevSupport, const Vec2F& nextSupport);

            // Copy-constructor
            Key(const Key& other);

            // Copy operator
            Key& operator=(const Key& other);

            // Assign from value operator
            Key& operator=(const Vec2F& position);

            // Vec2F cast operator
            operator Vec2F() const;

            // Check equals operator
            bool operator==(const Key& other) const;

            // Check not equals operator
            bool operator!=(const Key& other) const;

            // Returns approximated points
            const ApproximationVec2F* GetApproximatedPointsLeft() const;

            // Returns approximated points
            const ApproximationVec2F* GetApproximatedPointsRight() const;

            // Returns approximated points count
            int GetApproximatedPointsCount() const;

            // Returns bounds of approximation values
            const RectF& GetGetApproximatedPointsBounds() const;

            // Returns range normal. Prev and next can be nullptr
            static Vec2F GetRangeNormal(const Vec2F& position, Vec2F* prev, Vec2F* next);

            SERIALIZABLE(Key);

        public:
            static const int   mApproxValuesCount = 20;                // Approximation values count
            ApproximationVec2F mLeftApproxValues[mApproxValuesCount];  // Approximation values from left side
            ApproximationVec2F mRightApproxValues[mApproxValuesCount]; // Approximation values from right side
            RectF              mApproxValuesBounds;                    // Bounds of approximation values

            friend class Spline;
        };

    protected:
        bool mBatchChange = false; // It is true when began batch change
        bool mChangedKeys = false; // It is true when some keys changed during batch change

        Vector<Key> mKeys; // Spline keys @SERIALIZABLE

        bool mClosed = false; // Is spline closed

    protected:
        // Checks all smooth keys and updates supports points
        void CheckSmoothKeys();

        // Updates approximation
        void UpdateApproximation();

        // Returns keys (for property)
        Vector<Key> GetKeysNonContant();

        // Completion deserialization callback
        void OnDeserialized(const DataValue& node) override;

        // Smooths key at index and doesn't updates approximation
        void InternalSmoothKeyAt(int idx, float smoothCoef = 1.0f);
    };
}
// --- META ---

PRE_ENUM_META(o2::Spline::Key::Type);

CLASS_BASES_META(o2::Spline)
{
    BASE_CLASS(o2::ISerializable);
    BASE_CLASS(o2::RefCounterable);
}
END_META;
CLASS_FIELDS_META(o2::Spline)
{
    FIELD().PUBLIC().NAME(keys);
    FIELD().PUBLIC().NAME(length);
    FIELD().PUBLIC().NAME(onKeysChanged);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mBatchChange);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mChangedKeys);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().NAME(mKeys);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mClosed);
}
END_META;
CLASS_METHODS_META(o2::Spline)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(const Vector<Vec2F>&, bool);
    FUNCTION().PUBLIC().CONSTRUCTOR(const Spline&);
    FUNCTION().PUBLIC().SIGNATURE(Vec2F, Evaluate, float, float);
    FUNCTION().PUBLIC().SIGNATURE(Vec2F, Evaluate, float, float, bool, int&, int&);
    FUNCTION().PUBLIC().SIGNATURE(void, BeginKeysBatchChange);
    FUNCTION().PUBLIC().SIGNATURE(void, CompleteKeysBatchingChange);
    FUNCTION().PUBLIC().SIGNATURE(void, SetClosed, bool);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsClosed);
    FUNCTION().PUBLIC().SIGNATURE(void, AppendSpline, const Spline&);
    FUNCTION().PUBLIC().SIGNATURE(void, PrependSpline, const Spline&);
    FUNCTION().PUBLIC().SIGNATURE(void, InsertSpline, const Spline&, int);
    FUNCTION().PUBLIC().SIGNATURE(void, AppendKeys, const Vector<Vec2F>&, bool);
    FUNCTION().PUBLIC().SIGNATURE(void, AppendKeys, const Vector<Key>&);
    FUNCTION().PUBLIC().SIGNATURE(void, PrependKeys, const Vector<Vec2F>&, bool);
    FUNCTION().PUBLIC().SIGNATURE(void, PrependKeys, const Vector<Key>&);
    FUNCTION().PUBLIC().SIGNATURE(void, InsertKeys, const Vector<Vec2F>&, int, bool);
    FUNCTION().PUBLIC().SIGNATURE(void, InsertKeys, const Vector<Key>&, int);
    FUNCTION().PUBLIC().SIGNATURE(void, InsertKey, const Key&, int);
    FUNCTION().PUBLIC().SIGNATURE(void, InsertKey, int, const Vec2F&, float, const Vec2F&, const Vec2F&);
    FUNCTION().PUBLIC().SIGNATURE(void, InsertKey, int, const Vec2F&, float, float);
    FUNCTION().PUBLIC().SIGNATURE(int, AppendKey, const Vec2F&, float, const Vec2F&, const Vec2F&);
    FUNCTION().PUBLIC().SIGNATURE(int, AppendKey, const Vec2F&, float, float);
    FUNCTION().PUBLIC().SIGNATURE(int, PrependKey, const Vec2F&, float, const Vec2F&, const Vec2F&);
    FUNCTION().PUBLIC().SIGNATURE(int, PrependKey, const Vec2F&, float, float);
    FUNCTION().PUBLIC().SIGNATURE(bool, RemoveKey, int);
    FUNCTION().PUBLIC().SIGNATURE(void, RemoveAllKeys);
    FUNCTION().PUBLIC().SIGNATURE(const Vector<Key>&, GetKeys);
    FUNCTION().PUBLIC().SIGNATURE(Key, GetKey, int);
    FUNCTION().PUBLIC().SIGNATURE(Key, FindKey, UInt64);
    FUNCTION().PUBLIC().SIGNATURE(int, GetNextKey, float);
    FUNCTION().PUBLIC().SIGNATURE(int, FindKeyIdx, UInt64);
    FUNCTION().PUBLIC().SIGNATURE(void, SetKeys, const Vector<Key>&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetKey, const Key&, int);
    FUNCTION().PUBLIC().SIGNATURE(void, SmoothKey, int, float);
    FUNCTION().PUBLIC().SIGNATURE(float, Length);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsEmpty);
    FUNCTION().PUBLIC().SIGNATURE(RectF, GetRect);
    FUNCTION().PROTECTED().SIGNATURE(void, CheckSmoothKeys);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateApproximation);
    FUNCTION().PROTECTED().SIGNATURE(Vector<Key>, GetKeysNonContant);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDeserialized, const DataValue&);
    FUNCTION().PROTECTED().SIGNATURE(void, InternalSmoothKeyAt, int, float);
}
END_META;

CLASS_BASES_META(o2::Spline::Key)
{
    BASE_CLASS(o2::ISerializable);
}
END_META;
CLASS_FIELDS_META(o2::Spline::Key)
{
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(Math::Random()).NAME(uid);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(0.0f).NAME(position);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().NAME(value);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(0.0f).NAME(valueRange);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().NAME(prevSupport);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().NAME(nextSupport);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(Type::Smooth).NAME(supportsType);
    FIELD().PUBLIC().NAME(mLeftApproxValues);
    FIELD().PUBLIC().NAME(mRightApproxValues);
    FIELD().PUBLIC().NAME(mApproxValuesBounds);
}
END_META;
CLASS_METHODS_META(o2::Spline::Key)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(const Vec2F&, float, const Vec2F&, const Vec2F&);
    FUNCTION().PUBLIC().CONSTRUCTOR(const Key&);
    FUNCTION().PUBLIC().SIGNATURE(const ApproximationVec2F*, GetApproximatedPointsLeft);
    FUNCTION().PUBLIC().SIGNATURE(const ApproximationVec2F*, GetApproximatedPointsRight);
    FUNCTION().PUBLIC().SIGNATURE(int, GetApproximatedPointsCount);
    FUNCTION().PUBLIC().SIGNATURE(const RectF&, GetGetApproximatedPointsBounds);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(Vec2F, GetRangeNormal, const Vec2F&, Vec2F*, Vec2F*);
}
END_META;
// --- END META ---
