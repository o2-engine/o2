#pragma once

#include "o2/Assets/AssetsTree.h"
#include "o2/Render/Spine/SpineManager.h"
#include "o2/Utils/Property.h"
#include "o2/Utils/Serialization/Serializable.h"
#include "o2/Utils/Singleton.h"
#include "o2/Utils/Types/Containers/Vector.h"

// Assets system access macros
#define o2Assets o2::Assets::Instance()

namespace Editor
{
    class EditorApplication;
}

namespace o2
{
    class LogStream;
    class Render;
    struct AssetInfo;

    FORWARD_CLASS_REF(Asset);

    // ----------------
    // Assets utilities
    // ----------------
    class Assets : public Singleton<Assets>
    {
    public:
        PROPERTIES(Assets);
        GETTER(String, assetsPath, GetAssetsPath); // Assets path getter

    public:
        Function<void(const Vector<UID>&)> onAssetsRebuilt; // Assets rebuilding event

    public:
        // Default constructor
        Assets(RefCounter* refCounter);

        // Destructor
        ~Assets();

        // Returns assets path
        String GetAssetsPath() const;

        // Returns data path
        String GetBuiltAssetsPath() const;

        // Returns asset path by asset id
        const String& GetAssetPath(const UID& id) const;

        // Returns asset id by path
        const UID& GetAssetId(const String& path) const;

        // Returns asset info by id
        const AssetInfo& GetAssetInfo(const UID& id) const;

        // Returns asset info by path
        const AssetInfo& GetAssetInfo(const String& path) const;

        // Returns assets types and extensions dictionary. Key - extension, value - type of asset
        static const Map<String, const Type*> GetAssetsExtensionsTypes();

        // Returns standard asset type
        static const Type* GetStdAssetType();

        // Returns asset type for extension
        static const Type* GetAssetTypeByExtension(const String& extension);

        // Returns asset reference by path
        AssetRef<Asset> GetAssetRef(const String& path);

        // Returns asset reference by path
        template<typename _asset_type>
        AssetRef<_asset_type> GetAssetRefByType(const String& path);

        // Returns asset reference by id
        AssetRef<Asset> GetAssetRef(const UID& id);

        // Returns asset reference by id
        template<typename _asset_type>
        AssetRef<_asset_type> GetAssetRefByType(const UID& id);

        // Creates asset type _asset_type
        template<typename _asset_type, typename ... _args>
        AssetRef<Asset> CreateAsset(_args ... args);

        // Returns true if asset exist by path
        bool IsAssetExist(const String& path) const;

        // Returns true if asset exist by id
        bool IsAssetExist(const UID& id) const;

        // Removes asset
        bool RemoveAsset(const AssetRef<Asset>& asset);

        // Removes asset by path
        bool RemoveAsset(const String& path);

        // Removes asset by id
        bool RemoveAsset(const UID& id);

        // Copies asset
        bool CopyAsset(const AssetRef<Asset>& asset, const String& dest);

        // Copies asset by path
        bool CopyAsset(const String& path, const String& dest);

        // Copies asset by id
        bool CopyAsset(const UID& id, const String& dest);

        // Moves asset to new path
        bool MoveAsset(const AssetRef<Asset>& asset, const String& newPath);

        // Moves asset by path to new path
        bool MoveAsset(const String& path, const String& newPath);

        // Moves asset by id to new path
        bool MoveAsset(const UID& id, const String& newPath);

        // Moves assets to new path
        bool MoveAssets(const Vector<UID>& assets, const String& destPath);

        // Renames asset to new path
        bool RenameAsset(const AssetRef<Asset>& asset, const String& newName);

        // Renames asset by path to new path
        bool RenameAsset(const String& path, const String& newName);

        // Renames asset by id to new path
        bool RenameAsset(const UID& id, const String& newName);

        // Returns all assets trees
        const Vector<Ref<AssetsTree>>& GetAssetsTrees() const;

        // Returns main tree
        const AssetsTree& GetAssetsTree() const;

        // Checks assets with zero references and removes them
        void CheckAssetsUnload();

        // Makes unique asset name from first path variant
        String MakeUniqueAssetName(const String& path);

#if IS_EDITOR
        // Rebuilds assets
        void RebuildAssets(bool resetCache = false);

        // Refreshes cached assets' mInfo from tree by UID; called after tree reload
        void RefreshCachedAssetsInfo(const Ref<AssetsTree>& tree);
#endif

    protected:
        bool mReady = true; // Is assets manager ready to work
        
        Ref<AssetsTree>         mMainAssetsTree; // Main assets tree
        Vector<Ref<AssetsTree>> mAssetsTrees;    // Assets trees

        Ref<LogStream> mLog; // Log stream

        Vector<AssetRef<Asset>>      mCachedAssets;       // Current cached assets
        Map<String, AssetRef<Asset>> mCachedAssetsByPath; // Current cached assets by path
        Map<UID, AssetRef<Asset>>    mCachedAssetsByUID;  // Current cached assets by uid

        Ref<SpineManager> mSpineManager; // Spine manager

    protected:
        // Loads asset infos
        void LoadAssetsTree();

        // Returns asset cache by path
        AssetRef<Asset> FindAssetCache(const String& path) const;

        // Returns asset cache by id
        AssetRef<Asset> FindAssetCache(const UID& id) const;

        // Clears assets cache
        void ClearAssetsCache();

        // Adds asset to cache
        AssetRef<Asset> AddAssetCache(Asset* asset);

        // Removes asset from cache by UID and path
        void RemoveAssetCache(Asset* asset);

        // Updates asset cached path and id
        AssetRef<Asset> UpdateAssetCache(Asset* asset, const String& oldPath, const UID& oldUID);

        // Removes asset by info
        bool RemoveAsset(const AssetInfo& info);

        // Copies asset by info
        bool CopyAsset(const AssetInfo& info, const String& dest);

        // Moves asset to new path
        bool MoveAsset(const AssetInfo& info, const String& newPath);

        // Renames asset to new path
        bool RenameAsset(const AssetInfo& info, const String& newName);

#if IS_EDITOR
        // Reloads asset infos and returns list of changed assets
        Vector<UID> ReloadAssetsTree();
#endif

        template<typename _asset_type>
        friend class AssetRef;

        friend class Asset;
        friend class FolderAsset;
        friend class Render;
        friend class Editor::EditorApplication;
    };
}

#include "o2/Assets/AssetRef.h"

namespace o2
{
    template<typename _asset_type>
    AssetRef<_asset_type> Assets::GetAssetRefByType(const String& path)
    {
        return DynamicCast<_asset_type>(GetAssetRef(path).GetRef());
    }

    template<typename _asset_type>
    AssetRef<_asset_type> Assets::GetAssetRefByType(const UID& id)
    {
        return DynamicCast<_asset_type>(GetAssetRef(id).GetRef());
    }

    template<typename _asset_type, typename ... _args>
    AssetRef<Asset> Assets::CreateAsset(_args ... args)
    {
        auto newAset = AssetRef(mmake<_asset_type>(args ...));

        mCachedAssets.Add(newAset);
        mCachedAssetsByUID[newAset->GetUID()] = newAset;

        return newAset;
    }

}
