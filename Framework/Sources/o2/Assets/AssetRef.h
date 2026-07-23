#pragma once

#include "o2/Utils/Types/Ref.h"
#include "o2/Utils/Types/StringDef.h"

namespace o2
{
    class Asset;

    // -----------------------------------------------------------------------
    // Basic asset reference interface. All Ref<AssetType> are derived from it
    // -----------------------------------------------------------------------
    class BaseAssetRef : public ISerializable
    {
    public:
        // Returns asset type
        virtual const Type& GetAssetType() const;

        // Returns asset raw pointer
        virtual const Asset* GetAssetBase() const { return nullptr; }

        // Returns asset raw pointer
        virtual Asset* GetAssetBase() { return nullptr; }

        // Sets asset
        virtual void SetAssetBase(Asset* asset) {}

        // Sets asset instance
        virtual void SetInstance(Asset* asset) {}

        // Creates own asset instance. If asset is empty creates empty instance, copies asset if else
        virtual void CreateInstance() {}

        // Removes own asset instance
        virtual void RemoveInstance() {}

        // Saves asset instance
        virtual void SaveInstance(const String& path) {}

        // Is asset instance owner
        virtual bool IsInstance() const { return false; }

        SERIALIZABLE(BaseAssetRef);
    };

    // -----------------------------------------------------------------------------------
    // Asset reference. Contains asset pointer. Can contain asset instance owned by itself
    // -----------------------------------------------------------------------------------
    template<typename _asset_type>
    class AssetRef: public BaseAssetRef
    {
    public:
        // Default constructor, no reference
        AssetRef();

        // Nullptr constructor
        AssetRef(std::nullptr_t);

        // Constructor from asset path
        explicit AssetRef(const String& path);

        // Constructor from asset id
        explicit AssetRef(const UID& id);

        // Constructor with asset pointer
        explicit AssetRef(_asset_type* ptr);

        // Copy constructor from other base reference
        AssetRef(const BaseAssetRef& other);

        // Copy constructor from other reference
        AssetRef(const Ref<_asset_type>& other);

        // Move constructor from other reference
        AssetRef(Ref<_asset_type>&& other);

        // Copy constructor from other asset reference
        template<typename _other_type, typename _enable = std::enable_if<std::is_convertible<_other_type*, _asset_type*>::value>::type>
        AssetRef(const AssetRef<_other_type>& other);

        // Move constructor from other asset reference
        template<typename _other_type, typename _enable = std::enable_if<std::is_convertible<_other_type*, _asset_type*>::value>::type>
        AssetRef(AssetRef<_other_type>&& other);

        // Equality operator
        bool operator==(const AssetRef<_asset_type>& other) const;

        // Equality operator
        bool operator==(const _asset_type* other) const;

        // Inequality operator
        bool operator!=(const AssetRef<_asset_type>& other) const;

        // Inequality operator
        bool operator!=(const _asset_type* other) const;

        // Copy operator from other asset reference
        template<typename _other_type, typename _enable = std::enable_if<std::is_convertible<_other_type*, _asset_type*>::value>::type>
        AssetRef<_asset_type>& operator=(const AssetRef<_other_type>& other);

		// Copy operator from other asset reference
		template<typename _other_type, typename _enable = std::enable_if<std::is_convertible<_other_type*, _asset_type*>::value>::type>
		AssetRef<_asset_type>& operator=(const Ref<_other_type>& other);

        // Move operator from nullptr
        AssetRef<_asset_type>& operator=(std::nullptr_t);

        // Less operator
        bool operator<(const AssetRef<_asset_type>& other) const;

        // Returns is reference is valid
        bool IsValid() const;

        // Returns is reference is valid
        explicit operator bool() const;

        // Returns asset reference
        operator Ref<_asset_type>() const;

        // Returns asset reference
        _asset_type& operator*() const;

        // Returns asset pointer
        _asset_type* operator->() const;

        // Returns asset pointer 
        _asset_type* Get();

        // Returns asset pointer
        const _asset_type* Get() const;

        // Returns reference
        Ref<_asset_type>& GetRef();

        // Returns reference
        const Ref<_asset_type>& GetRef() const;

        // Returns asset type
        const Type& GetAssetType() const override;

        // Returns asset type
        static const Type* GetAssetTypeStatic();

        // Returns asset raw pointer
        const Asset* GetAssetBase() const override;

        // Returns asset raw pointer
        Asset* GetAssetBase() override;

        // Sets asset
        void SetAssetBase(Asset* asset) override;

        // Sets asset instance
        void SetInstance(Asset* asset) override;

        // Creates own asset instance. If asset is empty creates empty instance, copies asset if else
        void CreateInstance() override;

        // Removes own asset instance
        void RemoveInstance() override;

        // Saves asset instance
        void SaveInstance(const String& path) override;

        // Is asset instance owner
        bool IsInstance() const override;

        // Creates asset and returns reference
        template<typename ... _args>
        static AssetRef<_asset_type> CreateAsset(_args ... args);

    protected:
        bool             mIsInstance = false; // Is this reference owner of asset
        Ref<_asset_type> mPtr;                // Asset reference

    protected:
        // Beginning serialization callback - writes path and id
        void OnSerialize(DataValue& node) const override;

        // Completion deserialization callback -  reads path and id and searches asset
        void OnDeserialized(const DataValue& node) override;

        // Beginning serialization delta callback
        void OnSerializeDelta(DataValue& node, const IObject& origin) const override;

        // Completion deserialization delta callback
        void OnDeserializedDelta(const DataValue& node, const IObject& origin) override;

        // It is required to process asset reference as single object when searching deltas for prototypes
        static bool IsDeltaAsSingleObject();

    public:
        using _this_type = AssetRef<_asset_type>;

        SERIALIZABLE_MAIN(_this_type);
        IOBJECT_SCRIPTING(_this_type);

        template<typename _type_processor>
        static void ProcessBaseTypes(_this_type* object, _type_processor& processor)
        {
            typedef _this_type thisclass;
            processor.template StartBases<_this_type>(object, type);

            BASE_CLASS(BaseAssetRef);
        }

        template<typename _type_processor>
        static void ProcessFields(_this_type* object, _type_processor& processor)
        {
            typedef _this_type thisclass;
            processor.template StartFields<_this_type>(object, type);
        }

        template<typename _type_processor>
        static void ProcessMethods(_this_type* object, _type_processor& processor)
        {
            typedef _this_type thisclass;
            processor.template StartMethods<_this_type>(object, type);

            FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().CONSTRUCTOR(const String&);

            FUNCTION().PUBLIC().CONSTRUCTOR();
            FUNCTION().PUBLIC().CONSTRUCTOR(const Ref<_asset_type>&);
            FUNCTION().PUBLIC().CONSTRUCTOR(_asset_type*);
            FUNCTION().PUBLIC().CONSTRUCTOR(const UID&);
            FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(bool, IsValid);
            FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(_asset_type*, Get);
            FUNCTION().PUBLIC().SIGNATURE(const Type&, GetAssetType);
            FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetInstance, Asset*);
            FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, CreateInstance);
            FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, RemoveInstance);
            FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SaveInstance, const String&);
            FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(bool, IsInstance);

            FUNCTION().PUBLIC().SIGNATURE_STATIC(AssetRef<_asset_type>, CreateAsset);
            FUNCTION().PUBLIC().SIGNATURE_STATIC(const Type*, GetAssetTypeStatic);
        }

        template<typename _other_type>
        friend class AssetRef;
    };
}

#include "o2/Assets/Assets.h"

namespace o2
{

    template<typename _asset_type>
    AssetRef<_asset_type>::AssetRef()
    {}

    template<typename _asset_type>
    AssetRef<_asset_type>::AssetRef(std::nullptr_t) :
        mPtr(nullptr)
    {}

    template<typename _asset_type>
    AssetRef<_asset_type>::AssetRef(const String& path):
        mPtr(o2Assets.GetAssetRefByType<_asset_type>(path))
    {}

    template<typename _asset_type>
    AssetRef<_asset_type>::AssetRef(const UID& id):
        mPtr(o2Assets.GetAssetRefByType<_asset_type>(id))
    {}

    template<typename _asset_type>
    AssetRef<_asset_type>::AssetRef(_asset_type* ptr) :
        mPtr(ptr)
    {}

    template<typename _asset_type>
    AssetRef<_asset_type>::AssetRef(const BaseAssetRef& other)
    {
        mPtr = Ref(dynamic_cast<_asset_type*>(const_cast<Asset*>(other.GetAssetBase())));
        mIsInstance = other.IsInstance();
    }

    template<typename _asset_type>
    AssetRef<_asset_type>::AssetRef(const Ref<_asset_type>& other) :
        mPtr(other)
    {}

    template<typename _asset_type>
    AssetRef<_asset_type>::AssetRef(Ref<_asset_type>&& other) :
        mPtr(std::move(other))
    {}

    template<typename _asset_type>
    template<typename _other_type, typename _enable>
    AssetRef<_asset_type>::AssetRef(const AssetRef<_other_type>& other) :
        mPtr(other.mPtr), mIsInstance(other.mIsInstance)
    {}

    template<typename _asset_type>
    template<typename _other_type, typename>
    AssetRef<_asset_type>::AssetRef(AssetRef<_other_type>&& other) :
        mPtr(std::move(other.mPtr)), mIsInstance(other.mIsInstance)
    {}

    template<typename _asset_type>
    bool AssetRef<_asset_type>::operator==(const AssetRef<_asset_type>& other) const
    {
        return mPtr == other.mPtr && mIsInstance == other.mIsInstance;
    }

    template<typename _asset_type>
    bool AssetRef<_asset_type>::operator==(const _asset_type* other) const
    {
        return mPtr == other;
    }

    template<typename _asset_type>
    bool AssetRef<_asset_type>::operator!=(const AssetRef<_asset_type>& other) const
    {
        return mPtr != other.mPtr || mIsInstance != other.mIsInstance;
    }

    template<typename _asset_type>
    bool AssetRef<_asset_type>::operator!=(const _asset_type* other) const
    {
        return mPtr != other;
    }

    template<typename _asset_type>
    template<typename _other_type, typename _enable>
    AssetRef<_asset_type>& AssetRef<_asset_type>::operator=(const AssetRef<_other_type>& other)
    {
        mPtr = other.mPtr;
        mIsInstance = other.mIsInstance;
        return *this;
    }

	template<typename _asset_type>
	template<typename _other_type, typename _enable>
	AssetRef<_asset_type>& AssetRef<_asset_type>::operator=(const Ref<_other_type>& other)
	{
		mPtr = other;
		mIsInstance = false;
		return *this;
	}

    template<typename _asset_type>
    AssetRef<_asset_type>& AssetRef<_asset_type>::operator=(std::nullptr_t)
    {
        mPtr = nullptr;
        return *this;
    }

    template<typename _asset_type>
    bool AssetRef<_asset_type>::operator<(const AssetRef<_asset_type>& other) const
    {
        return mPtr < other.mPtr;
    }

    template<typename _asset_type>
    bool AssetRef<_asset_type>::IsValid() const
    {
        return mPtr.IsValid();
    }

    template<typename _asset_type>
    AssetRef<_asset_type>::operator bool() const
    {
        return IsValid();
    }

    template<typename _asset_type>
    AssetRef<_asset_type>::operator Ref<_asset_type>() const
    {
        return mPtr;
    }

    template<typename _asset_type>
    _asset_type& AssetRef<_asset_type>::operator*() const
    {
        return *mPtr;
    }

    template<typename _asset_type>
    _asset_type* AssetRef<_asset_type>::operator->() const
    {
        return mPtr.Get();
    }

    template<typename _asset_type>
    _asset_type* AssetRef<_asset_type>::Get()
    {
        return mPtr.Get();
    }

    template<typename _asset_type>
    const _asset_type* AssetRef<_asset_type>::Get() const
    {
        return mPtr.Get();
    }

    template<typename _asset_type>
    Ref<_asset_type>& AssetRef<_asset_type>::GetRef()
    {
        return mPtr;
    }

    template<typename _asset_type>
    const Ref<_asset_type>& AssetRef<_asset_type>::GetRef() const
    {
        return mPtr;
    }

    template<typename _asset_type>
    const Type& AssetRef<_asset_type>::GetAssetType() const
    {
        return TypeOf(_asset_type);
    }

    template<typename _asset_type>
    const Type* AssetRef<_asset_type>::GetAssetTypeStatic()
    {
        return &TypeOf(_asset_type);
    }

    template<typename _asset_type>
    const Asset* AssetRef<_asset_type>::GetAssetBase() const
    {
        return Get();
    }

    template<typename _asset_type>
    Asset* AssetRef<_asset_type>::GetAssetBase()
    {
        return Get();
    }

    template<typename _asset_type>
    void AssetRef<_asset_type>::SetAssetBase(Asset* asset)
    {
        mPtr = Ref(dynamic_cast<_asset_type*>(asset));
        mIsInstance = false;
    }

    template<typename _asset_type>
    void AssetRef<_asset_type>::SetInstance(Asset* asset)
    {
        mPtr = Ref(dynamic_cast<_asset_type*>(asset));
        mIsInstance = true;
    }

    template<typename _asset_type>
    void AssetRef<_asset_type>::CreateInstance()
    {
        Ref<_asset_type> asset;
        if (mPtr)
        {
            asset = DynamicCast<_asset_type>(mPtr->template CloneAsRef<Asset>());
        }
        else
        {
            auto objectType = dynamic_cast<const ObjectType*>(&GetAssetType());
            asset = DynamicCast<_asset_type>(objectType->CreateSampleRef());
        }

        SetInstance(asset.Get());
    }

    template<typename _asset_type>
    void AssetRef<_asset_type>::RemoveInstance()
    {
        if (!mIsInstance)
            return;

        mIsInstance = false;
        mPtr = nullptr;
    }

    template<typename _asset_type>
    void AssetRef<_asset_type>::SaveInstance(const String& path)
    {
        mPtr->SetPath(path);
        mPtr->Save();

		mIsInstance = false;
    }

    template<typename _asset_type>
    bool AssetRef<_asset_type>::IsInstance() const
    {
        return mIsInstance;
    }

    template<typename _asset_type>
    template<typename ... _args>
    AssetRef<_asset_type> AssetRef<_asset_type>::CreateAsset(_args ... args)
    {
        return DynamicCast<_asset_type>(o2Assets.CreateAsset<_asset_type>(args ...).GetRef());
    }

    template<typename _asset_type>
    void AssetRef<_asset_type>::OnSerialize(DataValue& node) const
    {
        if (mPtr)
        {
            if (mIsInstance)
            {
                mPtr->ID().Randomize(); // Randomize ID to avoid conflicts after loading same asset multiple times

                node["instance"] = mPtr;
                node["meta"] = mPtr->GetMeta();
            }
            else
            {
                node["id"] = mPtr->GetUID().ToString();
                node["path"] = mPtr->GetPath();
            }
        }
    }

    template<typename _asset_type>
    void AssetRef<_asset_type>::OnDeserialized(const DataValue& node)
    {
        mPtr = nullptr;
        mIsInstance = false;

        if (auto instanceNode = node.FindMember("instance"))
        {
            mIsInstance = true;
            instanceNode->Get(mPtr);

            UID oldUid = mPtr->GetUID();
            node.GetMember("meta").Get(mPtr->mInfo.meta);
            o2Assets.UpdateAssetCache(mPtr.Get(), "", oldUid);
        }
        else if (auto idNode = node.FindMember("id"))
        {
            mPtr = o2Assets.GetAssetRefByType<_asset_type>((UID)(*idNode));

            // The id may belong to another assets tree revision; recover through the path
            if (!mPtr)
            {
                if (auto pathNode = node.FindMember("path"))
                {
                    String path;
                    pathNode->Get(path);
                    mPtr = o2Assets.GetAssetRefByType<_asset_type>(path);

                    if (mPtr)
                    {
                        String staleId;
                        idNode->Get(staleId);
                        o2Debug.LogWarning("Asset reference resolved by path fallback: " + path +
                                           ", stale id " + staleId);
                    }
                }
            }
        }
        else if (auto pathNode = node.FindMember("path"))
        {
            String path;
            pathNode->Get(path);
            mPtr = o2Assets.GetAssetRefByType<_asset_type>(path);
        }
    }

    template<typename _asset_type>
    void AssetRef<_asset_type>::OnSerializeDelta(DataValue& node, const IObject& origin) const
    {
        OnSerialize(node);
    }

    template<typename _asset_type>
    void AssetRef<_asset_type>::OnDeserializedDelta(const DataValue& node, const IObject& origin)
    {
        OnDeserialized(node);
    }

    template<typename _asset_type>
    bool AssetRef<_asset_type>::IsDeltaAsSingleObject()
    {
        return true;
    }
}
// --- META ---

CLASS_BASES_META(o2::BaseAssetRef)
{
    BASE_CLASS(o2::ISerializable);
}
END_META;
CLASS_FIELDS_META(o2::BaseAssetRef)
{
}
END_META;
CLASS_METHODS_META(o2::BaseAssetRef)
{

    FUNCTION().PUBLIC().SIGNATURE(const Type&, GetAssetType);
    FUNCTION().PUBLIC().SIGNATURE(const Asset*, GetAssetBase);
    FUNCTION().PUBLIC().SIGNATURE(Asset*, GetAssetBase);
    FUNCTION().PUBLIC().SIGNATURE(void, SetAssetBase, Asset*);
    FUNCTION().PUBLIC().SIGNATURE(void, SetInstance, Asset*);
    FUNCTION().PUBLIC().SIGNATURE(void, CreateInstance);
    FUNCTION().PUBLIC().SIGNATURE(void, RemoveInstance);
    FUNCTION().PUBLIC().SIGNATURE(void, SaveInstance, const String&);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsInstance);
}
END_META;
// --- END META ---
