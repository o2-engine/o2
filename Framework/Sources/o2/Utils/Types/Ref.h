#pragma once

#include <memory>
#include "Containers/Vector.h"
#include "o2/Utils/Memory/MemoryAnalyzeableObject.h"

namespace o2
{
    template<typename _type>
    class Ref;

    template<typename _type, typename ... _args>
    Ref<_type> Make(_args&& ... args);

    // --------------------------------
    // Reference counter implementation
    // --------------------------------
    struct RefCounter
    {
        UInt strongReferences = 0; // Strong references count
        UInt weakReferences = 0;   // Weak references count

#if ENABLE_MEMORY_ANALYZE
        std::vector<MemoryAnalyzeObject*> strongRefPointers; // Pointers to strong reference objects
#endif

    protected:
        template<typename _type>
        friend class Ref;

        template<typename _type>
        friend class WeakRef;

        template<typename _type, typename ... _args>
        friend Ref<_type> Make(_args&& ... args);

        template<typename _type, typename ... _args>
        friend Ref<_type> MakePlace(const char* location, int line, _args&& ... args);
    };

    // -------------------------------------------------------------
    // Base reference counterable object. 
    // Use it as base class for your objects, that can be referenced
    // -------------------------------------------------------------
    class RefCounterable
    {
    public:
        // Default constructor
        RefCounterable() {}

        // Constructor with reference counter
        RefCounterable(RefCounter* refCounter);

        // Virtual destructor, for inheritance
        virtual ~RefCounterable() {}

        // Returns strong references count
        UInt16 GetStrongReferencesCount() const;

        // Returns weak references count
        UInt16 GetWeakReferencesCount() const;

    protected:
        mutable RefCounter* mRefCounter = nullptr; // Pointer to reference counter

    protected:
        // Returns reference counter
        RefCounter* GetRefCounter() const;

        // Sets reference counter
        void SetRefCounter(RefCounter* refCounter);

        template<typename _type>
        friend class Ref;

        template<typename _type>
        friend class WeakRef;

        template<typename _type>
        friend RefCounter* GetRefCounterImpl(_type* ptr);

        friend struct RefMaker;

        template<typename _object_type>
        friend struct RefCountersSetter;
    };

    // ---------------------------------------------------------------------------------
    // Reference counterable interface. Used for multiple reference counters inheritance
    // ---------------------------------------------------------------------------------
    class IRefCounterable
    {
    protected:
        // Returns reference counter
        virtual RefCounter* GetRefCounter() const = 0;

        template<typename _type>
        friend class Ref;

        template<typename _type>
        friend class WeakRef;

        template<typename _type>
        friend RefCounter* GetRefCounterImpl(_type* ptr);

        template<typename _object_type>
        friend struct RefCountersSetter;
    };

    template<typename _object_ptr_type>
    struct RefCountersSetter
    {
        template<typename _type, typename ... _other_types>
        static void Set(_object_ptr_type object, RefCounter* refCounter)
        {
            static_cast<_type*>(object)->SetRefCounter(refCounter);

            if constexpr (sizeof...(_other_types) > 0)
                Set<_other_types...>(object, refCounter);
        }
    };

    // Reference counterable implementation to override GetRefCounter method
#define REF_COUNTERABLE_IMPL_TEMPLATE(THIS_CLASS, BASE_CLASS, ...)                                                                                \
        RefCounter* GetRefCounter() const { return BASE_CLASS::GetRefCounter(); }                                                                 \
        void SetRefCounter(RefCounter* refCounter) { RefCountersSetter<THIS_CLASS*>::template Set<BASE_CLASS, ##__VA_ARGS__>(this, refCounter); } \
        template<typename __type> friend RefCounter* o2::GetRefCounterImpl(__type* ptr);                                                          \
        template<typename _object_type> friend struct o2::RefCountersSetter;                                                                      \
        friend struct o2::RefMaker


    // Reference counterable implementation to override GetRefCounter method
#define REF_COUNTERABLE_IMPL(BASE_CLASS, ...) \
        REF_COUNTERABLE_IMPL_TEMPLATE(typename std::remove_pointer<decltype(this)>::type, BASE_CLASS, ##__VA_ARGS__)

#if ENABLE_MEMORY_ANALYZE
    class IObject;

#include <vector>
#include <algorithm>

#define OPTIONAL_BASE_REF : public MemoryAnalyzeObject
#define OPTIONAL_REFS_MANAGE_FORWARD(CLASS)          \
    o2::IObject* GetIObjectPtrFwd(CLASS* obj);       \
    const std::type_info& GetTypeInfoFwd(CLASS* obj)
     
#define OPTIONAL_REFS_MANAGE_FORWARD_IMPL(CLASS)                                      \
    o2::IObject* GetIObjectPtrFwd(CLASS* obj) { return dynamic_cast<IObject*>(obj); } \
    const std::type_info& GetTypeInfoFwd(CLASS* obj) { return typeid(*obj); }

#else
#define OPTIONAL_BASE_REF
#define OPTIONAL_REFS_MANAGE_FORWARD(CLASS)
#define OPTIONAL_REFS_MANAGE_FORWARD_IMPL(CLASS)
#endif

    // ----------------
    // Strong reference
    // ----------------
    template<typename _type>
    class Ref OPTIONAL_BASE_REF
    {
    public:
        // Default constructor
        Ref();

        // Constructor from pointer
        explicit Ref(_type* ptr);

        // Constructor from nullptr
        Ref(std::nullptr_t);

        // Copy constructor
        Ref(const Ref<_type>& other);

        // Move constructor
        Ref(Ref<_type>&& other);

        // Copy constructor from other type
        template<typename _other_type, typename _enable = std::enable_if<std::is_convertible<_other_type*, _type*>::value>::type>
        Ref(const Ref<_other_type>& other);

        // Copy constructor from other type
        template<typename _other_type, typename _enable = std::enable_if<std::is_convertible<_other_type*, _type*>::value>::type>
        Ref(Ref<_other_type>&& other);

        // Destructor
        ~Ref();

        // Equality operator
        bool operator==(const Ref<_type>& other) const;

        // Equality operator
        bool operator==(const _type* other) const;

        // Inequality operator
        bool operator!=(const Ref<_type>& other) const;

        // Inequality operator
        bool operator!=(const _type* other) const;

        // Copy operator from nullptr
        Ref<_type>& operator=(std::nullptr_t);

        // Copy operator
        Ref<_type>& operator=(const Ref<_type>& other);

        // Move operator
        Ref<_type>& operator=(Ref<_type>&& other);

        // Copy operator from other type
        template<typename _other_type, typename _enable = std::enable_if<std::is_convertible<_other_type*, _type*>::value>::type>
        Ref<_type>& operator=(const Ref<_other_type>& other);

        // Move operator from other type
        template<typename _other_type, typename _enable = std::enable_if<std::is_convertible<_other_type*, _type*>::value>::type>
        Ref<_type>& operator=(Ref<_other_type>&& other);

        // Less operator
        bool operator<(const Ref<_type>& other) const { return mPtr < other.mPtr; }

        // Returns true if reference is valid
        bool IsValid() const;

        // Returns pointer to object
        _type* Get() const;

        // Boolean cast operator, true means that reference is valid
        explicit operator bool() const;

        // Getter operator
        _type& operator*() const;

        // Pointer getter operator
        _type* operator->() const;

    protected:
        _type* mPtr = nullptr; // Pointer to object

#if ENABLE_MEMORY_ANALYZE
        std::byte* GetMemory() const override;
        IObject* GetIObject() const override;
        const std::type_info& GetTypeInfo() const override;

        void MoveReference(MemoryAnalyzeObject* other);

        friend class MemoryAnalyzer;
#endif 

    protected:
        void IncrementRef(); // Increments reference counter
        void DecrementRef(); // Decrements reference counter

        template<typename _other_type>
        friend class WeakRef;

        template<typename _other_type>
        friend class Ref;
    };

    // Returns reference counter
    template<typename _type>
    RefCounter* GetRefCounterImpl(_type* ptr)
    {
        return ptr->GetRefCounter();
    }

    // Destructs object
    template<typename _type>
    void DestructObjectImpl(_type* obj)
    {
        obj->~_type();
    }

    // Static check if type is complete
    template<typename T, typename = void>
    struct IsCompleteRef : std::false_type {};

    // Static check if type is complete
    template<typename T>
    struct IsCompleteRef<T, std::void_t<decltype(sizeof(T))>> : std::true_type {};

    // Returns reference counter
    template<typename _type>
    RefCounter* GetRefCounter(_type* ptr)
    {
        if constexpr (IsCompleteRef<_type>::value)
            return GetRefCounterImpl(ptr);
        else
            return GetRefCounterFwd(ptr);
    }

    // Destructs object
    template<typename _type>
    void DestructObject(_type* obj)
    {
        if constexpr (IsCompleteRef<_type>::value)
            return DestructObjectImpl(obj);
        else
            return DestructObjectFwd(obj);
    }

    // Forward declaration of class and reference counter implementation
#define FORWARD_REF(CLASS)                        \
    o2::RefCounter* GetRefCounterFwd(CLASS* ptr); \
    void DestructObjectFwd(CLASS* obj);           \
    OPTIONAL_REFS_MANAGE_FORWARD(CLASS)

    // Forward declaration of class and reference counter implementation
#define FORWARD_CLASS_REF(CLASS) \
    class CLASS;                 \
    FORWARD_REF(CLASS)

    // Implementation of forward declared reference counter 
#define FORWARD_REF_IMPL(CLASS)                                                     \
    o2::RefCounter* GetRefCounterFwd(CLASS* ptr) { return GetRefCounterImpl(ptr); } \
    void DestructObjectFwd(CLASS* obj) { DestructObjectImpl(obj); }                 \
    OPTIONAL_REFS_MANAGE_FORWARD_IMPL(CLASS)

    // Declares friend function for creating new object
#define FRIEND_REF_MAKE()  \
    friend struct RefMaker

    // Dynamic cast from one reference type to another
    template<typename _to_type, typename _from_type>
    Ref<_to_type> DynamicCast(const Ref<_from_type>& from)
    {
        return Ref<_to_type>(dynamic_cast<_to_type*>(const_cast<_from_type*>(from.Get())));
    }

    // Dynamic cast from one references vector type to another
    template<typename _to_type, typename _from_type>
    Vector<Ref<_to_type>> DynamicCastVector(const _from_type& from)
    {
        return from.template Convert<Ref<_to_type>>([](auto& ref) { return DynamicCast<_to_type>(ref); });
    }

    // Static cast from one reference type to another
    template<typename _to_type, typename _from_type>
    Ref<_to_type> StaticCast(const Ref<_from_type>& from)
    {
        return Ref<_to_type>(static_cast<_to_type*>(from.Get()));
    }

#if ENABLE_MEMORY_MANAGE
    // Special macro for making new object and returning reference to it with storing location and line of creation for debug
#define mmake o2::RefMaker(__FILE__, __LINE__).Make
#else
    // Special macro for making new object and returning reference to it with storing location and line of creation for debug
#define mmake o2::RefMaker().Make
#endif

    // ------------------------------
    // Helper struct for mmake macros
    // ------------------------------
    struct RefMaker
    {
        // Static check for presence of PostRefConstruct method
        template<class T, class = std::void_t<>>
        struct HasPostRefConstruct : std::false_type {};

        // Static check for presence of PostRefConstruct method
        template<class T>
        struct HasPostRefConstruct<T, std::void_t<decltype(&T::PostRefConstruct)>> : std::true_type {};

        // Static check for presence of RefConstruct method with specified arguments
        template<typename T, typename... Args>
        struct HasRefConstructArgs
        {
        private:
            template<typename C>
            static auto test(int) -> decltype(std::declval<C>().RefConstruct(std::declval<Args>()...), std::true_type{});

            template<typename>
            static auto test(...) -> std::false_type;

        public:
            static constexpr bool value = decltype(test<T>(0))::value;
        };

    public:
        RefMaker() = default;

#if ENABLE_MEMORY_MANAGE
        const char* location;
        int line;

        RefMaker(const char* location, int line) :location(location), line(line) {}

        void* allocate(std::size_t size) const
        {
            return _mmalloc(size, location, line);
        }
#else
        void* allocate(std::size_t size) const
        {
            return malloc(size);
        }
#endif
        // Makes new object and returns reference to it. Creates memory block with reference counter and object to keep them together.
        // Stores location and line of creation for debug
        template<typename _type, typename ... _args>
        Ref<_type> Make(_args&& ... args)
        {
            constexpr auto refSize = sizeof(RefCounter);
            constexpr auto typeSize = sizeof(_type);

            auto memory = (std::byte*)allocate(refSize + typeSize);
            auto refCounter = new (memory) RefCounter();
            refCounter->strongReferences += 1;

            _type* object;

            constexpr bool isConstructibleWithRefCounter = std::is_constructible<_type, RefCounter*, _args...>::value;
            if constexpr (isConstructibleWithRefCounter)
            {
                object = new (memory + refSize) _type(refCounter, std::forward<_args>(args)...);
            }
            else
            {
                object = new (memory + refSize) _type(std::forward<_args>(args)...);
                object->SetRefCounter(refCounter);
            }

            if constexpr (HasRefConstructArgs<_type, _args ...>::value)
                object->RefConstruct(std::forward<_args>(args)...);

            if constexpr (HasPostRefConstruct<_type>::value)
                object->PostRefConstruct();

            refCounter->strongReferences -= 1;

            return Ref<_type>(object);
        }
    };

    // BaseRef implementation

    template<typename _type>
    Ref<_type>::Ref()
    {}

    template<typename _type>
    Ref<_type>::Ref(_type* ptr) :
        mPtr(ptr)
    {
        IncrementRef();
    }

    template<typename _type>
    Ref<_type>::Ref(std::nullptr_t)
    {}

    template<typename _type>
    Ref<_type>::Ref(const Ref<_type>& other) :
        mPtr(other.mPtr)
    {
        IncrementRef();
    }

    template<typename _type>
    Ref<_type>::Ref(Ref<_type>&& other) :
        mPtr(other.mPtr)
    {
		other.mPtr = nullptr;

#if ENABLE_MEMORY_ANALYZE
		MoveReference(static_cast<MemoryAnalyzeObject*>(&other));
#endif
    }

    template<typename _type>
    template<typename _other_type, typename _enable>
    Ref<_type>::Ref(const Ref<_other_type>& other) :
        mPtr(other.mPtr)
    {
        IncrementRef();
    }

    template<typename _type>
    template<typename _other_type, typename _enable>
    Ref<_type>::Ref(Ref<_other_type>&& other) :
        mPtr(other.mPtr)
    {
        other.mPtr = nullptr;

#if ENABLE_MEMORY_ANALYZE
		MoveReference(static_cast<MemoryAnalyzeObject*>(&other));
#endif
    }

    template<typename _type>
    Ref<_type>::~Ref()
    {
        DecrementRef();
    }

    template<typename _type>
    bool Ref<_type>::operator==(const Ref<_type>& other) const
    {
        return mPtr == other.mPtr;
    }

    template<typename _type>
    bool Ref<_type>::operator==(const _type* other) const
    {
        return mPtr == other;
    }

    template<typename _type>
    bool Ref<_type>::operator!=(const Ref<_type>& other) const
    {
        return mPtr != other.mPtr;
    }

    template<typename _type>
    bool Ref<_type>::operator!=(const _type* other) const
    {
        return mPtr != other;
    }

    template<typename _type>
    Ref<_type>& Ref<_type>::operator=(std::nullptr_t)
    {
        DecrementRef();
        mPtr = nullptr;

        return *this;
    }

    template<typename _type>
    Ref<_type>& Ref<_type>::operator=(const Ref<_type>& other)
    {
        DecrementRef();

        mPtr = other.mPtr;

        IncrementRef();

        return *this;
    }

    template<typename _type>
    template<typename _other_type, typename _enable>
    Ref<_type>& Ref<_type>::operator=(const Ref<_other_type>& other)
    {
        DecrementRef();

        mPtr = other.mPtr;

        IncrementRef();

        return *this;
    }

    template<typename _type>
    template<typename _other_type, typename _enable>
    Ref<_type>& Ref<_type>::operator=(Ref<_other_type>&& other)
    {
        DecrementRef();

        mPtr = other.mPtr;
		other.mPtr = nullptr;

#if ENABLE_MEMORY_ANALYZE
		MoveReference(static_cast<MemoryAnalyzeObject*>(&other));
#endif

        return *this;
    }

    template<typename _type>
    Ref<_type>& Ref<_type>::operator=(Ref<_type>&& other)
    {
        DecrementRef();

        mPtr = other.mPtr;
		other.mPtr = nullptr;

#if ENABLE_MEMORY_ANALYZE
		MoveReference(static_cast<MemoryAnalyzeObject*>(&other));
#endif

        return *this;
    }

    template<typename _type>
    _type* Ref<_type>::Get() const
    {
        return mPtr;
    }

    template<typename _type>
    bool Ref<_type>::IsValid() const
    {
        return mPtr != nullptr;
    }

    template<typename _type>
    Ref<_type>::operator bool() const
    {
        return IsValid();
    }

    template<typename _type>
    _type& Ref<_type>::operator*() const
    {
        return *mPtr;
    }

    template<typename _type>
    _type* Ref<_type>::operator->() const
    {
        return mPtr;
    }

    template<typename _type>
    void Ref<_type>::IncrementRef()
    {
        if (mPtr)
        {
            auto refCounter = GetRefCounter(mPtr);
            refCounter->strongReferences++;
#if ENABLE_MEMORY_ANALYZE
            refCounter->strongRefPointers.push_back(static_cast<MemoryAnalyzeObject*>(this));
#endif
        }
    }

    template<typename _type>
    void Ref<_type>::DecrementRef()
    {
        if (mPtr)
        {
            auto refCounter = GetRefCounter(mPtr);

#if ENABLE_MEMORY_ANALYZE
            auto it = std::find(refCounter->strongRefPointers.begin(), refCounter->strongRefPointers.end(), 
                               static_cast<MemoryAnalyzeObject*>(this));
            if (it != refCounter->strongRefPointers.end())
                refCounter->strongRefPointers.erase(it);
#endif

            refCounter->strongReferences--;
            if (refCounter->strongReferences == 0)
            {
                refCounter->weakReferences++;

                DestructObject(mPtr);

                refCounter->weakReferences--;

                mPtr = nullptr;

                if (refCounter->weakReferences == 0)
                    _mfree(refCounter);
            }
        }
    }
}

#include "o2/Utils/Basic/IObject.h"

namespace o2
{
#if ENABLE_MEMORY_ANALYZE
    template<typename _type>
    std::byte* Ref<_type>::GetMemory() const
    {
        if (mPtr)
            return reinterpret_cast<std::byte*>(GetRefCounter(mPtr));

        return nullptr;
    }

    template<typename _type>
    IObject* GetIObjectPtrImpl(_type* ptr)
    {
        if constexpr (std::is_base_of<IObject, _type>::value)
            return dynamic_cast<IObject*>(ptr);

        return nullptr;
    }

    template<typename _type>
    IObject* GetIObjectPtr(_type* ptr)
    {
        if constexpr (IsCompleteRef<_type>::value)
            return GetIObjectPtrImpl(ptr);
        else
            return GetIObjectPtrFwd(ptr);
    }

    template<typename _type>
    IObject* Ref<_type>::GetIObject() const
    {
        return GetIObjectPtr(mPtr);
    }

    template<typename _type>
    const std::type_info& GetTypeInfoImpl(_type* ptr)
    {
        return typeid(*ptr);
    }

    template<typename _type>
    const std::type_info& GetTypeInfo(_type* ptr)
    {
        if constexpr (IsCompleteRef<_type>::value)
            return GetTypeInfoImpl(ptr);
        else
            return GetTypeInfoFwd(ptr);
    }

    template<typename _type>
    const std::type_info& Ref<_type>::GetTypeInfo() const 
    { 
        return o2::GetTypeInfo(mPtr);
	}

	template<typename _type>
	void Ref<_type>::MoveReference(MemoryAnalyzeObject* other)
	{
		if (mPtr)
		{
			auto refCounter = GetRefCounter(mPtr);
			auto& strongRefPointers = refCounter->strongRefPointers;
            
			auto it = std::find(strongRefPointers.begin(), strongRefPointers.end(), other);
			if (it != strongRefPointers.end())
				strongRefPointers.erase(it);

			strongRefPointers.push_back(static_cast<MemoryAnalyzeObject*>(this));
		}
	}
#endif
}

#include "o2/Utils/Types/WeakRef.h"
