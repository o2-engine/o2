#pragma once

#include <string>
#include <cstdarg>
#include "o2/Utils/Types/Containers/Vector.h"
#include "o2/Utils/Types/CommonTypes.h"

namespace o2
{
    template<typename T>
    class Vec2;

    template<typename T>
    class Rect;

    typedef Vec2<float> Vec2F;
    typedef Vec2<int>   Vec2I;

    typedef Rect<float> RectF;
    typedef Rect<int>   RectI;

    class Color4;

    template<typename T>
    struct Border;

    typedef Border<float> BorderF;
    typedef Border<int>   BorderI;


#define CheckCharacterType \
    typename X = typename std::enable_if<std::is_same<T2, char>::value || \
                                         std::is_same<T2, wchar_t>::value || \
                                         std::is_same<T2, const char>::value || \
                                         std::is_same<T2, const wchar_t>::value>::type

    // -------------------------
    // Template character string
    // -------------------------
    template<typename T>
    class TString: public std::basic_string<T>
    {
    public:
        // Default constructor
        TString();

        // Constructor from characters array
        TString(const T* data);

        // Constructor from characters array
        template<typename T2, CheckCharacterType>
        TString(const T2* data);

        // Copy-constructor
        TString(const TString& other);

        // Move-constructor
        TString(TString&& other);

        // Copy-constructor from string with another characters type
        template<typename T2, CheckCharacterType>
        TString(const TString<T2>& other);

        // Constructor from basic string
        TString(const std::basic_string<T>& data);

        // Explicit constructor from char value
        explicit TString(char value);

        // Explicit constructor from boolean value
        explicit TString(bool value);

        // Explicit constructor from integer value
        explicit TString(int value);

        // Explicit constructor from unsigned integer value
        explicit TString(UInt value);

        // Explicit constructor from unsigned integer value
        explicit TString(UInt64 value);

        // Explicit constructor from float value
        explicit TString(float value);

        // Explicit constructor from double value
        explicit TString(double value);

        // Explicit constructor from float vector value
        explicit TString(const Vec2F& value);

        // Explicit constructor from integer vactor value
        explicit TString(const Vec2I& value);

        // Explicit constructor from float rectangle value
        explicit TString(const RectF& value);

        // Explicit constructor from integer rectangle value
        explicit TString(const RectI& value);

        // Explicit constructor from float border value
        explicit TString(const BorderF& value);

        // Explicit constructor from integer border value
        explicit TString(const BorderI& value);

        // Explicit constructor from color value
        explicit TString(const Color4& value);

        // Destructor
        ~TString();

        // Copy-operator from string with another characters type
        template<typename T2, CheckCharacterType>
        TString& operator=(const TString<T2>& other);

        // Copy-operator
        TString& operator=(const TString& other);

        // Assign operator from characters array
        template<typename T2, CheckCharacterType>
        TString& operator=(const T2* data);

        // Cast to characters array
        operator const T*() const;

        // Cast operator to boolean
        explicit operator bool() const;

        // Cast operator to integer
        explicit operator int() const;

        // Cast operator to float
        explicit operator float() const;

        // Cast operator to unsigned integer
        explicit operator UInt() const;

        // Cast operator to unsigned integer
        explicit operator UInt64() const;

        // Cast operator to float vector
        explicit operator Vec2F() const;

        // Cast operator to integer vector
        explicit operator Vec2I() const;

        // Cast operator to float rectangle
        explicit operator RectF() const;

        // Cast operator to integer rectangle
        explicit operator RectI() const;

        // Cast operator to float border
        explicit operator BorderF() const;

        // Cast operator to integer border
        explicit operator BorderI() const;

        // Cast operator to color 
        explicit operator Color4() const;

        // Check equal operator to characters array
        template<typename T2, CheckCharacterType>
        bool operator==(const T2* data) const;

        // Check not equal operator to characters array
        template<typename T2, CheckCharacterType>
        bool operator!=(const T2* data) const;

        // Check equal operator
        bool operator==(const TString& other) const;

        // Check not equal operator
        bool operator!=(const TString& other) const;

        // Check greater operator
        bool operator>(const TString& other) const;

        // Plus operator - returns sum string
        TString operator+(const TString& other) const;

        // Plus and assign operator - adds string at the end
        TString& operator+=(const TString& other);

        // Plus operator - returns sum string
        TString operator+(T symbol) const;

        // Plus and assign operator - adds string at the end
        TString& operator+=(T symbol);

        // Plus operator - returns sum string
        template<typename T2, CheckCharacterType>
        TString operator+(T2 symbol) const;

        // Plus and assign operator - adds string at the end
        template<typename T2, CheckCharacterType>
        TString& operator+=(T2 symbol);

        // Plus operator - returns sum string
        TString operator+(const T* str) const;

        // Plus and assign operator - adds string at the end
        TString& operator+=(const T* str);

        // Plus operator - returns sum string
        template<typename T2, CheckCharacterType>
        TString operator+(const T2* str) const;

        // Plus and assign operator - adds string at the end
        template<typename T2, CheckCharacterType>
        TString& operator+=(const T2* str);

        // Symbol access operator
        T& operator[](int idx);

        // Return data
        const T* Data() const;

        // Returns length of string
        int Length() const;

        // Returns capacity of array
        int Capacity() const;

        // Returns true if string is empty
        bool IsEmpty() const;

        // Clears string
        void Clear();

        // Reserving array. Can't be less than current capacity
        void Reserve(int size);

        // Appends string at the end
        void Append(const TString& other);

        // Appends symbol at the end
        void Append(T symbol);

        // Returns sum string
        TString Appended(const TString& other) const;

        // Inserts string at position
        void Insert(const TString& other, int position = 0);

        // Inserts character at position
        void Insert(T character, int position = 0);

        // Returns string with inserted other string
        TString Inserted(const TString& other, int position = 0) const;

        // Removes symbols from begin to end
        void Erase(int begin, int end = -1);

        // Returns string with removed symbols from begin to end
        TString Erased(int begin, int end = -1) const;

        // Gets symbol from back, removes him and returns
        T PopBack();

        // Returns first symbol
        T First() const;

        // Returns last symbol
        T Last() const;

        // Replace part of string to another string in range
        void Replace(const TString& other, int begin, int end);

        // Returns string with replace by another string in range
        TString Replaced(const TString& other, int begin, int end) const;

        // Replaces all occurrences of all old strings to new
        void ReplaceAll(const TString& oldStr, const TString& newStr);

        // Replaces all occurrences of all old strings to new and returns as new string
        TString ReplacedAll(const TString& oldStr, const TString& newStr) const;

        // Return index of specified string. Returns -1 if can't find
        int Find(const TString& other, int startIdx = 0) const;

        // Return index of specified symbol. Returns -1 if can't find
        int Find(T symbol, int startIdx = 0) const;

        // Return this contains string. Returns -1 if can't find
        bool Contains(const TString& other, int startIdx = 0) const;

        // Return is this contains symbol. Returns -1 if can't find
        bool Contains(T symbol, int startIdx = 0) const;

        // Returns count of string inside this
        int CountOf(const TString& other, int startIdx = 0) const;

        // Returns last index of specified string. Returns -1 if can't find
        int FindLast(const TString& other, int startIdx = -1) const;

        // Returns true if string ends with specified string
        bool EndsWith(const TString& other) const;

        // Returns true if string starts with specified string
        bool StartsWith(const TString& other) const;

        // Returns subtracted string
        TString SubStr(int begin, int end = -1) const;

        // Splits string by specified string and returns results in vector
        Vector<TString> Split(const TString& splitStr) const;

        // Converts string to lower case
        TString ToLowerCase() const;

        // Converts string to upper case
        TString ToUpperCase() const;

        // Removing specified symbols at start of string
        void Trim(const TString& trimSymbols = " ");

        // Removing specified symbols at start of string
        void TrimStart(const TString& trimSymbols = " ");

        // Removing specified symbols at end of string
        void TrimEnd(const TString& trimSymbols = " ");

        // Removing specified symbols at start of string
        TString Trimed(const TString& trimSymbols = " ");

        // Removing specified symbols at start of string
        TString TrimedStart(const TString& trimSymbols = " ");

        // Removing specified symbols at end of string
        TString TrimedEnd(const TString& trimSymbols = " ");

        // Returns formatted string, like sprintf()
        static TString Format(TString format, ...);

        // Returns formatted string, like sprintf()
        static TString Format(TString format, va_list vlist);

		// Converts value to string
		template<typename T2>
		static TString ToString(const T2& value);

        static TString empty;
    };

    template<typename T>
    TString<T> TString<T>::empty;

    // ---------------------------
    // String with wide characters
    // ---------------------------
    typedef TString<wchar_t> WString;

    // ------------------------------
    // String with regular characters
    // ------------------------------
    typedef TString<char> String;

}

namespace std 
{
    template <>
    struct hash<o2::String>
    {
        std::size_t operator()(const o2::String& k) const
        {
            return hash<std::string>()(k);
        }
    };
}
