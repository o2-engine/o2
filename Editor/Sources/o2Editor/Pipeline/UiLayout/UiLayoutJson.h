#pragma once

#include <algorithm>
#include <vector>

#include "o2/Utils/Types/Containers/Pair.h"
#include "o2/Utils/Types/Containers/Vector.h"
#include "o2/Utils/Types/String.h"

using namespace o2;

namespace Editor::UiLayout
{
    // Sorts keeping the order of equal elements, through an index permutation (std::stable_sort straight over o2 vectors of
    // structs gave wrong orders)
    template<typename _type, typename _less>
    void StableSortBy(Vector<_type>& items, const _less& less)
    {
        std::vector<int> order(items.Count());
        for (int i = 0; i < items.Count(); i++)
            order[i] = i;
        std::stable_sort(order.begin(), order.end(), [&](int a, int b) { return less(items[a], items[b]); });

        Vector<_type> sorted;
        sorted.Reserve(items.Count());
        for (int i : order)
            sorted.Add(items[i]);
        items = sorted;
    }

    // True when the text ends with the suffix (o2's String::EndsWith answers true for a text one character shorter)
    inline bool HasSuffix(const String& text, const String& suffix)
    {
        return text.Length() >= suffix.Length() && text.SubStr(text.Length() - suffix.Length()) == suffix;
    }

    // ------------------------------------------------------------------------------------------------
    // JSON value with ordered object members, read and written the way JavaScript's JSON does: the node
    // writes ui_structure.json and trace.json with JSON.stringify's layout and number format, and reads
    // the model answers with JSON.parse's rules
    // ------------------------------------------------------------------------------------------------
    class Json
    {
    public:
        enum class Kind { Null, Bool, Number, String, Array, Object };

    public:
        Json() = default;
        Json(std::nullptr_t) {}
        Json(bool value): mKind(Kind::Bool), mBool(value) {}
        Json(int value): mKind(Kind::Number), mNumber(value) {}
        Json(Int64 value): mKind(Kind::Number), mNumber((double)value) {}
        Json(float value): mKind(Kind::Number), mNumber(value) {}
        Json(double value): mKind(Kind::Number), mNumber(value) {}
        Json(const char* value): mKind(Kind::String), mString(value) {}
        Json(const String& value): mKind(Kind::String), mString(value) {}

        // Returns an empty array
        static Json Array();

        // Returns an empty object
        static Json Object();

        Kind GetKind() const { return mKind; }
        bool IsNull() const { return mKind == Kind::Null; }
        bool IsBool() const { return mKind == Kind::Bool; }
        bool IsNumber() const { return mKind == Kind::Number; }
        bool IsString() const { return mKind == Kind::String; }
        bool IsArray() const { return mKind == Kind::Array; }
        bool IsObject() const { return mKind == Kind::Object; }

        // Returns the number, def for anything else
        double Num(double def = 0.0) const { return mKind == Kind::Number ? mNumber : def; }

        // Returns the string, def for anything else
        const String& Str(const String& def = String::empty) const { return mKind == Kind::String ? mString : def; }

        // Returns the flag, def for anything else
        bool Bool(bool def = false) const { return mKind == Kind::Bool ? mBool : def; }

        // JavaScript's String(value) for a primitive: numbers in their shortest form, null as "null"
        String ToJsString() const;

        // JavaScript's Number(value): NaN for objects, arrays and text that is not a number
        double ToJsNumber() const;

        // Returns the member, null when this is not an object or has no such member
        const Json* Get(const String& key) const;
        Json* Get(const String& key);

        // Returns the member, adding a null one (and turning this into an object) when missing
        Json& operator[](const String& key);
        Json& operator[](const char* key) { return (*this)[String(key)]; }

        // Returns true when the object has the member
        bool Has(const String& key) const { return Get(key) != nullptr; }

        // Removes the member when present
        void Remove(const String& key);

        // Returns the members of an object in order
        const Vector<Pair<String, Json>>& Members() const { return mMembers; }
        Vector<Pair<String, Json>>& Members() { return mMembers; }

        // Returns the elements of an array
        const Vector<Json>& Items() const { return mItems; }
        Vector<Json>& Items() { return mItems; }

        // Appends an element, turning this into an array
        Json& Add(const Json& value);

        // Returns the element count of an array, the member count of an object, else 0
        int Count() const;

        // Returns the element, null when out of range or not an array
        const Json* At(int index) const;

        // Writes the value as JSON.stringify(value, null, indent) does; indent 0 writes it on one line
        String Dump(int indent = 0) const;

        // Parses JSON text as JSON.parse does; false with the reason when it is not JSON
        static bool Parse(const String& text, Json& out, String* error = nullptr);

        // Formats a number as JavaScript's Number#toString does
        static String FormatNumber(double value);

    private:
        Kind   mKind = Kind::Null;
        bool   mBool = false;
        double mNumber = 0.0;
        String mString;

        Vector<Json>               mItems;
        Vector<Pair<String, Json>> mMembers;

    private:
        void Write(String& out, int indent, int depth) const;
    };
}
// --- META ---

PRE_ENUM_META(Editor::UiLayout::Json::Kind);
// --- END META ---
