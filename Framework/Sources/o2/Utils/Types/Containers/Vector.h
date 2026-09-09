#pragma once

#include "o2/Utils/Debug/Assert.h"
#include "o2/Utils/Function/Function.h"
#include "o2/Utils/Math/Math.h"
#include "o2/Utils/Memory/MemoryAnalyzeableObject.h"
#include "o2/Utils/Memory/MemoryManager.h"
#include <algorithm>
#include <vector>

namespace o2
{
#if ENABLE_MEMORY_ANALYZE
#define OPTIONAL_BASE_VECTOR , public MemoryAnalyzeObject
#else
#define OPTIONAL_BASE_VECTOR
#endif

    // --------------------
    // Dynamic linear array
    // --------------------
    template<typename _type>
    class Vector : public std::vector<_type> OPTIONAL_BASE_VECTOR
    {
    public:
        typedef typename std::vector<_type>::iterator Iterator;
        typedef typename std::vector<_type>::const_iterator ConstIterator;
        typedef _type ElementType;

    public:
        // Constructor by initial capacity
        Vector();

        // Constructor from initializer list
        Vector(std::initializer_list<_type> init);

        // Copy-constructor
        Vector(const Vector& arr);

        // Move-constructor
        Vector(Vector&& arr);

        // Assign operator
        Vector& operator=(const Vector& arr);

        // Move operator
        Vector& operator=(Vector&& arr);

        // Plus operator - returns sum of this array and other elements
        Vector operator+(const Vector& arr) const;

        // Plus and assign operator - adding elements to this from other array
        Vector& operator+=(const Vector& arr);

        // Plus operator - returns vector with this elements and one more
        Vector operator+(const _type& value) const;

        // Plus and assign operator - adds element to this
        Vector& operator+=(const _type& value);

        // Minus operator - returns new vector, where removed elements
        Vector operator-(const Vector& arr) const;

        // Minus and assign operator - removes elements from array
        Vector& operator-=(const Vector& arr);

        // Minus operator - returns new vector without specified element
        Vector operator-(const _type& value) const;

        // Minus and assign operator - removes element from array
        Vector& operator-=(const _type& value);

        // Equal operator
        bool operator==(const Vector& arr) const;

        // Not equal operator
        bool operator!=(const Vector& arr) const;

        // Returns a copy of this
        Vector<_type>* Clone() const;

        // Returns data pointer
        _type* Data();

        // Returns data pointer
        const _type* Data() const;

        // Returns count of elements in vector
        int Count() const;

        // Returns count of elements in array by lambda
        int Count(const Function<bool(const _type&)>& match) const;

        // Returns true if array is empty
        bool IsEmpty() const;

        // Returns capacity of vector
        int Capacity() const;

        // Changes count of elements in array. If new size less than array size elements will be removed
        // Otherwise empty elements will be added at end
        void Resize(int newCount);

        // Changes capacity of vector. New capacity can't be less than current
        void Reserve(int newCapacity);

        // Reduces memory usage by freeing unused memory
        void ShrinkToFit();

        // Returns value at index
        const _type& Get(int idx) const;

        // Returns value at index
        _type& Get(int idx);

        // Sets value at index
        void Set(int idx, const _type& value);

        // Adds new element
        _type& Add(const _type& value);

        // Adds elements from other array
        void Add(const Vector<_type>& arr);

        // Inserts new value at position
        _type& Insert(const _type& value, int position);

        // Inserts new values from other array at position
        void Insert(const Vector<_type>& arr, int position);

        // Returns index of equal element. Returns -1 when array haven't equal element
        int IndexOf(const _type& value) const;

        // Returns index of first element that pass function
        int IndexOf(const Function<bool(const _type&)>& match) const;

        // Returns true, if array contains the element
        bool Contains(const _type& value) const;

        // Removes element from back and returns him
        _type PopBack();

        // Removes equal array element
        void Remove(const _type& value);

        // Removes equal array elements from other array
        void Remove(const Vector<_type>& arr);

        // Removes element by iterator
        Iterator Remove(const Iterator& it);

        // Removes elements by iterators range
        Iterator Remove(const Iterator& first, const Iterator& last);

        // Removes element at position
        void RemoveAt(int idx);

        // Removes elements in range
        void RemoveRange(int first, int last);

        // Removes matched array element
        void RemoveFirst(const Function<bool(const _type&)>& match);

        // Removes all elements that pass function
        void RemoveAll(const Function<bool(const _type&)>& match);

        // Removes all elements
        void Clear();

        // Returns true, if array contains a element that pass function
        bool Contains(const Function<bool(const _type&)>& match) const;

        // Returns elements of array that pass function
        const _type* Find(const Function<bool(const _type&)>& match) const;

        // Returns elements of array that pass function
        _type* Find(const Function<bool(const _type&)>& match);

        // Returns elements of array that pass function or default value
        _type FindOrDefault(const Function<bool(const _type&)>& match) const;

        // Sorts elements in array by sorting value, that gets from function
        template<typename _sort_type>
        void SortBy(const Function<_sort_type(const _type&)>& selector);

        // Returns first element
        _type& First();

        // Returns first element
        const _type& First() const;

        // Returns first element that pass function
        const _type* First(const Function<bool(const _type&)>& match) const;

        // Returns first element that pass function
        _type* First(const Function<bool(const _type&)>& match);

        // Returns last element
        _type& Last();

        // Returns constant last element
        const _type& Last() const;

        // Returns last element that pass function
        const _type* Last(const Function<bool(const _type&)>& match) const;

        // Returns last element that pass function
        _type* Last(const Function<bool(const _type&)>& match);

        // Returns index of last element that pass function
        int LastIndexOf(const Function<bool(const _type&)>& match) const;

        // Returns element by minimal result of function
        template<typename _sel_type>
        _type Min(const Function<_sel_type(const _type&)>& selector) const;

        // Returns element index by minimal result of function
        template<typename _sel_type>
        int MinIdx(const Function<_sel_type(const _type&)>& selector) const;

        // Returns element by maximal result of function
        template<typename _sel_type>
        _type Max(const Function<_sel_type(const _type&)>& selector) const;

        // Returns element index by maximal result of function
        template<typename _sel_type>
        int MaxIdx(const Function<_sel_type(const _type&)>& selector) const;

        // Returns all elements that pass function
        bool All(const Function<bool(const _type&)>& match) const;

        // Returns true if any of elements pass function
        bool Any(const Function<bool(const _type&)>& match) const;

        // Returns sum of function results for all elements
        template<typename _sel_type>
        _sel_type Sum(const Function<_sel_type(const _type&)>& selector) const;

        // Invokes function for all elements in array
        void ForEach(const Function<void(_type&)>& func);

        // Invokes function for all elements in array
        void ForEach(const Function<void(const _type&)>& func) const;

        // Reversing array
        void Reverse();

        // Sorts elements in array by predicate
        void Sort(const Function<bool(const _type&, const _type&)>& pred = Math::Fewer);

        // Returns copy with sorts elements in array by predicate
        Vector Sorted(const Function<bool(const _type&, const _type&)>& pred = Math::Fewer);

        // Return vector of elements which pass function
        Vector FindAll(const Function<bool(const _type&)>& match) const;

        // Return vector of elements which pass function
        Vector Where(const Function<bool(const _type&)>& match) const;

        // Return vector of function results of all elements
        template<typename _sel_type>
        Vector<_sel_type> Convert(const Function<_sel_type(const _type&)>& selector) const;

        // Return vector with casted type
        template<typename _sel_type>
        Vector<_sel_type> Cast() const;

        // Return vector with dynamic casted type
        template<typename _sel_type>
        Vector<_sel_type> DynamicCast() const;

        // Returns first specified count elements
        Vector Take(int count) const;

        // Returns array from begin to end
        Vector Take(int begin, int end) const;

        // Returns begin iterator
        Iterator Begin();

        // Returns end iterator
        Iterator End();

        // Returns constant begin iterator
        ConstIterator Begin() const;

        // Returns constant end iterator
        ConstIterator End() const;

#if ENABLE_MEMORY_ANALYZE
        std::byte* GetMemory() const override { return const_cast<std::byte*>(reinterpret_cast<const std::byte*>(std::vector<_type>::data())); }
        size_t GetMemorySize() const { return Count() * sizeof(_type) + sizeof(*this); }
        //void IterateChildren(const std::function<void(const String&, MemoryAnalyzeObject*)>& callback) {}
        const std::type_info& GetTypeInfo() const override { return typeid(Vector<_type>); }
#endif
    };

    template<typename _type>
    Vector<_type>::Vector() :
        std::vector<_type>()
    {}

    template<typename _type>
    Vector<_type>::Vector(std::initializer_list<_type> init) :
        std::vector<_type>(init)
    {}

    template<typename _type>
    Vector<_type>::Vector(const Vector& arr) :
        std::vector<_type>((const std::vector<_type>&)arr)
    {}

    template<typename _type>
    Vector<_type>::Vector(Vector&& arr):
        std::vector<_type>((std::vector<_type>&&)arr)
    {}

    template<typename _type>
    _type* Vector<_type>::Data()
    {
        return std::vector<_type>::data();
    }

    template<typename _type>
    const _type* Vector<_type>::Data() const
    {
        return std::vector<_type>::data();
    }

    template<typename _type>
    Vector<_type>* Vector<_type>::Clone() const
    {
        return mnew Vector<_type>(this);
    }

    template<typename _type>
    Vector<_type>& Vector<_type>::operator=(const Vector<_type>& arr)
    {
        std::vector<_type>::operator=(arr);
        return *this;
    }

    template<typename _type>
    Vector<_type>& Vector<_type>::operator=(Vector&& arr)
    {
        std::vector<_type>::operator=(arr);
        return *this;
    }

    template<typename _type>
    Vector<_type> Vector<_type>::operator+(const Vector<_type>& arr) const
    {
        Vector<_type> res(*this);
        res.Add(arr);
        return res;
    }

    template<typename _type>
    Vector<_type>& Vector<_type>::operator+=(const Vector<_type>& arr)
    {
        Add(arr);
        return *this;
    }

    template<typename _type>
    Vector<_type> Vector<_type>::operator+(const _type& value) const
    {
        Vector<_type> res(*this);
        res.Add(value);
        return res;
    }

    template<typename _type>
    Vector<_type>& Vector<_type>::operator+=(const _type& value)
    {
        Add(value);
        return *this;
    }


    template<typename _type>
    Vector<_type> Vector<_type>::operator-(const Vector<_type>& arr) const
    {
        Vector<_type> res(*this);
        res.Remove(arr);
        return res;
    }

    template<typename _type>
    Vector<_type>& Vector<_type>::operator-=(const Vector<_type>& arr)
    {
        Remove(arr);
        return *this;
    }

    template<typename _type>
    Vector<_type> Vector<_type>::operator-(const _type& value) const
    {
        Vector<_type> res(*this);
        res.Remove(value);
        return res;
    }

    template<typename _type>
    Vector<_type>& Vector<_type>::operator-=(const _type& value)
    {
        Remove(value);
        return *this;
    }

    template<typename _type>
    bool Vector<_type>::operator==(const Vector<_type>& arr) const
    {
        if (arr.size() != std::vector<_type>::size())
            return false;

        for (unsigned int i = 0; i < std::vector<_type>::size(); i++)
        {
            if (!((*this)[i] == arr[i]))
                return false;
        }

        return true;
    }

    template<typename _type>
    bool Vector<_type>::operator!=(const Vector<_type>& arr) const
    {
        return !(*this == arr);
    }

    template<typename _type>
    int Vector<_type>::Count() const
    {
        return (int)std::vector<_type>::size();
    }

    template<typename _type>
    int Vector<_type>::Capacity() const
    {
        return std::vector<_type>::capacity();
    }

    template<typename _type>
    void Vector<_type>::Resize(int newCount)
    {
        std::vector<_type>::resize(newCount);
    }

    template<typename _type>
    void Vector<_type>::Reserve(int newCapacity)
    {
        std::vector<_type>::reserve(newCapacity);
    }

    template<typename _type>
    void Vector<_type>::ShrinkToFit()
    {
        std::vector<_type>::shrink_to_fit();
    }

    template<typename _type>
    const _type& Vector<_type>::Get(int idx) const
    {
        return std::vector<_type>::at(idx);
    }

    template<typename _type>
    _type& Vector<_type>::Get(int idx)
    {
        return std::vector<_type>::at(idx);
    }

    template<typename _type>
    void Vector<_type>::Set(int idx, const _type& value)
    {
        (*this)[idx] = value;
    }

    template<typename _type>
    _type& Vector<_type>::Add(const _type& value)
    {
        std::vector<_type>::push_back(value);
        return (*this)[std::vector<_type>::size() - 1];
    }

    template<typename _type>
    void Vector<_type>::Add(const Vector<_type>& arr)
    {
        std::vector<_type>::insert(std::vector<_type>::end(), arr.begin(), arr.end());
    }

    template<typename _type>
    _type Vector<_type>::PopBack()
    {
        _type res = std::vector<_type>::back();
        std::vector<_type>::pop_back();
        return res;
    }

    template<typename _type>
    _type& Vector<_type>::Insert(const _type& value, int position)
    {
        std::vector<_type>::insert(std::vector<_type>::begin() + position, value);
        return std::vector<_type>::at(position);
    }

    template<typename _type>
    void Vector<_type>::Insert(const Vector<_type>& arr, int position)
    {
        std::vector<_type>::insert(std::vector<_type>::begin() + position, arr.begin(), arr.end());
    }

    template<typename _type>
    int Vector<_type>::IndexOf(const _type& value) const
    {
        auto fnd = std::find(std::vector<_type>::begin(), std::vector<_type>::end(), value);
        if (fnd == std::vector<_type>::end())
            return -1;

        return (int)(fnd - std::vector<_type>::begin());
    }

    template<typename _type>
    bool Vector<_type>::Contains(const _type& value) const
    {
        return std::find(std::vector<_type>::begin(), std::vector<_type>::end(), value) != std::vector<_type>::end();
    }

    template<typename _type>
    void Vector<_type>::RemoveAt(int idx)
    {
        std::vector<_type>::erase(std::vector<_type>::begin() + idx);
    }

    template<typename _type>
    void Vector<_type>::RemoveRange(int first, int last)
    {
        std::vector<_type>::erase(std::vector<_type>::begin() + first, std::vector<_type>::begin() + last);
    }

    template<typename _type>
    void Vector<_type>::Remove(const _type& value)
    {
        auto fnd = std::find(std::vector<_type>::begin(), std::vector<_type>::end(), value);
        if (fnd != std::vector<_type>::end())
            std::vector<_type>::erase(fnd);
    }

    template<typename _type>
    void Vector<_type>::Remove(const Vector<_type>& arr)
    {
        for (auto it = arr.begin(); it != arr.end(); ++it)
            Remove(*it);
    }

    template<typename _type>
    typename Vector<_type>::Iterator Vector<_type>::Remove(const Iterator& first, const Iterator& last)
    {
        return std::vector<_type>::erase(first, last);
    }

    template<typename _type>
    typename Vector<_type>::Iterator Vector<_type>::Remove(const Iterator& it)
    {
        return std::vector<_type>::erase(it);
    }

    template<typename _type>
    void Vector<_type>::RemoveFirst(const Function<bool(const _type&)>& match)
    {
        for (auto it = std::vector<_type>::begin(); it != std::vector<_type>::end(); ++it)
        {
            if (match(*it))
            {
                std::vector<_type>::erase(it);
                return;
            }
        }
    }

    template<typename _type>
    void Vector<_type>::Clear()
    {
        std::vector<_type>::clear();
    }

    template<typename _type>
    bool Vector<_type>::IsEmpty() const
    {
        return std::vector<_type>::empty();
    }

    template<typename _type>
    _type& Vector<_type>::First()
    {
        return std::vector<_type>::front();
    }

    template<typename _type>
    const _type& Vector<_type>::First() const
    {
        return std::vector<_type>::front();
    }

    template<typename _type>
    const _type& Vector<_type>::Last() const
    {
        return std::vector<_type>::back();
    }

    template<typename _type>
    _type& Vector<_type>::Last()
    {
        return std::vector<_type>::back();
    }

    template<typename _type>
    void Vector<_type>::Sort(const Function<bool(const _type&, const _type&)>& pred /*= Math::Fewer*/)
    {
        std::sort(std::vector<_type>::begin(), std::vector<_type>::end(), pred);
    }

    template<typename _type>
    Vector<_type> Vector<_type>::Sorted(const Function<bool(const _type&, const _type&)>& pred /*= Math::Fewer*/)
    {
        Vector<_type> copy = *this;
        copy.Sort(pred);
        return copy;
    }

    template<typename _type>
    typename Vector<_type>::Iterator Vector<_type>::Begin()
    {
        return std::vector<_type>::begin();
    }

    template<typename _type>
    typename Vector<_type>::Iterator Vector<_type>::End()
    {
        return std::vector<_type>::end();
    }

    template<typename _type>
    typename Vector<_type>::ConstIterator Vector<_type>::Begin() const
    {
        return std::vector<_type>::cbegin();
    }

    template<typename _type>
    typename Vector<_type>::ConstIterator Vector<_type>::End() const
    {
        return std::vector<_type>::cend();
    }

    template<typename _type>
    Vector<_type> Vector<_type>::FindAll(const Function<bool(const _type&)>& match) const
    {
        Vector<_type> res;
        for (auto& element : *this)
        {
            if (match(element))
                res.Add(element);
        }

        return res;
    }

    template<typename _type>
    Vector<_type> Vector<_type>::Where(const Function<bool(const _type&)>& match) const
    {
        Vector<_type> res;
        for (auto& element : *this)
        {
            if (match(element))
                res.Add(element);
        }

        return res;
    }

    template<typename _type>
    template<typename _sel_type>
    Vector<_sel_type> Vector<_type>::Convert(const Function<_sel_type(const _type&)>& selector) const
    {
        Vector<_sel_type> res;
        for (auto& element : *this)
            res.Add(selector(element));

        return res;
    }

    template<typename _type>
    template<typename _sel_type>
    Vector<_sel_type> Vector<_type>::Cast() const
    {
        Vector<_sel_type> res;
        for (auto& element : *this)
            res.Add((_sel_type)element);

        return res;
    }

    template<typename _type>
    template<typename _sel_type>
    Vector<_sel_type> Vector<_type>::DynamicCast() const
    {
        Vector<_sel_type> res;
        for (auto& element : *this)
            res.Add(dynamic_cast<_sel_type>(element));

        return res;
    }

    template<typename _type>
    Vector<_type> Vector<_type>::Take(int count) const
    {
        Vector<_type> res;
        int i = 0;
        for (auto& element : *this)
        {
            res.Add(element);
            i++;
            if (i == count)
                break;
        }

        return res;
    }

    template<typename _type>
    Vector<_type> Vector<_type>::Take(int begin, int end) const
    {
        Vector<_type> res;
        for (int i = begin; i < end && i < (int)std::vector<_type>::size(); i++)
            res.Add(Get(i));

        return res;
    }

    template<typename _type>
    int Vector<_type>::Count(const Function<bool(const _type&)>& match) const
    {
        int res = 0;
        int count = Count();
        for (int i = 0; i < count; i++)
        {
            if (match(Get(i)))
                res++;
        }

        return res;
    }

    template<typename _type>
    void Vector<_type>::RemoveAll(const Function<bool(const _type&)>& match)
    {
        for (auto it = std::vector<_type>::begin(); it != std::vector<_type>::end();)
        {
            if (match(*it))
                it = std::vector<_type>::erase(it);
            else
                ++it;
        }
    }

    template<typename _type>
    bool Vector<_type>::Contains(const Function<bool(const _type&)>& match) const
    {
        for (auto& element : *this)
        {
            if (match(element))
                return true;
        }

        return false;
    }

    template<typename _type>
    const _type* Vector<_type>::Find(const Function<bool(const _type&)>& match) const
    {
        for (auto& element : *this)
        {
            if (match(element))
                return &element;
        }

        return nullptr;
    }

    template<typename _type>
    _type* Vector<_type>::Find(const Function<bool(const _type&)>& match)
    {
        for (auto& element : *this)
        {
            if (match(element))
                return &element;
        }

        return nullptr;
    }

    template<typename _type>
    _type Vector<_type>::FindOrDefault(const Function<bool(const _type&)>& match) const
    {
        auto fnd = Find(match);
        if (!fnd)
            return _type();

        return *fnd;
    }

    template<typename _type>
    int Vector<_type>::IndexOf(const Function<bool(const _type&)>& match) const
    {
        int count = Count();
        for (int i = 0; i < count; i++)
        {
            if (match(Get(i)))
                return i;
        }

        return -1;
    }

    template<typename _type>
    template<typename _sort_type>
    void Vector<_type>::SortBy(const Function<_sort_type(const _type&)>& selector)
    {
        Sort([&](const _type& l, const _type& r) { return selector(l) < selector(r); });
    }

    template<typename _type>
    const _type* Vector<_type>::First(const Function<bool(const _type&)>& match) const
    {
        return Find(match);
    }

    template<typename _type>
    _type* Vector<_type>::First(const Function<bool(const _type&)>& match)
    {
        return Find(match);
    }

    template<typename _type>
    const _type* Vector<_type>::Last(const Function<bool(const _type&)>& match) const
    {
        for (auto& element : *this)
        {
            if (match(element))
                return &element;
        }

        return nullptr;
    }

    template<typename _type>
    _type* Vector<_type>::Last(const Function<bool(const _type&)>& match)
    {
        for (auto& element : *this)
        {
            if (match(element))
                return &element;
        }

        return nullptr;
    }

    template<typename _type>
    int Vector<_type>::LastIndexOf(const Function<bool(const _type&)>& match) const
    {
        for (auto it = std::vector<_type>::rbegin(); it != std::vector<_type>::rend(); it--)
        {
            if (match(*it))
                return it - std::vector<_type>::begin();
        }

        return -1;
    }

    template<typename _type>
    template<typename _sel_type>
    _type Vector<_type>::Min(const Function<_sel_type(const _type&)>& selector) const
    {
        int count = Count();
        if (count == 0)
            return _type();

        const _type& res = Get(0);
        _sel_type minSel = selector(res);

        for (int i = 1; i < count; i++)
        {
            const _type& itVal = Get(i);
            _sel_type itSel = selector(itVal);

            if (itSel < minSel)
            {
                minSel = itSel;
                res = itVal;
            }
        }

        return res;
    }

    template<typename _type>
    template<typename _sel_type>
    int Vector<_type>::MinIdx(const Function<_sel_type(const _type&)>& selector) const
    {
        int count = Count();
        if (count == 0)
            return -1;

        int res = 0;
        _sel_type minSel = selector(Get(0));

        for (int i = 1; i < count; i++)
        {
            _sel_type itSel = selector(Get(i));

            if (itSel < minSel)
            {
                res = i;
                minSel = itSel;
            }
        }

        return res;
    }

    template<typename _type>
    template<typename _sel_type>
    _type Vector<_type>::Max(const Function<_sel_type(const _type&)>& selector) const
    {
        int count = Count();
        if (count == 0)
            return _type();

        const _type* res = &Get(0);
        _sel_type minSel = selector(*res);

        for (int i = 1; i < count; i++)
        {
            const _type* itVal = &Get(i);
            _sel_type itSel = selector(*itVal);

            if (itSel > minSel)
            {
                minSel = itSel;
                res = itVal;
            }
        }

        return *res;
    }

    template<typename _type>
    template<typename _sel_type>
    int Vector<_type>::MaxIdx(const Function<_sel_type(const _type&)>& selector) const
    {
        int count = Count();
        if (count == 0)
            return -1;

        int res = 0;
        _sel_type minSel = selector(Get(0));

        for (int i = 1; i < count; i++)
        {
            _sel_type itSel = selector(Get(i));

            if (itSel > minSel)
            {
                res = i;
                minSel = itSel;
            }
        }

        return res;
    }

    template<typename _type>
    bool Vector<_type>::All(const Function<bool(const _type&)>& match) const
    {
        for (auto& element : *this)
        {
            if (!match(element))
                return false;
        }

        return true;
    }

    template<typename _type>
    bool Vector<_type>::Any(const Function<bool(const _type&)>& match) const
    {
        for (auto& element : *this)
        {
            if (match(element))
                return true;
        }

        return false;
    }

    template<typename _type>
    template<typename _sel_type>
    _sel_type Vector<_type>::Sum(const Function<_sel_type(const _type&)>& selector) const
    {
        int count = Count();
        if (count == 0)
            return _sel_type();

        _sel_type res = selector(Get(0));
        for (int i = 1; i < count; i++)
            res = res + selector(Get(i));

        return res;
    }

    template<typename _type>
    void Vector<_type>::ForEach(const Function<void(_type&)>& func)
    {
        for (auto& element : *this)
            func(element);
    }

    template<typename _type>
    void Vector<_type>::ForEach(const Function<void(const _type&)>& func) const
    {
        for (auto& element : *this)
            func(element);
    }

    template<typename _type>
    void Vector<_type>::Reverse()
    {
        int c = Count();
        for (int i = 0; i < c/2; i++)
        {
            _type a = Get(i);
            _type b = Get(c - i - 1);

            Set(i, b);
            Set(c - i - 1, a);
        }
    }
}
