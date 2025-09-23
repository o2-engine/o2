#include "o2/stdafx.h"
#include "DataValue.h"

#include "o2/Utils/FileSystem/FileSystem.h"
#include "o2/Utils/Serialization/JsonDataFormat.h"

#include "rapidjson/document.h"

namespace o2
{
    DataValue::DataValue(DataDocument& document):
        mDocument(&document), mData()
    {
        mData.flagsData.flags = Flags::Null;
    }

    DataValue::DataValue(const DataValue& other, DataDocument& document):
        mData(), mDocument(&document)
    {
        if (other.IsObject())
        {
            for (auto memberIt = other.BeginMember(); memberIt != other.EndMember(); ++memberIt)
            {
                DataValue name(memberIt->name, *mDocument);
                AddMember(name) = memberIt->value;
            }
        }
        else if (other.IsArray())
        {
            for (auto element : other)
            {
                DataValue newElement(element, *mDocument);
                AddElement(newElement);
            }
        }
        else if (other.IsString())
        {
            SetString(other.GetString(), other.GetStringLength(), true);
        }
        else
            mData = other.mData;
    }

    DataValue::DataValue(const DataValue& other):
        DataValue(other, *other.mDocument)
    {}

    DataValue::DataValue() :
        mData(), mDocument(nullptr)
    {
        mData.flagsData.flags = Flags::Null;
    }

    DataValue::DataValue(const char* string, int length, bool isCopy, DataDocument& document):
        mData(), mDocument(&document)
    {
        SetString(string, length, isCopy);
    }

    DataValue::DataValue(DataValue&& other):
        mDocument(other.mDocument), mData(other.mData)
    {
        other.mData.flagsData.flags = Flags::Null;
    }

    DataValue::DataValue(const char* stringRef):
        mData(), mDocument(nullptr)
    {
        mData.flagsData.flags = Flags::String | Flags::StringRef;
        mData.stringPtrData.stringPtr = const_cast<char*>(stringRef);
        mData.stringPtrData.stringLength = strlen(stringRef);
    }

    bool DataValue::Transcode(rapidjson::GenericStringBuffer<rapidjson::UTF8<>>& target, const wchar_t* source)
    {
        rapidjson::GenericStringStream<rapidjson::UTF16<>> sourceStream(source);

        bool hasError = false;
        while (sourceStream.Peek() != '\0')
        {
            if (!rapidjson::Transcoder<rapidjson::UTF16<>, rapidjson::UTF8<>>::Transcode(sourceStream, target))
            {
                hasError = true;
                break;
            }
        }

        return !hasError;
    }

    bool DataValue::Transcode(rapidjson::GenericStringBuffer<rapidjson::UTF16<>>& target, const char* source)
    {
        rapidjson::GenericStringStream<rapidjson::UTF8<>> sourceStream(source);

        bool hasError = false;
        while (sourceStream.Peek() != '\0')
        {
            if (!rapidjson::Transcoder<rapidjson::UTF8<>, rapidjson::UTF16<>>::Transcode(sourceStream, target))
            {
                hasError = true;
                break;
            }
        }

        return !hasError;
    }

    DataValue::~DataValue()
    {
        // Not required, document's allocator does not require freeing memory
    }

    DataDocument& DataValue::GetDocument()
    {
        return *mDocument;
    }

    const DataDocument& DataValue::GetDocument() const
    {
        return *mDocument;
    }

    bool DataValue::IsValue() const
    {
        return mData.flagsData.Is(Flags::Value);
    }

    bool DataValue::IsNumber() const
    {
        return mData.flagsData.Is(Flags::Int) || mData.flagsData.Is(Flags::UInt) ||
            mData.flagsData.Is(Flags::Int64) || mData.flagsData.Is(Flags::UInt64) ||
            mData.flagsData.Is(Flags::Double);
    }

    bool DataValue::IsString() const
    {
        return mData.flagsData.Is(Flags::String);
    }

    bool DataValue::IsBoolean() const
    {
        return mData.flagsData.Is(Flags::Bool);
    }

    bool DataValue::IsNull() const
    {
        return mData.flagsData.Is(Flags::Null);
    }

    void DataValue::SetNull()
    {
        mData.flagsData.flags = Flags::Null;
    }

    void DataValue::SetString(const char* string, int length, bool isCopy)
    {
        if (isCopy)
        {
            if (length <= ShortStringData::maxLength)
            {
                memcpy(mData.shortStringData.stringValue, string, (length + 1)*sizeof(char));
                mData.flagsData.flags = Flags::String | Flags::ShortString;
            }
            else
            {
                UInt64 size = length*sizeof(char);
                mData.stringPtrData.stringPtr = (char*)mDocument->mAllocator.Allocate(size + 1);
                memcpy(mData.stringPtrData.stringPtr, string, size);
                mData.stringPtrData.stringPtr[size] = '\0';
                mData.stringPtrData.stringLength = (size_t)length;
                mData.flagsData.flags = Flags::String | Flags::StringCopy;
            }
        }
        else
        {
            mData.stringPtrData.stringPtr = const_cast<char*>(string);
            mData.stringPtrData.stringLength = length;
            mData.flagsData.flags = Flags::String | Flags::StringRef;
        }
    }

    const char* DataValue::GetString() const
    {
        Assert(mData.flagsData.Is(Flags::String), "Can't get string, value isn't string");

        if (mData.flagsData.Is(Flags::ShortString))
            return mData.shortStringData.stringValue;

        return mData.stringPtrData.stringPtr;
    }

    int DataValue::GetStringLength() const
    {
        Assert(mData.flagsData.Is(Flags::String), "Can't get string length, value isn't string");

        if (mData.flagsData.Is(Flags::ShortString))
            return strlen(mData.shortStringData.stringValue);

        return (int)mData.stringPtrData.stringLength;
    }

    bool DataValue::IsArray() const
    {
        return mData.flagsData.Is(Flags::Array);
    }

    bool DataValue::IsObject() const
    {
        return mData.flagsData.Is(Flags::Object);
    }

    int DataValue::GetElementsCount() const
    {
        if (!IsArray())
            return -1;

        return mData.arrayData.count;
    }

    bool DataValue::operator!=(const DataValue& other) const
    {
        return !(*this == other);
    }

    bool DataValue::operator==(const DataValue& other) const
    {
        if (IsArray())
        {
            if (!other.IsArray())
                return false;

            if (GetElementsCount() != other.GetElementsCount())
                return false;

            for (int i = 0; i < GetElementsCount(); i++)
            {
                if (GetElement(i) != other.GetElement(i))
                    return false;
            }

            return true;
        }

        if (IsObject())
        {
            if (!other.IsObject())
                return false;

            if (mData.objectData.count != other.mData.objectData.count)
                return false;

            for (auto memberIt = BeginMember(); memberIt != EndMember(); ++memberIt)
            {
                auto otherMember = other.FindMember(memberIt->name);
                if (!otherMember || *otherMember != memberIt->value)
                    return false;
            }

            return true;
        }

        if (IsString())
        {
            if (!other.IsString())
                return false;

            if (mData.flagsData.Is(Flags::StringRef) && other.mData.flagsData.Is(Flags::StringRef) &&
                mData.stringPtrData.stringPtr == other.mData.stringPtrData.stringPtr)
            {
                return true;
            }

            return strcmp(GetString(), other.GetString()) == 0;
        }

        if (IsNumber())
        {
            if (mData.flagsData.Is(Flags::Double) || mData.flagsData.Is(Flags::Double))
            {
                double a, b;
                Get(a); other.Get(b);
                return a >= b && a <= b;
            }
            else if (mData.flagsData.Is(Flags::Int))
                return (int)(*this) == (int)other;
            else if (mData.flagsData.Is(Flags::UInt))
                return (UInt)(*this) == (UInt)other;
            else if (mData.flagsData.Is(Flags::Int64))
                return (Int64)(*this) == (Int64)other;
            else if (mData.flagsData.Is(Flags::UInt64))
                return (UInt64)(*this) == (UInt64)other;

            return false;
        }

        if (IsBoolean())
            return other.IsBoolean() && (bool)(*this) == (bool)other;

        if (IsNull())
            return other.IsNull();

        return false;
    }

    DataValue& DataValue::operator=(const DataValue& other)
    {
        Clear();

        if (other.IsObject())
        {
            for (auto memberIt = other.BeginMember(); memberIt != other.EndMember(); ++memberIt)
            {
                DataValue name(memberIt->name, *mDocument);
                AddMember(name) = memberIt->value;
            }
        }
        else if (other.IsArray())
        {
            for (auto element : other)
            {
                DataValue newElement(element, *mDocument);
                AddElement(newElement);
            }
        }
        else if (other.IsString())
        {
            SetString(other.GetString(), other.GetStringLength(), true);
        }
        else
            mData = other.mData;

        return *this;
    }

    DataValue& DataValue::operator=(DataValue&& other)
    {
        mDocument = other.mDocument;
        mData = other.mData;
        other.mData.flagsData.flags = Flags::Null;
        return *this;
    }

    DataValue& DataValue::operator[](const DataValue& name)
    {
        return GetMember(name);
    }

    DataValue& DataValue::operator[](const char* name)
    {
        return GetMember(name);
    }

    const DataValue& DataValue::operator[](const DataValue& name) const
    {
        return GetMember(name);
    }

    const DataValue& DataValue::operator[](const char* name) const
    {
        return GetMember(name);
    }

    int DataValue::GetMembersCount() const
    {
        if (!IsObject())
            return 0;

        return mData.objectData.count;
    }

    DataValue& DataValue::GetMember(const DataValue& name)
    {
        if (auto res = FindMember(name))
            return *res;

        DataValue t(name, *mDocument);
        return AddMember(t);
    }

    const DataValue& DataValue::GetMember(const DataValue& name) const
    {
        if (auto res = FindMember(name))
            return *res;

        Assert(false, "Can't find data member");

        static DataValue empty;
        return empty;
    }

    DataValue& DataValue::GetMember(const char* name)
    {
        DataValue t(name);
        return GetMember(t);
    }

    const DataValue& DataValue::GetMember(const char* name) const
    {
        return GetMember(DataValue(name));
    }

    DataValue* DataValue::FindMember(const DataValue& name)
    {
        if (!IsObject())
            return nullptr;

        for (auto memberIt = BeginMember(); memberIt != EndMember(); ++memberIt)
        {
            if (memberIt->name == name)
                return &memberIt->value;
        }

        return nullptr;
    }

    const DataValue* DataValue::FindMember(const DataValue& name) const
    {
        if (!IsObject())
            return nullptr;

        for (auto memberIt = BeginMember(); memberIt != EndMember(); ++memberIt)
        {
            if (memberIt->name == name)
                return &memberIt->value;
        }

        return nullptr;
    }

    DataValue* DataValue::FindMember(const char* name)
    {
        return FindMember(DataValue(name));
    }

    const DataValue* DataValue::FindMember(const char* name) const
    {
        return FindMember(DataValue(name));
    }

    void DataValue::SetObject()
    {
        if (IsObject())
            return;

        mData.flagsData.flags = Flags::Object;

        mData.objectData.members = (DataMember*)mDocument->mAllocator.Allocate(sizeof(DataMember)*ObjectInitialCapacity);
        mData.objectData.capacity = ObjectInitialCapacity;
        mData.objectData.count = 0;
    }

    DataValue& DataValue::AddMember(DataValue& name)
    {
        SetObject();

        if (mData.objectData.count == mData.objectData.capacity)
        {
            if (mData.objectData.members)
            {
                UInt newCapacity = Math::Max(mData.objectData.capacity*2, ObjectInitialCapacity);
                mData.objectData.members = (DataMember*)mDocument->mAllocator.Reallocate(
                    mData.objectData.members, sizeof(DataMember)*mData.objectData.capacity,
                    sizeof(DataMember)*newCapacity);

                mData.objectData.capacity = newCapacity;
            }
            else
            {
                mData.objectData.members = (DataMember*)mDocument->mAllocator.Allocate(sizeof(DataMember)*ObjectInitialCapacity);
                mData.objectData.capacity = ObjectInitialCapacity;
            }
        }

        DataValue value(*mDocument);

        DataMember* newMember =
            new (mData.objectData.members + mData.objectData.count) DataMember(name, value);

        mData.objectData.count++;

        return newMember->value;
    }

    DataValue& DataValue::AddMember(const char* name)
    {
        DataValue nameValue(name, *mDocument);
        return AddMember(nameValue);
    }

    void DataValue::RemoveMember(const DataValue& name)
    {
        Assert(IsObject(), "Trying remove member, but value isn't object");

        for (auto memberIt = BeginMember(); memberIt != EndMember(); ++memberIt)
        {
            if (memberIt->name == name)
            {
                *memberIt = *(mData.objectData.members + mData.objectData.count - 1);
                mData.objectData.count--;
                break;
            }
        }
    }

    DataMemberIterator DataValue::RemoveMember(DataMemberIterator it)
    {
        Assert(IsObject(), "Trying remove member, but value isn't object");
        Assert(it >= BeginMember() && it < EndMember(), "Iterator is invalid");

        *it = *(mData.objectData.members + mData.objectData.count - 1);
        mData.objectData.count--;

        return it;
    }

    void DataValue::RemoveMember(const char* name)
    {
        RemoveMember(DataValue(name));
    }

    DataMemberIterator DataValue::BeginMember()
    {
        Assert(IsObject(), "Trying get member iterator, but value isn't object");
        return DataMemberIterator(mData.objectData.members);
    }

    ConstDataMemberIterator DataValue::BeginMember() const
    {
        Assert(IsObject(), "Trying get member iterator, but value isn't object");
        return ConstDataMemberIterator(mData.objectData.members);
    }

    DataMemberIterator DataValue::EndMember()
    {
        Assert(IsObject(), "Trying get member iterator, but value isn't object");
        return DataMemberIterator(mData.objectData.members + mData.objectData.count);
    }

    ConstDataMemberIterator DataValue::EndMember() const
    {
        Assert(IsObject(), "Trying get member iterator, but value isn't object");
        return ConstDataMemberIterator(mData.objectData.members + mData.objectData.count);
    }

    DataValue& DataValue::operator[](const int idx)
    {
        return GetElement(idx);
    }

    DataValue& DataValue::GetElement(int idx)
    {
        Assert(IsArray(), "Trying get element, but value isn't array");
        Assert(idx >= 0 && idx < (int)mData.arrayData.count, "Index is out of range");
        return mData.arrayData.elements[idx];
    }

    const DataValue& DataValue::GetElement(int idx) const
    {
        Assert(IsArray(), "Trying get element, but value isn't array");
        Assert(idx >= 0 && idx < (int)mData.arrayData.count, "Index is out of range");
        return mData.arrayData.elements[idx];
    }

    void DataValue::SetArray()
    {
        if (IsArray())
            return;

        mData.flagsData.flags = Flags::Array;

        mData.arrayData.elements = (DataValue*)mDocument->mAllocator.Allocate(sizeof(DataValue)*ArrayInitialCapacity);
        mData.arrayData.capacity = ArrayInitialCapacity;
        mData.arrayData.count = 0;
    }

    DataValue& DataValue::AddElement(DataValue& value)
    {
        DataValue& newElement = AddElement();
        newElement = std::move(value);
        return newElement;
    }

    DataValue& DataValue::AddElement()
    {
        SetArray();

        if (mData.arrayData.count == mData.arrayData.capacity)
        {
            UInt newCapacity = Math::Max(mData.arrayData.capacity*2, ArrayInitialCapacity);
            mData.arrayData.elements = (DataValue*)mDocument->mAllocator.Reallocate(
                mData.arrayData.elements, sizeof(DataValue)*mData.arrayData.capacity,
                sizeof(DataValue)*newCapacity);
            mData.arrayData.capacity = newCapacity;
        }

        DataValue* newElement =
            new (mData.arrayData.elements + mData.arrayData.count) DataValue(*mDocument);

        mData.arrayData.count++;

        return *newElement;
    }

    DataValue* DataValue::RemoveElement(DataValue* it)
    {
        Assert(IsArray(), "Trying get element, but value isn't array");
        Assert(it >= Begin() && it < End(), "Iterator is invalid");

        *it = *(mData.arrayData.elements + mData.arrayData.count - 1);
        mData.arrayData.count--;

        return it;
    }

    DataValue* DataValue::Begin()
    {
        Assert(IsArray(), "Trying get element iterator, but value isn't array");
        return mData.arrayData.elements;
    }

    const DataValue* DataValue::Begin() const
    {
        Assert(IsArray(), "Trying get element iterator, but value isn't array");
        return mData.arrayData.elements;
    }

    DataValue* DataValue::End()
    {
        Assert(IsArray(), "Trying get element iterator, but value isn't array");
        return mData.arrayData.elements + mData.arrayData.count;
    }

    const DataValue* DataValue::End() const
    {
        Assert(IsArray(), "Trying get element iterator, but value isn't array");
        return mData.arrayData.elements + mData.arrayData.count;
    }

    DataValue* DataValue::begin()
    {
        Assert(IsArray(), "Trying get element iterator, but value isn't array");
        return mData.arrayData.elements;
    }

    const DataValue* DataValue::begin() const
    {
        Assert(IsArray(), "Trying get element iterator, but value isn't array");
        return mData.arrayData.elements;
    }

    DataValue* DataValue::end()
    {
        Assert(IsArray(), "Trying get element iterator, but value isn't array");
        return mData.arrayData.elements + mData.arrayData.count;
    }

    const DataValue* DataValue::end() const
    {
        Assert(IsArray(), "Trying get element iterator, but value isn't array");
        return mData.arrayData.elements + mData.arrayData.count;
    }

    bool DataValue::IsEmpty() const
    {
        if (IsObject())
            return mData.objectData.count == 0;

        if (IsArray())
            return mData.arrayData.count == 0;

        return true;
    }

    void DataValue::Clear()
    {
        if (IsObject())
            mData.objectData.count = 0;
        else if (IsArray())
            mData.arrayData.count = 0;
        else
            mData.flagsData.flags = Flags::Null;
    }

    DataDocument::DataDocument():
        DataValue(*this), mAllocator()
    {}

    DataDocument::DataDocument(const DataDocument& other) :
        mAllocator()
    {
		*this = other;
    }

    DataDocument::DataDocument(DataDocument&& other) :
        DataValue(other), mAllocator(other.mAllocator)
    {}

    DataDocument::~DataDocument()
    {
        mAllocator.Clear();
    }

    bool DataDocument::operator!=(const DataDocument& other) const
    {
        return DataValue::operator!=(other);
    }

    bool DataDocument::operator==(const DataDocument& other) const
    {
        return DataValue::operator==(other);
    }

    DataDocument& DataDocument::operator=(const DataDocument& other)
    {
        DataValue::operator=(other);
        return *this;
    }

    DataDocument& DataDocument::operator=(DataDocument&& other)
    {
        DataValue::operator=(other);
        mAllocator = other.mAllocator;
        return *this;
    }

    bool DataDocument::LoadFromFile(const String& fileName, Format format /*= Format::JSON*/)
    {
        InFile file(fileName);
        if (!file.IsOpened())
            return false;

        auto size = file.GetDataSize();
        char* data = (char*)mAllocator.Allocate(size);
        file.ReadData(data, size);

        if (format == Format::JSON)
            return ParseJsonInplace(data, *this);

        return false;
    }

    bool DataDocument::LoadFromData(const String& data, Format format /*= Format::JSON*/)
    {
        if (format == Format::JSON)
            return ParseJson(data.Data(), *this);

        return false;
    }

    bool DataDocument::SaveToFile(const String& fileName, Format format /*= Format::JSON*/) const
    {
        String data = SaveAsString(format);

        if (!o2FileSystem.IsFolderExist(o2FileSystem.GetParentPath(fileName)))
            o2FileSystem.FolderCreate(o2FileSystem.GetParentPath(fileName));

        OutFile file(fileName);
        if (!file.IsOpened())
            return false;

        file.WriteData(data.Data(), data.Length());

        return false;
    }

    String DataDocument::SaveAsString(Format format /*= Format::JSON*/) const
    {
        if (format == Format::JSON)
        {
            String buf;
            WriteJson(buf, *this);
            return buf;
        }

        return "";
        //return XmlDataFormat::SaveDataDoc(*this);
    }

    DataValue::Flags operator&(const DataValue::Flags& a, const DataValue::Flags& b)
    {
        return static_cast<DataValue::Flags>(
            static_cast<std::underlying_type<DataValue::Flags>::type>(a) &
            static_cast<std::underlying_type<DataValue::Flags>::type>(b));
    }

    DataValue::Flags operator|(const DataValue::Flags& a, const DataValue::Flags& b)
    {
        return static_cast<DataValue::Flags>(
            static_cast<std::underlying_type<DataValue::Flags>::type>(a) |
            static_cast<std::underlying_type<DataValue::Flags>::type>(b));
    }

    DataMember::DataMember(DataValue& name, DataValue& value):
        name(name), value(value)
    {}

    DataMember& DataMember::operator=(DataMember& other)
    {
        name = other.name;
        value = other.value;
        return *this;
    }

    bool DataMember::operator==(const DataMember& other) const
    {
        return name == other.name && value == other.value;
    }

    bool DataMember::operator!=(const DataMember& other) const
    {
        return !(*this == other);
    }

}
// --- META ---

ENUM_META(o2::DataValue::Flags)
{
    ENUM_ENTRY(Bool);
}
END_ENUM_META;

ENUM_META(o2::DataDocument::Format)
{
    ENUM_ENTRY(Binary);
    ENUM_ENTRY(JSON);
    ENUM_ENTRY(Xml);
}
END_ENUM_META;
// --- END META ---
