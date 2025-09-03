#pragma once

#include "3rdPartyLibs/utf8cpp/utf8.h"
#include "o2/Utils/Math/Border.h"
#include "o2/Utils/Math/Color.h"
#include "o2/Utils/Math/Rect.h"
#include "o2/Utils/Math/Vector2.h"

namespace o2
{
    inline String operator+(const char* left, const String& right)
    {
        String _left(left);
        return _left + right;
    }

    inline WString operator+(const wchar_t* left, const WString& right)
    {
        WString _left(left);
        return _left + right;
    }

    inline void ConvertString(TString<char>& dest, const TString<wchar_t>& source)
    {
        dest.Clear();
        utf8::utf16to8(source.begin(), source.end(), back_inserter(dest));
    }

    inline void ConvertString(TString<wchar_t>& dest, const TString<char>& source)
    {
        dest.Clear();
        utf8::utf8to16(source.begin(), source.end(), back_inserter(dest));
    }

    inline void ConvertString(TString<wchar_t>& dest, const TString<wchar_t>& source)
    {
        dest = source;
    }

    inline void ConvertString(TString<char>& dest, const TString<char>& source)
    {
        dest = source;
    }

    template<typename T>
    TString<T>::TString()
    {}

    template<typename T>
    TString<T>::TString(const T* data):
        std::basic_string<T>(data)
    {}

    template<typename T>
    TString<T>::TString(const std::basic_string<T>& data):
        std::basic_string<T>(data)
    {}

    template<typename T>
    TString<T>::TString(const TString& other):
        std::basic_string<T>(other)
    { }

    template<typename T>
    TString<T>::TString(TString&& other) :
        std::basic_string<T>(other)
    { }

    template<typename T>
    template<typename T2, typename X>
    TString<T>::TString(const T2* data)
    {
        ConvertString(*this, TString<T2>(data));
    }


    template<typename T>
    template<typename T2, typename X>
    TString<T>::TString(const TString<T2>& other)
    {
        ConvertString(*this, other);
    }

    template<typename T>
    TString<T>::TString(char value):
        std::basic_string<T>(1, value)
    {}

    template<typename T>
    TString<T>::TString(bool value)
    {
        *this = value ? "true" : "false";
    }

    template<typename T>
    TString<T>::TString(int value)
    {
        ConvertString(*this, TString<T>(std::to_string(value).c_str()));
    }

    template<typename T>
    TString<T>::TString(UInt value)
    {
        ConvertString(*this, TString<T>(std::to_string(value).c_str()));
    }

    template<typename T>
    TString<T>::TString(UInt64 value)
    {
        ConvertString(*this, TString<T>(std::to_string(value).c_str()));
    }

    template<typename T>
    TString<T>::TString(float value)
    {
        ConvertString(*this, TString<T>(std::to_string(value).c_str()));
        std::basic_string<T>::erase(std::basic_string<T>::find_last_not_of('0') + 1, std::string::npos);
        std::basic_string<T>::erase(std::basic_string<T>::find_last_not_of('.') + 1, std::string::npos);
    }

    template<typename T>
    TString<T>::TString(double value)
    {
        ConvertString(*this, TString<T>(std::to_string(value).c_str()));
        std::basic_string<T>::erase(std::basic_string<T>::find_last_not_of('0') + 1, std::string::npos);
        std::basic_string<T>::erase(std::basic_string<T>::find_last_not_of('.') + 1, std::string::npos);
    }

    template<typename T>
    TString<T>::TString(const Vec2F& value)
    {
        Append((TString)value.x);
        Append((TString)";");
        Append((TString)value.y);
    }

    template<typename T>
    TString<T>::TString(const Vec2I& value)
    {
        Append((TString)value.x);
        Append((TString)";");
        Append((TString)value.y);
    }

    template<typename T>
    TString<T>::TString(const RectF& value)
    {
        Append((TString)value.left);
        Append((TString)";");
        Append((TString)value.top);
        Append((TString)";");
        Append((TString)value.right);
        Append((TString)";");
        Append((TString)value.bottom);
    }

    template<typename T>
    TString<T>::TString(const RectI& value)
    {
        Append((TString)value.left);
        Append((TString)";");
        Append((TString)value.top);
        Append((TString)";");
        Append((TString)value.right);
        Append((TString)";");
        Append((TString)value.bottom);
    }

    template<typename T>
    TString<T>::TString(const BorderF& value)
    {
        Append((TString)value.left);
        Append((TString)";");
        Append((TString)value.top);
        Append((TString)";");
        Append((TString)value.right);
        Append((TString)";");
        Append((TString)value.bottom);
    }

    template<typename T>
    TString<T>::TString(const BorderI& value)
    {
        Append((TString)value.left);
        Append((TString)";");
        Append((TString)value.top);
        Append((TString)";");
        Append((TString)value.right);
        Append((TString)";");
        Append((TString)value.bottom);
    }

    template<typename T>
    TString<T>::TString(const Color4& value)
    {
        Append((TString)value.r);
        Append((TString)";");
        Append((TString)value.g);
        Append((TString)";");
        Append((TString)value.b);
        Append((TString)";");
        Append((TString)value.a);
    }

    template<typename T>
    TString<T>::~TString()
    {
        //~std::basic_string<T>();
    }

    template<typename T>
    template<typename T2, typename X>
    TString<T>& TString<T>::operator=(const TString<T2>& other)
    {
        Reserve(other.Capacity());
        ConvertString(*this, other);
        return *this;
    }

    template<typename T>
    TString<T>& TString<T>::operator=(const TString& other)
    {
        std::basic_string<T>::operator=(other);
        return *this;
    }

    template<typename T>
    template<typename T2, typename X>
    TString<T>& TString<T>::operator=(const T2* data)
    {
        ConvertString(*this, TString<T2>(data));
        return *this;
    }

    template<typename T>
    TString<T>::operator const T*() const
    {
        return std::basic_string<T>::c_str();
    }

    template<typename T>
    TString<T>::operator bool() const
    {
        return *this == "true" || *this == "TRUE" || *this == "True";
    }

    template<typename T>
    TString<T>::operator int() const
    {
        return std::stoi(*this);
    }

    template<typename T>
    TString<T>::operator float() const
    {
        if (std::basic_string<T>::empty())
            return 0.0f;

        return std::stof(*this);
    }

    template<typename T>
    TString<T>::operator UInt() const
    {
        return (UInt)std::stoi(*this);
    }


    template<typename T>
    TString<T>::operator UInt64() const
    {
        return std::stoull(*this);
    }

    template<typename T>
    TString<T>::operator Vec2F() const
    {
        if constexpr (std::is_same<T, char>::value)
        {
            wchar_t *px = std::basic_string<T>::c_str(), *py;
            return Vec2F(wcstof(px, &py), wcstof(py, nullptr));
        }
        else
        {
            char *px = std::basic_string<T>::c_str(), *py;
            return Vec2F(strtof(px, &py), strtof(py, nullptr));
        }
    }

    template<typename T>
    TString<T>::operator Vec2I() const
    {
        if constexpr (std::is_same<T, wchar_t>::value)
        {
            wchar_t *px = std::basic_string<T>::c_str(), *py;
            return Vec2F((int)wcstol(px, &py, 10), (int)wcstol(py, nullptr, 10));
        }
        else
        {
            char *px = std::basic_string<T>::c_str(), *py;
            return Vec2F((int)strtol(px, &py, 10), (int)strtol(py, nullptr, 10));
        }
    }

    template<typename T>
    TString<T>::operator RectF() const
    {
        if constexpr (std::is_same<T, char>::value)
        {
            wchar_t *pl = std::basic_string<T>::c_str(), *pb, *pr, *pt;
            return RectF(wcstof(pl, &pb), wcstof(pb, &pr), wcstof(pr, &pt), wcstof(pt, nullptr));
        }
        else
        {
            char *pl = std::basic_string<T>::c_str(), *pb, *pr, *pt;
            return RectF(strtof(pl, &pb), strtof(pb, &pr), strtof(pr, &pt), strtof(pt, nullptr));
        }
    }

    template<typename T>
    TString<T>::operator RectI() const
    {
        if constexpr (std::is_same<T, char>::value)
        {
            wchar_t *pl = std::basic_string<T>::c_str(), *pb, *pr, *pt;
            return RectF((int)wcstol(pl, &pb, 10), (int)wcstol(pb, &pr, 10), (int)wcstol(pr, &pt, 10), (int)wcstol(pt, nullptr, 10));
        }
        else
        {
            char *pl = std::basic_string<T>::c_str(), *pb, *pr, *pt;
            return RectF((int)strtol(pl, &pb, 10), (int)strtol(pb, &pr, 10), (int)strtol(pr, &pt, 10), (int)strtol(pt, nullptr, 10));
        }
    }

    template<typename T>
    TString<T>::operator BorderF() const
    {
        if constexpr (std::is_same<T, char>::value)
        {
            wchar_t *pl = std::basic_string<T>::c_str(), *pb, *pr, *pt;
            return BorderF(wcstof(pl, &pb), wcstof(pb, &pr), wcstof(pr, &pt), wcstof(pt, nullptr));
        }
        else
        {
            char *pl = std::basic_string<T>::c_str(), *pb, *pr, *pt;
            return BorderF(strtof(pl, &pb), strtof(pb, &pr), strtof(pr, &pt), strtof(pt, nullptr));
        }
    }

    template<typename T>
    TString<T>::operator BorderI() const
    {
        if constexpr (std::is_same<T, char>::value)
        {
            wchar_t *pl = std::basic_string<T>::c_str(), *pb, *pr, *pt;
            return BorderI((int)wcstol(pl, &pb, 10), (int)wcstol(pb, &pr, 10), (int)wcstol(pr, &pt, 10), (int)wcstol(pt, nullptr, 10));
        }
        else
        {
            char *pl = std::basic_string<T>::c_str(), *pb, *pr, *pt;
            return BorderI((int)strtol(pl, &pb, 10), (int)strtol(pb, &pr, 10), (int)strtol(pr, &pt, 10), (int)strtol(pt, nullptr, 10));
        }
    }

    template<typename T>
    TString<T>::operator Color4() const
    {
        if constexpr (std::is_same<T, char>::value)
        {
            wchar_t *pr = std::basic_string<T>::c_str(), *pg, *pb, *pa;
            return Color4((int)wcstol(pr, &pg, 10), (int)wcstol(pg, &pb, 10), (int)wcstol(pb, &pa, 10), (int)wcstol(pa, nullptr, 10));
        }
        else
        {
            char *pr = std::basic_string<T>::c_str(), *pg, *pb, *pa;
            return Color4((int)strtol(pr, &pg, 10), (int)strtol(pg, &pb, 10), (int)strtol(pb, &pa, 10), (int)strtol(pa, nullptr, 10));
        }
    }

    template<typename T>
    template<typename T2, typename X>
    bool TString<T>::operator==(const T2* data) const
    {
        return std::basic_string<T>::compare(TString<T>(data)) == 0;
    }

    template<typename T>
    template<typename T2, typename X>
    bool TString<T>::operator!=(const T2* data) const
    {
        return std::basic_string<T>::compare(TString<T>(data)) != 0;
    }

    template<typename T>
    bool TString<T>::operator==(const TString& other) const
    {
        return std::basic_string<T>::compare(other) == 0;
    }

    template<typename T>
    bool TString<T>::operator!=(const TString& other) const
    {
        return std::basic_string<T>::compare(other) != 0;
    }

    template<typename T>
    bool TString<T>::operator>(const TString& other) const
    {
        return std::basic_string<T>::compare(other) > 0;
    }

    template<typename T>
    TString<T> TString<T>::operator+(const TString& other) const
    {
        TString res(*this);
        res.Append(other);
        return res;
    }

    template<typename T>
    TString<T>& TString<T>::operator+=(const TString& other)
    {
        Append(other);
        return *this;
    }

    template<typename T>
    TString<T>& TString<T>::operator+=(T symbol)
    {
        Append(symbol);
        return *this;
    }

    template<typename T>
    TString<T> TString<T>::operator+(T symbol) const
    {
        TString res(*this);
        res.Append(symbol);
        return res;
    }

    template<typename T>
    template<typename T2, typename X>
    TString<T>& TString<T>::operator+=(T2 symbol)
    {
        TString<T2> str("x");
        str[0] = symbol;
        Append(str);
        return *this;
    }

    template<typename T>
    template<typename T2, typename X>
    TString<T> TString<T>::operator+(T2 symbol) const
    {
        TString res(*this);
        TString<T2> str("x");
        str[0] = symbol;
        res.Append(str);
        return res;
    }

    template<typename T>
    TString<T>& TString<T>::operator+=(const T* str)
    {
        Append(str);
        return *this;
    }

    template<typename T>
    TString<T> TString<T>::operator+(const T* str) const
    {
        TString res(*this);
        res.Append(str);
        return res;
    }

    template<typename T>
    template<typename T2, typename X>
    TString<T>& TString<T>::operator+=(const T2* str)
    {
        Append(TString<T>(str));
        return *this;
    }

    template<typename T>
    template<typename T2, typename X>
    TString<T> TString<T>::operator+(const T2* str) const
    {
        TString res(*this);
        res.Append(TString<T>(str));
        return res;
    }

    template<typename T>
    T& TString<T>::operator[](int idx)
    {
        return std::basic_string<T>::operator[](idx);
    }

    template<typename T>
    const T* TString<T>::Data() const
    {
        return std::basic_string<T>::c_str();
    }

    template<typename T>
    void TString<T>::Reserve(int size)
    {
        std::basic_string<T>::reserve(size);
    }

    template<typename T>
    int TString<T>::Length() const
    {
        return (int)std::basic_string<T>::length();
    }

    template<typename T>
    int TString<T>::Capacity() const
    {
        return (int)std::basic_string<T>::capacity();
    }


    template<typename T>
    void TString<T>::Clear()
    {
        std::basic_string<T>::clear();
    }

    template<typename T>
    bool TString<T>::IsEmpty() const
    {
        return std::basic_string<T>::length() == 0;
    }

    template<typename T>
    void TString<T>::Append(const TString& other)
    {
        std::basic_string<T>::operator+=(other);
    }

    template<typename T>
    void TString<T>::Append(T symbol)
    {
        std::basic_string<T>::operator+=(symbol);
    }

    template<typename T>
    void TString<T>::Insert(const TString& other, int position /*= 0*/)
    {
        std::basic_string<T>::insert(position, other);
    }

    template<typename T>
    void TString<T>::Insert(T character, int position /*= 0*/)
    {
        std::basic_string<T>::insert(std::basic_string<T>::begin() + position, character);
    }

    template<typename T>
    void TString<T>::Erase(int begin, int end /*= -1*/)
    {
        std::basic_string<T>::erase(begin, end - begin);
    }

    template<typename T>
    void TString<T>::Replace(const TString& other, int begin, int end)
    {
        if (end < begin)
            return;

        Erase(begin, end);
        Insert(other, begin);
    }

    template<typename T>
    TString<T> TString<T>::Replaced(const TString& other, int begin, int end) const
    {
        TString res(*this);
        res.Replace(other, begin, end);
        return res;
    }

    template<typename T>
    TString<T> TString<T>::ReplacedAll(const TString& oldStr, const TString& newStr) const
    {
        TString res(*this);
        res.ReplaceAll(oldStr, newStr);
        return res;
    }

    template<typename T>
    void TString<T>::ReplaceAll(const TString& oldStr, const TString& newStr)
    {
        while (true)
        {
            int fnd = Find(oldStr);
            if (fnd < 0)
                break;

            Replace(newStr, fnd, fnd + oldStr.Length());
        }
    }

    template<typename T>
    int TString<T>::Find(const TString& other, int startIdx /*= 0*/) const
    {
        return (int)std::basic_string<T>::find(other, startIdx);
    }

    template<typename T>
    int TString<T>::Find(T symbol, int startIdx /*= 0*/) const
    {
        return (int)std::basic_string<T>::find(symbol, startIdx);
    }

    template<typename T>
    bool TString<T>::Contains(const TString& other, int startIdx /*= 0*/) const
    {
        return Find(other, startIdx) != -1;
    }

    template<typename T>
    bool TString<T>::Contains(T symbol, int startIdx /*= 0*/) const
    {
        return Find(symbol, startIdx) != -1;
    }

    template<typename T>
    int TString<T>::CountOf(const TString& other, int startIdx /*= 0*/) const
    {
        int res = 0;
        int searchIdx = startIdx;
        int l1 = Length(), l2 = other.Length();
        do
        {
            int srch = Find(other, searchIdx);
            if (srch < 0)
                break;

            res++;
            searchIdx = srch + l2;
        } while (searchIdx < l1);

        return res;
    }

    template<typename T>
    int TString<T>::FindLast(const TString& other, int startIdx /*= -1*/) const
    {
        return (int)std::basic_string<T>::rfind(other, (size_t)startIdx);

    }

    template<typename T>
    bool TString<T>::EndsWith(const TString& other) const
    {
        return std::basic_string<T>::rfind(other) == Length() - other.Length();
    }

    template<typename T>
    bool TString<T>::StartsWith(const TString& other) const
    {
        return std::basic_string<T>::find(other) == 0;
    }

    template<typename T>
    TString<T> TString<T>::SubStr(int begin, int end /*= -1*/) const
    {
        return TString<T>(std::basic_string<T>::substr(begin, end - begin));
    }

    template<typename T>
    Vector<TString<T>> TString<T>::Split(const TString& splitStr) const
    {
        Vector<TString> res;

        if (IsEmpty())
            return res;

        int i = 0;
        int l1 = Length(), l2 = splitStr.Length();
        while (i < l1)
        {
            int f = Find(splitStr, i);
            if (f < 0)
            {
                res.Add(SubStr(i));
                break;
            }

            res.Add(SubStr(i, f));
            i = f + l2;
        }

        return res;
    }

    template<typename T>
    TString<T> TString<T>::ToLowerCase() const
    {
        TString<T> res(*this);
        std::transform(res.begin(), res.end(), res.begin(), ::tolower);
        return res;
    }

    template<typename T>
    TString<T> TString<T>::ToUpperCase() const
    {
        TString<T> res(*this);
        std::transform(res.begin(), res.end(), res.begin(), ::toupper);
        return res;
    }

    template<typename T>
    void TString<T>::Trim(const TString& trimSymbols /*= " "*/)
    {
        TrimStart(trimSymbols);
        TrimEnd(trimSymbols);
    }

    template<typename T>
    void TString<T>::TrimStart(const TString& trimSymbols /*= " "*/)
    {
        int trimEnd = -1;
        int l1 = Length(), l2 = trimSymbols.Length();
        for (int i = 0; i < l1; i++)
        {
            bool breaking = true;
            auto dataChar = (*this)[i];
            for (int j = 0; j < l2; j++)
            {
                if (dataChar == trimSymbols[j])
                {
                    breaking = false;
                    break;
                }
            }

            if (breaking)
                break;
            else
                trimEnd = i;
        }

        Erase(0, trimEnd + 1);
    }

    template<typename T>
    void TString<T>::TrimEnd(const TString& trimSymbols /*= " "*/)
    {
        int l1 = Length(), l2 = trimSymbols.Length();
        int trimEnd = l1;
        for (int i = l1 - 1; i >= 0; i--)
        {
            bool breaking = true;
            auto dataChar = (*this)[i];
            for (int j = 0; j < l2; j++)
            {
                if (dataChar == trimSymbols[j])
                {
                    breaking = false;
                    break;
                }
            }

            if (breaking)
                break;
            else
                trimEnd = i;
        }

        Erase(trimEnd, l1);
    }

    template<typename T>
    TString<T> TString<T>::TrimedEnd(const TString& trimSymbols /*= " "*/)
    {
        TString res(*this);
        res.TrimEnd(trimSymbols);
        return res;
    }

    template<typename T>
    TString<T> TString<T>::TrimedStart(const TString& trimSymbols /*= " "*/)
    {
        TString res(*this);
        res.TrimStart(trimSymbols);
        return res;
    }

    template<typename T>
    TString<T> TString<T>::Trimed(const TString& trimSymbols /*= " "*/)
    {
        TString res(*this);
        res.Trim(trimSymbols);
        return res;
    }

    template<typename T>
    TString<T> TString<T>::Format(const TString format, ...)
    {
        va_list vlist;
        va_start(vlist, format);

        TString res = Format(format, vlist);

        va_end(vlist);

        return res;
    }

    template<typename T>
    TString<T> TString<T>::Format(const TString format, va_list vlist)
    {
        const int maxSize = 2048;
        char buffer[maxSize];
        vsnprintf(buffer, maxSize, TString<char>(format).c_str(), vlist);

        return TString<T>(buffer);
	}

	template<typename T>
	template<typename T2>
	TString<T> TString<T>::ToString(const T2& value)
	{
		return TString<T>(value);
	}

    template<typename T>
    TString<T> TString<T>::Appended(const TString& other) const
    {
        TString res(*this);
        res.Append(other);
        return res;
    }

    template<typename T>
    TString<T> TString<T>::Inserted(const TString& other, int position /*= 0*/) const
    {
        TString res(*this);
        res.Insert(other, position);
        return res;
    }

    template<typename T>
    TString<T> TString<T>::Erased(int begin, int end /*= -1*/) const
    {
        TString res(*this);
        res.Erase(begin, end);
        return res;
    }

    template<typename T>
    T TString<T>::PopBack()
    {
        int l = Length();
        T res = (*this)[l - 1];
        (*this)[l] = '\0';
        return res;
    }

    template<typename T>
    T TString<T>::First() const
    {
        return (*this)[0];
    }

    template<typename T>
    T TString<T>::Last() const
    {
        int len = Length();
        if (len == 0)
            return T();

        return (*this)[len - 1];
    }
}
