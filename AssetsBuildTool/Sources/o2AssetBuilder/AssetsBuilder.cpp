#include "o2/stdafx.h"
#include "AssetsBuilder.h"

#include "o2/Assets/Assets.h"
#include "o2/Assets/Types/AtlasAsset.h"
#include "o2/Assets/Types/DataAsset.h"
#include "o2/Assets/Types/FolderAsset.h"
#include "o2/Assets/Types/SceneAsset.h"
#include "o2/Utils/Debug/Debug.h"
#include "o2/Utils/Debug/Log/LogStream.h"
#include "o2/Utils/FileSystem/FileSystem.h"
#include "o2/Utils/System/Time/Timer.h"
#include "o2AssetBuilder/ImageCompressor.h"

namespace o2
{
    AssetsBuilder::AssetsBuilder()
    {
        srand((UInt)time(NULL));

        mLog = mmake<LogStream>("Assets builder");
        o2Debug.GetLog()->BindStream(mLog);

        InitializeConverters();
    }

    AssetsBuilder::~AssetsBuilder()
    {
    }

    const Vector<UID>& AssetsBuilder::BuildAssets(Platform platform, const String& assetsPath, const String& builtAssetsPath,
                                                  const String& dataAssetsTreePath, const String& compressorConfig, bool forcible /*= false*/)
    {
        mPlatform = platform;
        mSourceAssetsPath = assetsPath;
        mBuiltAssetsPath = builtAssetsPath;
        mBuiltAssetsTreePath = dataAssetsTreePath;

        ImageCompressor::LoadConfig(compressorConfig);

        mLog->Out("Started assets building from: " + mSourceAssetsPath + " to: " + mBuiltAssetsPath);

        Timer timer;

        if (forcible)
            RemoveBuiltAssets();

        FolderInfo folderInfo = o2FileSystem.GetFolderInfo(mSourceAssetsPath);
        folderInfo.ClampPathNames();

        ProcessMissingMetasCreation(folderInfo);

        mSourceAssetsTree = mmake<AssetsTree>();
        mSourceAssetsTree->assetsPath = assetsPath;
        mSourceAssetsTree->log = mLog;
        mSourceAssetsTree->Build(folderInfo);

        DataDocument builtAssetsTreeDoc;
        builtAssetsTreeDoc.LoadFromFile(mBuiltAssetsTreePath);

        mBuiltAssetsTree = mmake<AssetsTree>();
        mBuiltAssetsTree->Deserialize(builtAssetsTreeDoc);

        mBuiltAssetsTree->log = mLog;

        ProcessRemovedAssets();
        ProcessNewAssets();
        ProcessModifiedAssets();
        ConvertersPostProcess();

        if (!mModifiedAssets.IsEmpty())
        {
            mBuiltAssetsTree->assetsPath = mSourceAssetsPath;
            mBuiltAssetsTree->builtAssetsPath = mBuiltAssetsPath;
            o2FileSystem.WriteFile(mBuiltAssetsTreePath, mBuiltAssetsTree->SerializeToString());
        }

        mLog->Out("Assets builder: Completed. Source: " + mSourceAssetsPath + ", Target: " + mBuiltAssetsPath + ", Time: " + (String)timer.GetDeltaTime() + " seconds");

        return mModifiedAssets;
    }

    const String& AssetsBuilder::GetSourceAssetsPath() const
    {
        return mSourceAssetsPath;
    }

    const String& AssetsBuilder::GetBuiltAssetsPath() const
    {
        return mBuiltAssetsPath;
    }

    Platform AssetsBuilder::GetPlatform() const
    {
        return mPlatform;
    }

    void AssetsBuilder::InitializeConverters()
    {
        auto converterTypes = TypeOf(IAssetConverter).GetDerivedTypes();
        for (auto converterType : converterTypes)
        {
            IAssetConverter* converter = (IAssetConverter*)converterType->CreateSample();
            converter->SetAssetsBuilder(this);
            auto availableAssetTypes = converter->GetProcessingAssetsTypes();
            for (auto tp : availableAssetTypes)
                mAssetConverters.Add(tp, converter);
        }

        mStdAssetConverter.SetAssetsBuilder(this);
    }

    void AssetsBuilder::RemoveBuiltAssets()
    {
        o2FileSystem.FileDelete(mBuiltAssetsTreePath);
        o2FileSystem.FolderRemove(mBuiltAssetsPath);
        o2FileSystem.FolderCreate(mBuiltAssetsPath);
    }

    void AssetsBuilder::ProcessMissingMetasCreation(FolderInfo& folder)
    {
        for (auto fileInfo : folder.files)
        {
            if (fileInfo.path.EndsWith(".meta"))
            {
                String metaFullPath = mSourceAssetsPath + fileInfo.path;
                String assetForMeta = o2FileSystem.GetFileNameWithoutExtension(metaFullPath);
                bool isExistAssetForMeta = o2FileSystem.IsFileExist(assetForMeta) || o2FileSystem.IsFolderExist(assetForMeta);
                if (!isExistAssetForMeta)
                {
                    mLog->Warning("Missing asset for meta: " + fileInfo.path + " - removing meta");
                    o2FileSystem.FileDelete(metaFullPath);
                }
            }
            else
            {
                String assetFullPath = mSourceAssetsPath + fileInfo.path;
                String metaFullPath = assetFullPath + ".meta";
                bool isExistMetaForAsset = o2FileSystem.IsFileExist(metaFullPath);
                if (!isExistMetaForAsset)
                {
                    auto assetType = Assets::GetAssetTypeByExtension(o2FileSystem.GetFileExtension(fileInfo.path));
                    GenerateMeta(*assetType, metaFullPath);
                }
            }
        }

        for (auto subFolder : folder.folders)
        {
            String subFolderPath = subFolder.path;
            subFolderPath.Erase(subFolderPath.Length() - 1);

            String folderFullPath = mSourceAssetsPath + subFolderPath;

            String metaFullPath = folderFullPath + ".meta";
            bool isExistMetaForFolder = o2FileSystem.IsFileExist(metaFullPath);
            if (!isExistMetaForFolder)
            {
                auto& assetType = TypeOf(FolderAsset);
                GenerateMeta(assetType, metaFullPath);
            }

            ProcessMissingMetasCreation(subFolder);
        }
    }

    void AssetsBuilder::ProcessRemovedAssets()
    {
        const Type* folderTypeId = &TypeOf(FolderAsset);

        mBuiltAssetsTree->SortAssetsInverse();

        // in first pass processing files, in second - folders
        for (int pass = 0; pass < 2; pass++)
        {
            for (auto builtAssetInfoIt = mBuiltAssetsTree->allAssets.Begin(); builtAssetInfoIt != mBuiltAssetsTree->allAssets.End(); )
            {
                auto builtAssetInfo = (*builtAssetInfoIt).Lock();
                bool isFolder = builtAssetInfo->meta->GetAssetType() == folderTypeId;
                bool skip = pass == 0 ? isFolder : !isFolder;
                if (skip)
                {
                    ++builtAssetInfoIt;
                    continue;
                }

                auto fnd = mSourceAssetsTree->allAssetsByUID.find(builtAssetInfo->meta->ID());
                bool needRemove = fnd == mSourceAssetsTree->allAssetsByUID.end();

                if (!needRemove)
                {
                    ++builtAssetInfoIt;
                    continue;
                }

                GetAssetConverter(builtAssetInfo->meta->GetAssetType())->RemoveAsset(*builtAssetInfo);

                mModifiedAssets.Add(builtAssetInfo->meta->ID());

                mLog->OutStr("Removed asset: " + builtAssetInfo->path);

                mBuiltAssetsTree->allAssetsByUID.Remove(builtAssetInfo->meta->ID());
                mBuiltAssetsTree->allAssetsByPath.Remove(builtAssetInfo->path);

                builtAssetInfoIt = mBuiltAssetsTree->allAssets.Remove(builtAssetInfoIt);

                if (builtAssetInfo->parent)
                    builtAssetInfo->parent.Lock()->RemoveChild(builtAssetInfo);
                else
                    mBuiltAssetsTree->rootAssets.Remove(builtAssetInfo);
            }
        }
    }

    void AssetsBuilder::ProcessModifiedAssets()
    {
        const Type* folderType = &TypeOf(FolderAsset);

        mSourceAssetsTree->SortAssets();

        // in first pass processing folders, in second - files
        for (int pass = 0; pass < 2; pass++)
        {
            for (auto sourceAssetInfoWeak : mSourceAssetsTree->allAssets)
            {
                auto sourceAssetInfo = sourceAssetInfoWeak.Lock();

                if (!sourceAssetInfo->meta)
                    continue;

                bool isFolder = sourceAssetInfo->meta->GetAssetType() == folderType;
                bool skip = pass == 0 ? !isFolder : isFolder;
                if (skip)
                    continue;

                WeakRef<AssetInfo> builtAssetInfoWeak;
                if (mBuiltAssetsTree->allAssetsByUID.TryGetValue(sourceAssetInfo->meta->ID(), builtAssetInfoWeak))
                {
                    auto builtAssetInfo = builtAssetInfoWeak.Lock();

                    if (sourceAssetInfo->path == builtAssetInfo->path)
                    {
                        if (sourceAssetInfo->editTime != builtAssetInfo->editTime ||
                            !sourceAssetInfo->meta->IsEqual(builtAssetInfo->meta.Get()))
                        {
                            GetAssetConverter(sourceAssetInfo->meta->GetAssetType())->ConvertAsset(*sourceAssetInfo);

                            mModifiedAssets.Add(sourceAssetInfo->meta->ID());

                            builtAssetInfo->editTime = sourceAssetInfo->editTime;
                            builtAssetInfo->meta = sourceAssetInfo->meta->CloneAsRef<AssetMeta>();

                            mLog->Out("Modified asset: " + sourceAssetInfo->path);
                        }
                    }
                    else
                    {
                        if (sourceAssetInfo->editTime != builtAssetInfo->editTime ||
                            !sourceAssetInfo->meta->IsEqual(builtAssetInfo->meta.Get()))
                        {
                            GetAssetConverter(builtAssetInfo->meta->GetAssetType())->RemoveAsset(*builtAssetInfo);

                            mBuiltAssetsTree->RemoveAsset(builtAssetInfo);

                            mLog->Out("Modified and moved to " + sourceAssetInfo->path + " asset: " + builtAssetInfo->path);

                            builtAssetInfo->path = sourceAssetInfo->path;
                            builtAssetInfo->editTime = sourceAssetInfo->editTime;

                            builtAssetInfo->meta = sourceAssetInfo->meta->CloneAsRef<AssetMeta>();

                            GetAssetConverter(sourceAssetInfo->meta->GetAssetType())->ConvertAsset(*sourceAssetInfo);

                            mModifiedAssets.Add(sourceAssetInfo->meta->ID());
                            mBuiltAssetsTree->AddAsset(builtAssetInfo);
                        }
                        else
                        {
                            GetAssetConverter(sourceAssetInfo->meta->GetAssetType())->MoveAsset(*builtAssetInfo, *sourceAssetInfo);
                            mLog->Out("Moved asset from " + builtAssetInfo->path + " to " + sourceAssetInfo->path);

                            mBuiltAssetsTree->RemoveAsset(builtAssetInfo);

                            builtAssetInfo->path = sourceAssetInfo->path;
                            builtAssetInfo->editTime = sourceAssetInfo->editTime;

                            builtAssetInfo->meta = sourceAssetInfo->meta->CloneAsRef<AssetMeta>();

                            mModifiedAssets.Add(sourceAssetInfo->meta->ID());
                            mBuiltAssetsTree->AddAsset(builtAssetInfo);
                        }
                    }
                }
            }
        }
    }

    void AssetsBuilder::ProcessNewAssets()
    {
        const Type* folderType = &TypeOf(FolderAsset);

        mSourceAssetsTree->SortAssets();

        // in first pass processing folders, in second - files
        for (int pass = 0; pass < 2; pass++)
        {
            for (auto sourceAssetInfoIt = mSourceAssetsTree->allAssets.Begin(); sourceAssetInfoIt != mSourceAssetsTree->allAssets.End(); ++sourceAssetInfoIt)
            {
                auto sourceAssetInfo = (*sourceAssetInfoIt).Lock();

                if (!sourceAssetInfo->meta)
                    continue;

                bool isFolder = sourceAssetInfo->meta->GetAssetType() == folderType;
                bool skip = pass == 0 ? !isFolder : isFolder;

                if (skip)
                    continue;

                auto fnd = mBuiltAssetsTree->allAssetsByUID.find(sourceAssetInfo->meta->ID());
                bool isNew = fnd == mBuiltAssetsTree->allAssetsByUID.end();

                if (!isNew)
                    continue;

                GetAssetConverter(sourceAssetInfo->meta->GetAssetType())->ConvertAsset(*sourceAssetInfo);

                mModifiedAssets.Add(sourceAssetInfo->meta->ID());

                mLog->Out("New asset: " + sourceAssetInfo->path);

                Ref<AssetInfo> newBuiltAsset = mmake<AssetInfo>();
                newBuiltAsset->path = sourceAssetInfo->path;
                newBuiltAsset->editTime = sourceAssetInfo->editTime;
                newBuiltAsset->meta = sourceAssetInfo->meta->CloneAsRef<AssetMeta>();

                mBuiltAssetsTree->AddAsset(newBuiltAsset);
            }
        }
    }

    void AssetsBuilder::ConvertersPostProcess()
    {
        for (auto it = mAssetConverters.Begin(); it != mAssetConverters.End(); ++it)
            mModifiedAssets.Add(it->second->AssetsPostProcess());

        mModifiedAssets.Add(mStdAssetConverter.AssetsPostProcess());
    }

    void AssetsBuilder::GenerateMeta(const Type& assetType, const String& metaFullPath)
    {
        auto assetTypeSample = (Asset*)assetType.CreateSample();
        auto assetTypeSampleMeta = assetTypeSample->GetMeta();

        DataDocument metaData;
        metaData = assetTypeSampleMeta;
        metaData["Value"]["mId"] = UID();

        metaData.SaveToFile(metaFullPath);

        delete assetTypeSample;
    }

    IAssetConverter* AssetsBuilder::GetAssetConverter(const Type* assetType)
    {
        if (mAssetConverters.ContainsKey(assetType))
            return mAssetConverters[assetType];

        return &mStdAssetConverter;
    }
}
