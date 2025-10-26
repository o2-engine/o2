#pragma once

#include <variant>

namespace o2
{
    // -----------------------------------------------------------------------
    // Type-safe union wrapper around std::variant with Boost-like interface
    // -----------------------------------------------------------------------
    template<typename... Types>
    class Variant : public std::variant<Types...>
    {
    public:
        using Base = std::variant<Types...>;
        
        using Base::Base;
        using Base::operator=;
        
        // Default constructor
        Variant() = default;
        
        // Copy constructor
        Variant(const Variant&) = default;

        // Move constructor
        Variant(Variant&&) = default;
        
        // Copy assignment operator
        Variant& operator=(const Variant&) = default;

        // Move assignment operator
        Variant& operator=(Variant&&) = default;
        
        // Constructor from value
        template<typename T>
        Variant(T&& value) : Base(std::forward<T>(value)) {}
        
        // Assignment from value
        template<typename T>
        Variant& operator=(T&& value)
        {
            Base::operator=(std::forward<T>(value));
            return *this;
        }
        
        // Returns index of currently held type
        int Which() const noexcept
        {
            return static_cast<int>(Base::index());
        }
        
        // Returns true if variant holds a value
        bool HasValue() const noexcept
        {
            return !Base::valueless_by_exception();
        }
        
        // Returns true if variant is valueless by exception
        bool ValuelessByException() const noexcept
        {
            return Base::valueless_by_exception();
        }
        
        // Returns true if variant holds specified type
        template<typename T>
        bool Holds() const noexcept
        {
            return std::holds_alternative<T>(*this);
        }
        
        // Returns reference to value of specified type. Throws if type mismatch
        template<typename T>
        T& Get()
        {
            return std::get<T>(*this);
        }
        
        // Returns const reference to value of specified type. Throws if type mismatch
        template<typename T>
        const T& Get() const
        {
            return std::get<T>(*this);
        }
        
        // Returns reference to value at specified index. Throws if index mismatch
        template<std::size_t Index>
        auto& Get()
        {
            return std::get<Index>(*this);
        }
        
        // Returns const reference to value at specified index. Throws if index mismatch
        template<std::size_t Index>
        const auto& Get() const
        {
            return std::get<Index>(*this);
        }
        
        // Returns pointer to value of specified type, or nullptr if type mismatch
        template<typename T>
        T* GetPointer() noexcept
        {
            return std::get_if<T>(this);
        }
        
        // Returns const pointer to value of specified type, or nullptr if type mismatch
        template<typename T>
        const T* GetPointer() const noexcept
        {
            return std::get_if<T>(this);
        }
        
        // Returns pointer to value at specified index, or nullptr if index mismatch
        template<std::size_t Index>
        auto* GetPointer() noexcept
        {
            return std::get_if<Index>(this);
        }
        
        // Returns const pointer to value at specified index, or nullptr if index mismatch
        template<std::size_t Index>
        const auto* GetPointer() const noexcept
        {
            return std::get_if<Index>(this);
        }
        
        // Applies visitor to currently held value
        template<typename Visitor>
        decltype(auto) Visit(Visitor&& visitor)
        {
            return std::visit(std::forward<Visitor>(visitor), *this);
        }
        
        // Applies visitor to currently held value (const version)
        template<typename Visitor>
        decltype(auto) Visit(Visitor&& visitor) const
        {
            return std::visit(std::forward<Visitor>(visitor), *this);
        }
        
        // Equal operator
        friend bool operator==(const Variant& lhs, const Variant& rhs)
        {
            return static_cast<const Base&>(lhs) == static_cast<const Base&>(rhs);
        }
        
        // Not equal operator
        friend bool operator!=(const Variant& lhs, const Variant& rhs)
        {
            return static_cast<const Base&>(lhs) != static_cast<const Base&>(rhs);
        }
        
        // Less operator
        friend bool operator<(const Variant& lhs, const Variant& rhs)
        {
            return static_cast<const Base&>(lhs) < static_cast<const Base&>(rhs);
        }
        
        // Less or equal operator
        friend bool operator<=(const Variant& lhs, const Variant& rhs)
        {
            return static_cast<const Base&>(lhs) <= static_cast<const Base&>(rhs);
        }
        
        // Greater operator
        friend bool operator>(const Variant& lhs, const Variant& rhs)
        {
            return static_cast<const Base&>(lhs) > static_cast<const Base&>(rhs);
        }
        
        // Greater or equal operator
        friend bool operator>=(const Variant& lhs, const Variant& rhs)
        {
            return static_cast<const Base&>(lhs) >= static_cast<const Base&>(rhs);
        }
    };
    
    // Returns reference to value of specified type from variant
    template<typename T, typename... Types>
    T& Get(Variant<Types...>& v)
    {
        return v.template Get<T>();
    }
    
    // Returns const reference to value of specified type from variant
    template<typename T, typename... Types>
    const T& Get(const Variant<Types...>& v)
    {
        return v.template Get<T>();
    }
    
    // Returns reference to value at specified index from variant
    template<std::size_t Index, typename... Types>
    auto& Get(Variant<Types...>& v)
    {
        return v.template Get<Index>();
    }
    
    // Returns const reference to value at specified index from variant
    template<std::size_t Index, typename... Types>
    const auto& Get(const Variant<Types...>& v)
    {
        return v.template Get<Index>();
    }
    
    // Returns pointer to value of specified type, or nullptr if type mismatch or v is nullptr
    template<typename T, typename... Types>
    T* GetIf(Variant<Types...>* v) noexcept
    {
        return v ? v->template GetPointer<T>() : nullptr;
    }
    
    // Returns const pointer to value of specified type, or nullptr if type mismatch or v is nullptr
    template<typename T, typename... Types>
    const T* GetIf(const Variant<Types...>* v) noexcept
    {
        return v ? v->template GetPointer<T>() : nullptr;
    }
    
    // Returns pointer to value at specified index, or nullptr if index mismatch or v is nullptr
    template<std::size_t Index, typename... Types>
    auto* GetIf(Variant<Types...>* v) noexcept
    {
        return v ? v->template GetPointer<Index>() : nullptr;
    }
    
    // Returns const pointer to value at specified index, or nullptr if index mismatch or v is nullptr
    template<std::size_t Index, typename... Types>
    const auto* GetIf(const Variant<Types...>* v) noexcept
    {
        return v ? v->template GetPointer<Index>() : nullptr;
    }
    
    // Returns true if variant holds specified type
    template<typename T, typename... Types>
    bool HoldsAlternative(const Variant<Types...>& v) noexcept
    {
        return v.template Holds<T>();
    }
    
    // Applies visitor to variants
    template<typename Visitor, typename... Variants>
    decltype(auto) Visit(Visitor&& visitor, Variants&&... variants)
    {
        return std::visit(std::forward<Visitor>(visitor), std::forward<Variants>(variants)...);
    }
}
