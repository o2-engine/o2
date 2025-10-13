#pragma once

#include "o2/Assets/AssetInfo.h"
#include "o2/Assets/Meta.h"
#include "o2/Utils/Basic/ICloneable.h"
#include "o2/Utils/Editor/Attributes/DontDeleteAttribute.h"
#include "o2/Utils/Editor/Attributes/EditorPropertyAttribute.h"
#include "o2/Utils/Editor/Attributes/ExpandedByDefaultAttribute.h"
#include "o2/Utils/Editor/Attributes/NoHeaderAttribute.h"
#include "o2/Utils/Property.h"
#include "o2/Utils/Serialization/Serializable.h"
#include "o2/Utils/Types/Ref.h"

namespace o2
{
    class Asset;

//     template<>
//     class AssetRef<Asset>;
}

#include "o2/Assets/AssetRef.h"


namespace o2
{
    // -------------------------------------------------------------------------------------------------
    // Basic asset interface. Contains copy of asset, without caching. For regular use assets references
    // -------------------------------------------------------------------------------------------------
    class Asset : public ISerializable, public RefCounterable, public ICloneableRef
    {
    public:
        typedef AssetMeta MetaType;

    public:
        PROPERTIES(Asset);
        PROPERTY(String, path, SetPath, GetPath); // Asset path property @EDITOR_IGNORE
        GETTER(String, fullPath, GetFullPath);    // Full asset path getter (from binary path)
        GETTER(UID, id, GetUID);                  // Asset id getter
        GETTER(Ref<AssetMeta>, meta, GetMeta);        // Asset meta information pointer getter

    public:
        // Default constructor
        Asset();
        
		// Default constructor with reference counter
		explicit Asset(RefCounter* refCounter);

        // Virtual destructor
        virtual ~Asset();

        // It is called after reference initialization at object construction, registers asset cache
        void PostRefConstruct();

        // Assign operator
        Asset& operator=(const Asset& asset);

        // Returns path @SCRIPTABLE
        const String& GetPath() const;

        // Sets path @SCRIPTABLE
        void SetPath(const String& path);

        // Returns full asset path (from binary path) @SCRIPTABLE
        String GetFullPath() const;

        // Returns full built asset path (from binary) @SCRIPTABLE
        String GetBuiltFullPath() const;

        // Returns id of asset
        const UID& GetUID() const;

        // Returns meta information pointer
        const Ref<AssetMeta>& GetMeta() const;

        // Returns asset info
        const AssetInfo& GetInfo() const;

        // Loads asset from path
        void Load(const String& path);

        // Loads asset by id
        void Load(const UID& id);

        // Reloads asset
        void Reload();

        // Saves asset by path
        void Save(const String& path);

        // Saves asset
        void Save();

        // Sets asset dirty
        void SetDirty(bool dirty = true);

        // Returns is asset dirty
        bool IsDirty() const;

        // Returns extensions string (something like "ext1 ext2 ent asf")
        static Vector<String> GetFileExtensions();

        // Returns editor icon
        static String GetEditorIcon() { return "ui/UI4_big_file_icon.png"; }

        // Returns editor sorting weight
        static int GetEditorSorting() { return 1; }

        // Is this asset type is available to create from editor's assets window
        static bool IsAvailableToCreateFromEditor() { return false; }

        // Is asset reference available to contain instance inside
        static bool IsReferenceCanOwnInstance() { return false; }

#if IS_EDITOR
        // Does the asset for the editor or not
        void SetEditorAsset(bool isEditor);

        // Returns is asset for editor or not
        bool IsEditorAsset() const;
#endif

        SERIALIZABLE(Asset);
        CLONEABLE_REF(Asset);

    protected:
        PROPERTY(Ref<AssetMeta>, mMeta, SetMeta, GetMeta); // @EDITOR_PROPERTY @DONT_DELETE @EXPANDED_BY_DEFAULT

        AssetInfo mInfo; // Asset info 

        bool mDirty = false; // Is asset was changed

    protected:
        // Constructor with meta, use it as default constructor
        Asset(const Ref<AssetMeta>& meta);

		// Constructor with meta and reference counter, use it as default constructor
		Asset(RefCounter* refCounter, const Ref<AssetMeta>& meta);

        // Copy-constructor
        Asset(const Asset& other);

		// Copy-constructor with reference counter
		Asset(RefCounter* refCounter, const Asset& other);

        // Returns meta full path (from binary path)
        String GetMetaFullPath() const;

        // Returns reference to id in meta
        UID& ID();

        // Returns assets log stream pointer
        const Ref<LogStream>& GetAssetsLogStream() const;

        // Meta setter, used for property
        void SetMeta(const Ref<AssetMeta>& meta);

        // Loads asset from path
        void Load(const AssetInfo& info);

        // Loads asset data, using DataValue and serialization
        virtual void LoadData(const String& path);

        // Saves asset data, using DataValue and serialization
        virtual void SaveData(const String& path) const;

        // It is called when UID has changed
        virtual void OnUIDChanged(const UID& oldUID);

        template<typename _asset_type>
        friend class AssetRef;

        friend class Assets;
        friend class AssetsBuilder;

        FRIEND_REF_MAKE();
    };

    // This macro defines asset type
#define ASSET_TYPE(THIS_TYPE, META_TYPE) \
SERIALIZABLE(THIS_TYPE);                 \
CLONEABLE_REF(THIS_TYPE);                \
typedef META_TYPE MetaType;                                         

    // ----------------------------
    // Base asset with default meta
    // ----------------------------
    template<typename T>
    class AssetWithDefaultMeta: public Asset
    {
    public:
        typedef DefaultAssetMeta<T> Meta;
        typedef Meta MetaType;

    public:
        PROPERTIES(AssetWithDefaultMeta<T>);
        GETTER(Ref<Meta>, meta, GetMeta);  // Meta information getter

    public:
		// Default constructor, creates asset with default meta type
        AssetWithDefaultMeta(): Asset(mmake<Meta>()) {}

		// Default constructor with reference counter, creates asset with default meta type
		explicit AssetWithDefaultMeta(RefCounter* refCounter) : Asset(refCounter, mmake<Meta>()) {}

		// Copy constructor, creates asset with default meta type
        AssetWithDefaultMeta(const AssetWithDefaultMeta<T>& other) : Asset(other) {}

		// Copy constructor with reference counter, creates asset with default meta type
		AssetWithDefaultMeta(RefCounter* refCounter, const AssetWithDefaultMeta<T>& other) : Asset(refCounter, other) {}

		// Returns meta information pointer
        Ref<Meta> GetMeta() const { return DynamicCast<Meta>(mInfo.meta); }

        SERIALIZABLE(AssetWithDefaultMeta<T>);
    };
}
// --- META ---

CLASS_BASES_META(o2::Asset)
{
    BASE_CLASS(o2::ISerializable);
    BASE_CLASS(o2::RefCounterable);
    BASE_CLASS(o2::ICloneableRef);
}
END_META;
CLASS_FIELDS_META(o2::Asset)
{
    FIELD().PUBLIC().EDITOR_IGNORE_ATTRIBUTE().NAME(path);
    FIELD().PUBLIC().NAME(fullPath);
    FIELD().PUBLIC().NAME(id);
    FIELD().PUBLIC().NAME(meta);
    FIELD().PUBLIC().DONT_DELETE_ATTRIBUTE().EDITOR_PROPERTY_ATTRIBUTE().EXPANDED_BY_DEFAULT_ATTRIBUTE().NAME(mMeta);
    FIELD().PROTECTED().NAME(mInfo);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mDirty);
}
END_META;
CLASS_METHODS_META(o2::Asset)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().SIGNATURE(void, PostRefConstruct);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(const String&, GetPath);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetPath, const String&);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(String, GetFullPath);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(String, GetBuiltFullPath);
    FUNCTION().PUBLIC().SIGNATURE(const UID&, GetUID);
    FUNCTION().PUBLIC().SIGNATURE(const Ref<AssetMeta>&, GetMeta);
    FUNCTION().PUBLIC().SIGNATURE(const AssetInfo&, GetInfo);
    FUNCTION().PUBLIC().SIGNATURE(void, Load, const String&);
    FUNCTION().PUBLIC().SIGNATURE(void, Load, const UID&);
    FUNCTION().PUBLIC().SIGNATURE(void, Reload);
    FUNCTION().PUBLIC().SIGNATURE(void, Save, const String&);
    FUNCTION().PUBLIC().SIGNATURE(void, Save);
    FUNCTION().PUBLIC().SIGNATURE(void, SetDirty, bool);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsDirty);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(Vector<String>, GetFileExtensions);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetEditorIcon);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(int, GetEditorSorting);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(bool, IsAvailableToCreateFromEditor);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(bool, IsReferenceCanOwnInstance);
#if  IS_EDITOR
    FUNCTION().PUBLIC().SIGNATURE(void, SetEditorAsset, bool);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsEditorAsset);
#endif
    FUNCTION().PROTECTED().CONSTRUCTOR(const Ref<AssetMeta>&);
    FUNCTION().PROTECTED().CONSTRUCTOR(RefCounter*, const Ref<AssetMeta>&);
    FUNCTION().PROTECTED().CONSTRUCTOR(const Asset&);
    FUNCTION().PROTECTED().CONSTRUCTOR(RefCounter*, const Asset&);
    FUNCTION().PROTECTED().SIGNATURE(String, GetMetaFullPath);
    FUNCTION().PROTECTED().SIGNATURE(UID&, ID);
    FUNCTION().PROTECTED().SIGNATURE(const Ref<LogStream>&, GetAssetsLogStream);
    FUNCTION().PROTECTED().SIGNATURE(void, SetMeta, const Ref<AssetMeta>&);
    FUNCTION().PROTECTED().SIGNATURE(void, Load, const AssetInfo&);
    FUNCTION().PROTECTED().SIGNATURE(void, LoadData, const String&);
    FUNCTION().PROTECTED().SIGNATURE(void, SaveData, const String&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnUIDChanged, const UID&);
}
END_META;

META_TEMPLATES(typename T)
CLASS_BASES_META(o2::AssetWithDefaultMeta<T>)
{
    BASE_CLASS(o2::Asset);
}
END_META;
META_TEMPLATES(typename T)
CLASS_FIELDS_META(o2::AssetWithDefaultMeta<T>)
{
    FIELD().PUBLIC().NAME(meta);
}
END_META;
META_TEMPLATES(typename T)
CLASS_METHODS_META(o2::AssetWithDefaultMeta<T>)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().CONSTRUCTOR(const AssetWithDefaultMeta<T>&);
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*, const AssetWithDefaultMeta<T>&);
    FUNCTION().PUBLIC().SIGNATURE(Ref<Meta>, GetMeta);
}
END_META;
// --- END META ---
