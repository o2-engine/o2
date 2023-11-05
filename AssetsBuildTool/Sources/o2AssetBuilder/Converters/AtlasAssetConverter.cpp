#include "o2/stdafx.h"
#include "AtlasAssetConverter.h"

#include "o2/Assets/Types/AtlasAsset.h"
#include "o2/Assets/Types/ImageAsset.h"
#include "o2/Assets/Assets.h"
#include "o2/Utils/Bitmap/Bitmap.h"
#include "o2/Utils/Debug/Log/LogStream.h"
#include "o2/Utils/FileSystem/FileSystem.h"
#include "o2AssetBuilder/ImageCompressor.h"

namespace o2
{
    Vector<const Type*> AtlasAssetConverter::GetProcessingAssetsTypes() const
    {
        Vector<const Type*> res;
        res.Add(&TypeOf(AtlasAsset));
        return res;
    }

    void AtlasAssetConverter::ConvertAsset(const AssetInfo& node)
    {
        String sourceAssetPath = mAssetsBuilder->GetSourceAssetsPath() + node.path;
        String buildedAssetPath = mAssetsBuilder->GetBuiltAssetsPath() + node.path;

        if (!o2FileSystem.IsFileExist(buildedAssetPath))
            o2FileSystem.WriteFile(buildedAssetPath, "");
    }

    void AtlasAssetConverter::RemoveAsset(const AssetInfo& node)
    {
        String buildedAssetPath = mAssetsBuilder->GetBuiltAssetsPath() + node.path;

        DataDocument atlasData;
        atlasData.LoadFromFile(buildedAssetPath);
        int pagesCount = atlasData["mPages"].GetMembersCount();

        for (int i = 0; i < pagesCount; i++)
            o2FileSystem.FileDelete(buildedAssetPath + (String)i + ".png");

        o2FileSystem.FileDelete(buildedAssetPath);
    }

    void AtlasAssetConverter::MoveAsset(const AssetInfo& nodeFrom, const AssetInfo& nodeTo)
    {
        String fullPathFrom = mAssetsBuilder->GetBuiltAssetsPath() + nodeFrom.path;
        String fullPathTo = mAssetsBuilder->GetBuiltAssetsPath() + nodeTo.path;

        DataDocument atlasData;
        atlasData.LoadFromFile(fullPathFrom);
        int pagesCount = atlasData["mPages"].GetMembersCount();

        for (int i = 0; i < pagesCount; i++)
            o2FileSystem.FileMove(fullPathFrom + (String)i + ".png", fullPathTo + (String)i + ".png");

        o2FileSystem.FileMove(fullPathFrom, fullPathTo);
    }

    Vector<UID> AtlasAssetConverter::AssetsPostProcess()
    {
        return CheckRebuildingAtlases();
    }

    void AtlasAssetConverter::Reset()
    {}

    Vector<UID> AtlasAssetConverter::CheckRebuildingAtlases()
    {
        Vector<UID> res;
        const Type* atlasAssetType = &TypeOf(AtlasAsset);

        for (auto info : mAssetsBuilder->mBuiltAssetsTree.allAssets)
        {
            if (info->meta->GetAssetType() == atlasAssetType)
            {
                if (CheckAtlasRebuilding(info))
                    res.Add(info->meta->ID());
            }
        }

        return res;
    }

    bool AtlasAssetConverter::CheckAtlasRebuilding(AssetInfo* atlasInfo)
    {
        DataDocument atlasData;
        atlasData.LoadFromFile(mAssetsBuilder->mBuiltAssetsPath + atlasInfo->path);

        Vector<Image> lastImages;
        lastImages = atlasData["mImages"];

        Vector<Image> currentImages;
        const Type* imageType = &TypeOf(ImageAsset);
        const UID& atlasId = atlasInfo->meta->ID();
        for (auto assetInfo : mAssetsBuilder->mBuiltAssetsTree.allAssets)
        {
            if (assetInfo->meta->GetAssetType() == imageType)
            {
                ImageAsset::Meta* imageMeta = (ImageAsset::Meta*)assetInfo->meta;
                if (imageMeta->atlasId == atlasId)
                    currentImages.Add(Image(imageMeta->ID(), assetInfo->editTime));
            }
        }

        bool isModified = mAssetsBuilder->mModifiedAssets.Contains(atlasInfo->meta->ID());
        if (isModified || ImagesListChanged(currentImages, lastImages))
        {
            RebuildAtlas(atlasInfo, currentImages);
            return true;
        }

        return false;
    }

    bool AtlasAssetConverter::ImagesListChanged(Vector<Image>& currentImages, Vector<Image>& lastImages)
    {
        if (currentImages.Count() != lastImages.Count())
            return true;

        for (auto lastImg : lastImages)
        {
            bool found = false;
            for (auto curImg : currentImages)
            {
                if (lastImg.id == curImg.id)
                {
                    if (lastImg.time != curImg.time)
                    {
                        return true;
                    }
                    else
                    {
                        found = true;
                        break;
                    }
                }
            }

            if (!found)
                return true;
        }

        for (auto curImg : currentImages)
        {
            if (mAssetsBuilder->mModifiedAssets.Contains(curImg.id))
                return true;
        }

        return false;
    }

    void AtlasAssetConverter::RebuildAtlas(AssetInfo* atlasInfo, Vector<Image>& images)
    {
        auto meta = ((AtlasAsset::Meta*)atlasInfo->meta)->GetResultPlatformMeta(mAssetsBuilder->GetPlatform());

        RectsPacker packer(meta.maxSize);
        float imagesBorder = (float)meta.border;

        // Initialize pack images
        Vector<ImagePackDef> packImages;
        for (auto img : images)
        {
            // Find image info
            AssetInfo* imgInfo = nullptr;
            mAssetsBuilder->mBuiltAssetsTree.allAssetsByUID.TryGetValue(img.id, imgInfo);
            if (!imgInfo)
            {
                mAssetsBuilder->mLog->Error("Can't find asset info by id: " + (String)img.id);
                continue;
            }

            String assetFullPath = mAssetsBuilder->GetSourceAssetsPath() + imgInfo->path;

            // Load bitmap
            Bitmap* bitmap = mnew Bitmap();
            if (!bitmap->Load(assetFullPath))
            {
                mAssetsBuilder->mLog->Error("Can't load bitmap for image asset: " + imgInfo->path);
                delete bitmap;
                continue;
            }

            // Create packing rect
            RectsPacker::Rect* packRect = packer.AddRect(bitmap->GetSize() +
                                                         Vec2F(imagesBorder*2.0f, imagesBorder*2.0f));

            ImagePackDef imagePackDef;
            imagePackDef.assetInfo = imgInfo;
            imagePackDef.bitmap = bitmap;
            imagePackDef.packRect = packRect;

            packImages.Add(imagePackDef);
        }

        // Try to pack
        if (!packer.Pack())
        {
            mAssetsBuilder->mLog->Warning("Atlas " + atlasInfo->path + " packing failed");
            return;
        }
        else mAssetsBuilder->mLog->Out("Atlas " + atlasInfo->path + " successfully packed");

        // Initialize bitmaps and pages
        int pagesCount = packer.GetPagesCount();
        Vector<Bitmap*> resAtlasBitmaps;
        Vector<AtlasAsset::Page> resAtlasPages;
        for (int i = 0; i < pagesCount; i++)
        {
            AtlasAsset::Page atlasPage;
            atlasPage.mId = i;
            atlasPage.mSize = packer.GetMaxSize();
            resAtlasPages.Add(atlasPage);

            Bitmap* newBitmap = mnew Bitmap(PixelFormat::R8G8B8A8, packer.GetMaxSize());
            newBitmap->Fill(Color4(255, 255, 255, 0));

            resAtlasBitmaps.Add(newBitmap);
        }

        // Save image assets data and fill pages
        for (auto imgDef : packImages)
        {
            imgDef.packRect->rect.left += imagesBorder;
            imgDef.packRect->rect.right -= imagesBorder;
            imgDef.packRect->rect.top -= imagesBorder;
            imgDef.packRect->rect.bottom += imagesBorder;

            resAtlasBitmaps[imgDef.packRect->page]->CopyImage(imgDef.bitmap,
                                                              imgDef.packRect->rect.LeftBottom());

            resAtlasPages[imgDef.packRect->page].mImagesRects.Add(imgDef.assetInfo->meta->ID(),
                                                                  imgDef.packRect->rect);

            SaveImageAsset(imgDef);
        }

        // Save pages bitmaps
        for (int i = 0; i < pagesCount; i++)
        {
            String builtPath = mAssetsBuilder->GetBuiltAssetsPath() + atlasInfo->path + (String)i;
            resAtlasBitmaps[i]->Save(builtPath + ".png", Bitmap::ImageType::Png);

            ImageCompressor::CompressImage(builtPath + ".png", builtPath, meta.format, 100);

            delete resAtlasBitmaps[i];
        }

        // Save atlas data
        String atlasFullPath = mAssetsBuilder->GetSourceAssetsPath() + atlasInfo->path;
        String atlasFullBuiltPath = mAssetsBuilder->GetBuiltAssetsPath() + atlasInfo->path;

        DataDocument atlasData;
        atlasData.LoadFromFile(atlasFullPath);
        atlasData["mPages"] = resAtlasPages;
        atlasData["mImages"] = images;

        atlasData.SaveToFile(atlasFullPath);
        atlasData.SaveToFile(atlasFullBuiltPath);

        atlasInfo->editTime = o2FileSystem.GetFileInfo(atlasFullPath).editDate;
    }

    void AtlasAssetConverter::SaveImageAsset(ImagePackDef& imgDef)
    {
        DataDocument imgData;
        imgData["mAtlasPage"] = imgDef.packRect->page;
        imgData["mSourceRect"] = (RectI)(imgDef.packRect->rect);
        String imageFullPath = mAssetsBuilder->GetBuiltAssetsPath() + imgDef.assetInfo->path;
        imgData.SaveToFile(imageFullPath);

        DataDocument metaData;
        metaData = imgDef.assetInfo->meta;
        metaData.SaveToFile(mAssetsBuilder->GetSourceAssetsPath() + imgDef.assetInfo->path + ".meta");
    }

    AtlasAssetConverter::Image::Image(const UID& id, const TimeStamp& time):
        id(id), time(time)
    {}

    bool AtlasAssetConverter::Image::operator==(const Image& other) const
    {
        return id == other.id;
    }

    bool AtlasAssetConverter::ImagePackDef::operator==(const ImagePackDef& other) const
    {
        return assetInfo == other.assetInfo && bitmap == other.bitmap && packRect == other.packRect;
    }
}
// --- META ---

DECLARE_CLASS(o2::AtlasAssetConverter);

DECLARE_CLASS(o2::AtlasAssetConverter::Image);
// --- END META ---
