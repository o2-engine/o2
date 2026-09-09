#pragma once

#include "o2/Utils/Memory/Allocators/ChunkPoolAllocator.h"
#include "o2/Utils/Types/Containers/Map.h"
#include "o2/Utils/Types/Containers/Vector.h"
#include "o2/Utils/Types/String.h"
#include "o2/Utils/Types/UID.h"

#include "rapidjson/stream.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/encodings.h"

namespace o2
{
    class ISerializable;

    class DataDocument;
    struct DataMember;

    template <bool _const>
    class BaseMemberIterator;

    typedef BaseMemberIterator<false> DataMemberIterator;
    typedef BaseMemberIterator<true> ConstDataMemberIterator;

    // ----------------------------------------------
    // DOM value. Contains value, or array, or object
    // ----------------------------------------------
    class DataValue
    {
    protected:
        // Specialize this template class for your custom serialization types
        template<typename _type, typename _enable = void>
        struct Converter
        {
            static constexpr bool isSupported = false;
            using __type = typename std::conditional<std::is_same<void, _type>::value, int, _type>::type;

            static void Write(const __type& value, DataValue& data) {}
            static void Read(__type& value, const DataValue& data) {}
        };

        // Specialize this template class for your custom serialization types with DELTA
        template<typename _type, typename _enable = void>
        struct DeltaConverter
        {
            static constexpr bool isSupported = Converter<_type>::isSupported;
            using __type = typename std::conditional<std::is_same<void, _type>::value, int, _type>::type;

            static void Write(const __type& value, const __type& origin, DataValue& data);
            static void Read(__type& value, const __type& origin, const DataValue& data);
        };

    public:
        // Is DataValue supporting type trait
        template<typename _type>
        struct IsSupports
        {
            static constexpr bool value = Converter<_type>::isSupported;
        };

        // Is DataValue supporting delta type trait
        template<typename _type>
        struct IsSupportsDelta
        {
            static constexpr bool value = DeltaConverter<_type>::isSupported;
        };

    public:
        // Default constructor
        DataValue(DataDocument& document);

        // Constructor as template value
        template<typename _type>
        DataValue(const _type& value, DataDocument& document);

        // Constructor as string value
        DataValue(const char* string, int length, bool isCopy, DataDocument& document);

        // Copy-constructor with same document
        DataValue(const DataValue& other);

        // Copy-constructor with another document
        DataValue(const DataValue& other, DataDocument& document);

        // Move-constructor
        DataValue(DataValue&& other);

        // Destructor
        ~DataValue();

        // Copy operator
        DataValue& operator=(const DataValue& other);

        // Move operator
        DataValue& operator=(DataValue&& other);

        // Equals operator
        bool operator==(const DataValue& other) const;

        // Not equals operator
        bool operator!=(const DataValue& other) const;

        // Returns data document reference
        DataDocument& GetDocument();

        // Returns data document reference
        const DataDocument& GetDocument() const;

    public:    // Value methods
        // Checks is value type
        bool IsValue() const;

        // Checks is value number type
        bool IsNumber() const;

        // Checks is value string value
        bool IsString() const;

        // Checks is value boolean
        bool IsBoolean() const;

        // Checks is value null
        bool IsNull() const;

        // Cast to type operator
        template<typename _type>
        operator _type() const;

        // Assign operator
        template<typename _type>
        DataValue& operator=(const _type& value);

        // Sets value. Using DataValue::Converter specializations
        template<typename _type>
        DataValue& Set(const _type& value);

        // Gets value. Using DataValue::Converter specializations
        template<typename _type>
        void Get(_type& value) const;

        // Sets value. Using DataValue::DeltaConverter specializations
        template<typename _type>
        DataValue& SetDelta(const _type& value, const _type& origin);

        // Gets value. Using DataValue::DeltaConverter specializations
        template<typename _type>
        void GetDelta(_type& value, const _type& origin) const;

        // Sets value as null
        void SetNull();

        // Optimized set string. When isCopy is false, string pointer can be used as reference
        void SetString(const char* string, int length, bool isCopy);

        // Returns string pointer
        const char* GetString() const;

        // Returns string length
        int GetStringLength() const;

    public: // Array methods
        // Checks value is array
        bool IsArray() const;

        // Sets data value type as array
        void SetArray();

        // Returns array elements count
        int GetElementsCount() const;

        // Array element access by index
        DataValue& operator[](const int idx);

        // Array methods
        // Returns element by index
        DataValue& GetElement(int idx);

        // Returns element by index
        const DataValue& GetElement(int idx) const;

        // Adds element to array
        DataValue& AddElement(DataValue& value);

        // Adds element to array
        DataValue& AddElement();

        // Adds element to array
        DataValue* RemoveElement(DataValue* it);

        // Begin array iterator
        DataValue* Begin();

        // Begin array const iterator
        const DataValue* Begin() const;

        // End array iterator
        DataValue* End();

        // End array const iterator
        const DataValue* End() const;

        // Begin array iterator for range based for
        DataValue* begin();

        // Begin array const iterator for range based for
        const DataValue* begin() const;

        // End array iterator for range based for
        DataValue* end();

        // End array const iterator for range based for
        const DataValue* end() const;

    public: // Object methods        
        // Checks value is object
        bool IsObject() const;

        // Sets data value type as object
        void SetObject();

        // Member access by name. Valid only for object type
        DataValue& operator[](const DataValue& name);

        // Member access by name. Valid only for object type
        DataValue& operator[](const char* name);

        // Member access by name. Valid only for object type
        const DataValue& operator[](const DataValue& name) const;

        // Member access by name. Valid only for object type
        const DataValue& operator[](const char* name) const;

        // Returns members count
        int GetMembersCount() const;

        // Returns node by name. Creates new nodes when required
        DataValue& GetMember(const DataValue& name);

        // Returns node by name. Creates new nodes when required
        DataValue& GetMember(const char* name);

        // Returns node by name.
        const DataValue& GetMember(const DataValue& name) const;

        // Returns node by name.
        const DataValue& GetMember(const char* name) const;

        // Returns node by name.
        DataValue* FindMember(const DataValue& name);

        // Returns node by name.
        DataValue* FindMember(const char* name);

        // Returns node by name.
        const DataValue* FindMember(const DataValue& name) const;

        // Returns node by name.
        const DataValue* FindMember(const char* name) const;

        // Add new node with name
        DataValue& AddMember(DataValue& name);

        // Add new node with name
        DataValue& AddMember(const char* name);

        // Removes node by name
        void RemoveMember(const DataValue& name);

        // Removes node by name
        void RemoveMember(const char* name);

        // Removes member
        DataMemberIterator RemoveMember(DataMemberIterator it);

        // Begin array iterator
        DataMemberIterator BeginMember();

        // Begin array const iterator
        ConstDataMemberIterator BeginMember() const;

        // End array iterator
        DataMemberIterator EndMember();

        // End array const iterator
        ConstDataMemberIterator EndMember() const;

    public:
        // Returns is node data empty and have no members or elements
        bool IsEmpty() const;

        // Removes all members or elements
        void Clear();

        // Writes data to writer
        template <typename _writer>
        void Write(_writer& writer) const;

    public:
        enum class Flags
        {
            Bool = 1 << 0,
            Int = 1 << 1,
            UInt = 1 << 2,
            Int64 = 1 << 3,
            UInt64 = 1 << 4,
            Double = 1 << 5,
            Value = 1 << 6,
            String = 1 << 7,
            Object = 1 << 8,
            Array = 1 << 9,
            Null = 1 << 10,

            BoolTrue = 1 << 11,
            BoolFalse = 1 << 12,

            ShortString = 1 << 13,
            StringRef = 1 << 14,
            StringCopy = 1 << 15
        };

    protected:
        static constexpr UInt DataPayloadSize = 16;
        static constexpr UInt DataSize = DataPayloadSize + sizeof(Flags);

        static constexpr UInt ObjectInitialCapacity = 7;
        static constexpr UInt ArrayInitialCapacity = 7;

        struct IntData
        {
            int intValue;
            UInt uintValue;
            Byte padding[DataPayloadSize - sizeof(int)];
        };

        struct Int64Data
        {
            Int64 intValue;
            UInt64 uintValue;
            Byte padding[DataPayloadSize - sizeof(Int64)];
        };

        struct DoubleData
        {
            double value;
            Byte padding[DataPayloadSize - sizeof(double)];
        };

        struct StringPtrData
        {
            char* stringPtr;
            size_t stringLength;
        };

        struct ShortStringData
        {
            static constexpr int maxLength = DataPayloadSize / sizeof(Byte) - 1;
            char stringValue[maxLength + 1];
        };

        struct ValueTypeData
        {
            Byte padding[DataPayloadSize];
            Flags flags;

            inline bool Is(Flags f) const
            {
                return static_cast<Flags>(
                    static_cast<std::underlying_type<Flags>::type>(flags) &
                    static_cast<std::underlying_type<Flags>::type>(f)) == f;
            }
        };

        struct ObjectData
        {
            DataMember* members;
            UInt count;
            UInt capacity;
        };

        struct ArrayData
        {
            DataValue* elements;
            UInt count;
            UInt capacity;
        };

        union ValueData
        {
            IntData         intData;
            Int64Data       int64Data;
            DoubleData      doubleData;
            StringPtrData   stringPtrData;
            ShortStringData shortStringData;
            ObjectData      objectData;
            ArrayData       arrayData;

            ValueTypeData flagsData;
        };

        ValueData     mData;
        DataDocument* mDocument;

    protected:
        // Hidden constructor without document
        DataValue();

        // Constructor temporary string reference
        explicit DataValue(const char* stringRef);

        // Transcode wide char to char
        static bool Transcode(rapidjson::GenericStringBuffer<rapidjson::UTF8<>>& target, const wchar_t* source);

        // Transcode char to wide char
        static bool Transcode(rapidjson::GenericStringBuffer<rapidjson::UTF16<>>& target, const char* source);

        friend class JsonDataDocumentParseHandler;
        friend class DataDocument;

        template<typename T>
        friend class TType;
    };

    // ------------------------------------------
    // Data values document. Must be object value
    // ------------------------------------------
    class DataDocument : public DataValue
    {
    public:
        enum class Format { Xml, JSON, Binary };

    public:
        // Default constructor
        DataDocument();

        // Copy-constructor
        DataDocument(const DataDocument& other);

        // Move-constructor
        DataDocument(DataDocument&& other);

        // Destructor
        ~DataDocument();

        // Copy-operator. 
        DataDocument& operator=(const DataDocument& other);

        // Move-operator
        DataDocument& operator=(DataDocument&& other);

        // Equals operator
        bool operator==(const DataDocument& other) const;

        // Not equals operator
        bool operator!=(const DataDocument& other) const;

        operator std::nullptr_t() const = delete;

        // Cast to type operator
        template<typename _type>
        operator _type() const;

        // Assign operator
        template<typename _type>
        DataDocument& operator=(const _type& value);

        // Loads data structure from file
        bool LoadFromFile(const String& fileName, Format format = Format::JSON);

        // Loads data structure from string
        bool LoadFromData(const String& data, Format format = Format::JSON);

        // Saves data to file with specified format
        bool SaveToFile(const String& fileName, Format format = Format::JSON) const;

        // Saves data to string
        String SaveAsString(Format format = Format::JSON) const;

    protected:
        ChunkPoolAllocator mAllocator;

    protected:
        // Points every nested value at this document after a move
        void RebindDocument(DataValue& value);

        friend class DataValue;
        friend class JsonDataDocumentParseHandler;
    };

    // --------------------------------------------
    // Data object value member. Has name and value
    // --------------------------------------------
    struct DataMember
    {
        DataValue name;
        DataValue value;

        DataMember(DataValue& name, DataValue& value);

        DataMember& operator=(DataMember& other);

        bool operator==(const DataMember& other) const;
        bool operator!=(const DataMember& other) const;
    };

    // -------------------------
    // Base data member iterator
    // -------------------------
    template <bool _const>
    class BaseMemberIterator
    {
        typedef typename std::conditional<_const, const DataMember, DataMember>::type DataMemberType;

    public:
        typedef DataMemberType value_type;
        typedef DataMemberType* pointer;
        typedef DataMemberType& reference;
        typedef std::ptrdiff_t difference_type;
        typedef std::random_access_iterator_tag iterator_category;

    public:
        BaseMemberIterator() {}
        BaseMemberIterator(const BaseMemberIterator<true>& other) : mPointer(other.mPointer) {}

        operator bool() const { return mPointer != nullptr; }

        BaseMemberIterator<_const>& operator=(const BaseMemberIterator<true>& other) { mPointer = other.mPointer; return *this; }

        int operator-(BaseMemberIterator<true> other) const { return mPointer - other.mPointer; }

        BaseMemberIterator<_const>& operator++() { ++mPointer; return *this; }
        BaseMemberIterator<_const>& operator--() { --mPointer; return *this; }
        BaseMemberIterator<_const>  operator++(int) { BaseMemberIterator<_const> old(*this); ++mPointer; return old; }
        BaseMemberIterator<_const>  operator--(int) { BaseMemberIterator<_const> old(*this); --mPointer; return old; }

        BaseMemberIterator<_const> operator+(int n) const { return Iterator(mPointer + n); }
        BaseMemberIterator<_const> operator-(int n) const { return Iterator(mPointer - n); }

        BaseMemberIterator<_const>& operator+=(int n) { mPointer += n; return *this; }
        BaseMemberIterator<_const>& operator-=(int n) { mPointer -= n; return *this; }

        template <bool _const_> bool operator==(const BaseMemberIterator<_const_>& other) const { return mPointer == other.mPointer; }
        template <bool _const_> bool operator!=(const BaseMemberIterator<_const_>& other) const { return mPointer != other.mPointer; }
        template <bool _const_> bool operator<=(const BaseMemberIterator<_const_>& other) const { return mPointer <= other.mPointer; }
        template <bool _const_> bool operator>=(const BaseMemberIterator<_const_>& other) const { return mPointer >= other.mPointer; }
        template <bool _const_> bool operator< (const BaseMemberIterator<_const_>& other) const { return mPointer < other.mPointer; }
        template <bool _const_> bool operator> (const BaseMemberIterator<_const_>& other) const { return mPointer > other.mPointer; }

        DataMemberType& operator*() const { return *mPointer; }
        DataMemberType* operator->() const { return mPointer; }
        DataMemberType& operator[](int n) const { return mPointer[n]; }

        bool IsValid() const { return mPointer != nullptr; }

    private:
        explicit BaseMemberIterator(DataMemberType* p) : mPointer(p) {}

    private:
        DataMemberType* mPointer = nullptr;

        friend class DataValue;
    };

    template<typename _type, typename _enable = void>
    struct IsDeltaEquals
    {
        static bool Check(const _type& obj, const _type& origin);
    };

    template<typename T>
    struct IsDeltaEquals<T, typename std::void_t<decltype(&T::EqualsDelta)>>
    {
        static bool Check(const T& obj, const T& origin)
        {
            return T::EqualsDelta(obj, origin);
        }
    };

    template<typename T>
    bool EqualsForDeltaSerialize(const T& obj, const T& origin)
    {
        return IsDeltaEquals<T>::Check(obj, origin);
    }

    DataValue::Flags operator&(const DataValue::Flags& a, const DataValue::Flags& b);
    DataValue::Flags operator|(const DataValue::Flags& a, const DataValue::Flags& b);
}

#include "o2/Utils/Reflection/Reflection.h"
#include "o2/Utils/Property.h"

namespace o2
{
    template<typename _type, typename _enable>
    bool IsDeltaEquals<_type, _enable>::Check(const _type& obj, const _type& origin)
    {
        if constexpr (SupportsEqualOperator<_type>::value)
            return Math::Equals(obj, origin);
        else
            return false;
    }

    template<typename _type>
    DataValue::DataValue(const _type& value, DataDocument& document) :
        mData(), mDocument(&document)
    {
        Set(value);
    }

    template<typename _type>
    void DataValue::Get(_type& value) const
    {
        Converter<_type>::Read(value, *this);
    }

    template<typename _type>
    DataValue& DataValue::Set(const _type& value)
    {
        Converter<_type>::Write(value, *this);
        return *this;
    }

    template<typename _type>
    void DataValue::GetDelta(_type& value, const _type& origin) const
    {
        DeltaConverter<_type>::Read(value, origin, *this);
    }

    template<typename _type>
    DataValue& DataValue::SetDelta(const _type& value, const _type& origin)
    {
        DeltaConverter<_type>::Write(value, origin, *this);
        return *this;
    }

    template<typename _type>
    DataValue::operator _type() const
    {
        _type result = _type();
        Get(result);
        return result;
    }

    template<typename _type>
    DataValue& DataValue::operator=(const _type& value)
    {
        Set(value);
        return *this;
    }

    template <typename _writer>
    void DataValue::Write(_writer& writer) const
    {
        if (IsObject())
        {
            writer.StartObject();

            for (auto it = BeginMember(); it != EndMember(); ++it)
            {
                writer.Key(it->name.GetString(), it->name.GetStringLength(), it->name.mData.flagsData.Is(Flags::StringCopy));
                it->value.Write(writer);
            }

            writer.EndObject(mData.objectData.count);
        }
        else if (IsArray())
        {
            writer.StartArray();

            for (auto it = Begin(); it != End(); ++it)
                it->Write(writer);

            writer.EndArray(mData.arrayData.count);
        }
        else if (IsString())
        {
            writer.String(GetString(), GetStringLength(), mData.flagsData.Is(Flags::StringCopy));
        }
        else if (mData.flagsData.Is(Flags::Int))
        {
            writer.Int(mData.intData.intValue);
        }
        else if (mData.flagsData.Is(Flags::UInt))
        {
            writer.Uint(mData.intData.uintValue);
        }
        else if (mData.flagsData.Is(Flags::Int64))
        {
            writer.Int64(mData.int64Data.intValue);
        }
        else if (mData.flagsData.Is(Flags::UInt64))
        {
            writer.Uint64(mData.int64Data.uintValue);
        }
        else if (mData.flagsData.Is(Flags::Double))
        {
            writer.Double(mData.doubleData.value);
        }
        else if (mData.flagsData.Is(Flags::BoolTrue))
        {
            writer.Bool(true);
        }
        else if (mData.flagsData.Is(Flags::BoolFalse))
        {
            writer.Bool(false);
        }
        else
        {
            writer.Null();
        }
    }

    template<typename _type>
    DataDocument& DataDocument::operator=(const _type& value)
    {
        DataValue::operator=(value);
        return *this;
    }

    template<typename _type>
    DataDocument::operator _type() const
    {
        _type result = _type();
        Get(result);
        return result;
    }
}

#include "o2/Utils/Serialization/DataValueConverters.h"
// --- META ---

PRE_ENUM_META(o2::DataValue::Flags);

PRE_ENUM_META(o2::DataDocument::Format);
// --- END META ---
