#include "o2/stdafx.h"
#include "AssetsTree.h"

#include "o2/Assets/Types/FolderAsset.h"
#include "o2/Utils/Debug/Debug.h"
#include "o2/Utils/Debug/Log/LogStream.h"
#include "o2/Utils/FileSystem/FileSystem.h"

namespace o2
{
    AssetsTree::AssetsTree() :
        log(nullptr)
    {}

    AssetsTree::~AssetsTree()
    {
        Clear();
    }

    void AssetsTree::Build(const String& path)
    {
        this->assetsPath = path;

        if (!o2FileSystem.IsFolderExist(path))
            o2FileSystem.FolderCreate(path);

        FolderInfo folderInfo = o2FileSystem.GetFolderInfo(path);
        folderInfo.ClampPathNames();

        LoadFolder(folderInfo, nullptr);

        auto thisRef = Ref(this);
        for (auto& asset : rootAssets)
            asset->SetTree(thisRef);
    }

    void AssetsTree::Build(const FolderInfo& folderInfo)
    {
        LoadFolder(folderInfo, nullptr);

        auto thisRef = Ref(this);
        for (auto& asset : rootAssets)
            asset->SetTree(thisRef);
    }

    void AssetsTree::Rebuild()
    {
        FolderInfo folderInfo = o2FileSystem.GetFolderInfo(assetsPath);
        folderInfo.ClampPathNames();
        LoadFolder(folderInfo, nullptr);

        auto thisRef = Ref(this);
        for (auto& asset : rootAssets)
            asset->SetTree(thisRef);
    }

    void AssetsTree::SortAssets()
    {
        allAssets.Sort([](const WeakRef<AssetInfo>& a, const WeakRef<AssetInfo>& b) { return a.Lock()->path.Length() < b.Lock()->path.Length(); });
    }

    void AssetsTree::SortAssetsInverse()
    {
        allAssets.Sort([](const WeakRef<AssetInfo>& a, const WeakRef<AssetInfo>& b) { return a.Lock()->path.Length() > b.Lock()->path.Length(); });
    }

    Ref<AssetInfo> AssetsTree::Find(const String& path) const
    {
        WeakRef<AssetInfo> res;
        allAssetsByPath.TryGetValue(path, res);
        return res.Lock();
    }

    Ref<AssetInfo> AssetsTree::Find(const UID& id) const
    {
        WeakRef<AssetInfo> res;
        allAssetsByUID.TryGetValue(id, res);
        return res.Lock();
    }

    Ref<AssetInfo> AssetsTree::AddAsset(Ref<AssetInfo> asset)
    {
        int delPos = asset->path.FindLast("/");
        if (delPos < 0 || delPos == asset->path.Length() - 1)
        {
            rootAssets.Add(asset);
            asset->SetTree(Ref(this));
        }
        else
        {
            String parentPath = asset->path.SubStr(0, delPos);
            WeakRef<AssetInfo> parent;
            allAssetsByPath.TryGetValue(parentPath, parent);

            if (!parent)
            {
                if (log)
                    log->Out("Failed to add built asset info: " + asset->path);
            }
            else 
                parent.Lock()->AddChild(asset);

            asset->SetTree(Ref(this));
        }

        return asset;
    }

    void AssetsTree::RemoveAsset(Ref<AssetInfo> asset)
    {
        if (allAssetsByPath[asset->path] == asset)
            allAssetsByPath.Remove(asset->path);

        if (asset->meta && allAssetsByUID[asset->meta->ID()] == asset)
            allAssetsByUID.Remove(asset->meta->ID());

        allAssets.Remove(asset);
        rootAssets.Remove(asset);

        if (asset->parent)
            asset->parent.Lock()->RemoveChild(asset);

        if (asset->meta->GetAssetType() == &TypeOf(FolderAsset))
        {
            // Removing a child detaches it from this same vector, so iterate a copy:
            // skipped children would stay in the maps as dangling weak refs
            auto childs = asset->mChildren;
            for (auto& ch : childs)
                RemoveAsset(ch);
        }
    }

    void AssetsTree::Clear()
    {
        rootAssets.Clear();
        allAssets.Clear();
        allAssetsByPath.Clear();
        allAssetsByUID.Clear();
    }

    void AssetsTree::LoadFolder(const FolderInfo& folder, Ref<AssetInfo> parentAsset)
    {
        for (auto& fileInfo : folder.files)
        {
            String extension = o2FileSystem.GetFileExtension(fileInfo.path);

            if (extension != "meta")
            {
                String assetFullPath = assetsPath + fileInfo.path;
                String metaFullPath = assetFullPath + ".meta";

                bool isExistMetaForAsset = o2FileSystem.IsFileExist(metaFullPath);
                if (!isExistMetaForAsset)
                    continue;

                LoadAssetNode(fileInfo.path, parentAsset, fileInfo.editDate);
            }
        }

        for (auto& subFolder : folder.folders)
        {
            Ref<AssetInfo> asset;

            String subFolderPath = subFolder.path;
            if (subFolderPath.EndsWith("/"))
                subFolderPath.Erase(subFolderPath.Length() - 1);

            String folderFullPath = assetsPath + subFolderPath;
            String metaFullPath = folderFullPath + ".meta";

            bool isExistMetaForFolder = o2FileSystem.IsFileExist(metaFullPath);
            if (!isExistMetaForFolder)
            {
                if (log)
                    log->Warning("Can't load asset info for " + subFolder.path + " - missing meta file");

                continue;
            }

            asset = LoadAssetNode(subFolderPath, parentAsset, TimeStamp());

            LoadFolder(subFolder, asset);
        }
    }

    Ref<AssetInfo> AssetsTree::LoadAssetNode(const String& path, Ref<AssetInfo> parent, const TimeStamp& time)
    {
        DataDocument metaData;
        if (!metaData.LoadFromFile(this->assetsPath + path + ".meta"))
        {
            o2Debug.LogError("Failed to load asset meta: " + path);
        }

        Ref<AssetMeta> mmeta;
        metaData.Get<Ref<AssetMeta>>(mmeta);

        Ref<AssetInfo> asset = mmake<AssetInfo>();

        asset->meta = mmeta;
        asset->path = path;
        asset->editTime = time;
        asset->SetParent(parent);

        if (!parent)
            rootAssets.Add(asset);

        return asset;
    }

    void AssetsTree::OnDeserialized(const DataValue& node)
    {
        Ref<AssetsTree> thisRef(this);
        for (auto& asset : rootAssets)
            asset->SetTree(thisRef);
    }

}
// --- META ---

DECLARE_CLASS(o2::AssetsTree, o2__AssetsTree);
// --- END META ---
