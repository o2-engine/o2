#include "o2/stdafx.h"
#include "VectorImageAsset.h"

#include <atomic>
#include <chrono>

#include "o2/Assets/Assets.h"
#include "o2/Render/VectorGraphics/SvgParser.h"
#include "o2/Render/VectorGraphics/VectorTessellator.h"
#include "o2/Utils/Debug/Log/LogStream.h"
#include "o2/Utils/FileSystem/FileSystem.h"

namespace o2
{
    namespace
    {
        const float minPixelScale = 1.0f/16.0f;
        const float maxPixelScale = 64.0f;
        const float pixelScaleStepsPerOctave = 1.0f;
        const float maxPixelScaleRatio = 4.0f;
        const double slowTessellationTime = 0.2; // Seconds of building a mesh that are worth a warning: a frame hitch

        float QuantizeScale(float scale)
        {
            if (!(scale > minPixelScale))
                return minPixelScale;

            if (scale > maxPixelScale)
                return maxPixelScale;

            return std::pow(2.0f, Math::Round(std::log2(scale)*pixelScaleStepsPerOctave)/pixelScaleStepsPerOctave);
        }

        // Versions are unique between assets: a drawable given another asset sees another version
        UInt NextVersion()
        {
            static std::atomic<UInt> lastVersion{ 0 };
            return ++lastVersion;
        }
    }

    UInt VectorImageAsset::mTessellationsCount = 0;
    double VectorImageAsset::mTessellationTime = 0.0;

    VectorImageAsset::VectorImageAsset():
        Asset(mmake<Meta>())
    {}

    VectorImageAsset::VectorImageAsset(const VectorImageAsset& other):
        Asset(other), mSource(other.mSource), mImage(other.mImage), mValid(other.mValid), mError(other.mError),
        mWarnings(other.mWarnings), mVersion(NextVersion())
    {}

    VectorImageAsset::~VectorImageAsset()
    {
        if (mSubscribedOnRebuild && Assets::IsSingletonInitialzed())
            o2Assets.onAssetsRebuilt -= MakeFunction(this, &VectorImageAsset::OnAssetsRebuilt);
    }

    VectorImageAsset& VectorImageAsset::operator=(const VectorImageAsset& other)
    {
        Asset::operator=(other);

        mSource = other.mSource;
        mImage = other.mImage;
        mValid = other.mValid;
        mError = other.mError;
        mWarnings = other.mWarnings;

        mVersion = NextVersion();
        mMeshes.Clear();

        return *this;
    }

    bool VectorImageAsset::SetSource(const String& svg)
    {
        mSource = svg;
        mImage.Clear();
        mError.Clear();
        mWarnings.Clear();

        mValid = SvgParser::Parse(mSource, mImage, mError, mWarnings);
        if (!mValid)
            mImage.Clear();

        mVersion = NextVersion();
        mMeshes.Clear();

        return mValid;
    }

    const String& VectorImageAsset::GetSource() const
    {
        return mSource;
    }

    const VectorImage& VectorImageAsset::GetImage() const
    {
        return mImage;
    }

    bool VectorImageAsset::IsValid() const
    {
        return mValid;
    }

    const String& VectorImageAsset::GetError() const
    {
        return mError;
    }

    const Vector<String>& VectorImageAsset::GetWarnings() const
    {
        return mWarnings;
    }

    Vec2F VectorImageAsset::GetSize() const
    {
        return mImage.size;
    }

    float VectorImageAsset::GetWidth() const
    {
        return mImage.size.x;
    }

    float VectorImageAsset::GetHeight() const
    {
        return mImage.size.y;
    }

    void VectorImageAsset::SetSliceBorder(const BorderI& border)
    {
        GetMeta()->sliceBorder = border;
    }

    BorderI VectorImageAsset::GetSliceBorder() const
    {
        return GetMeta()->sliceBorder;
    }

    void VectorImageAsset::SetDefaultMode(SpriteMode mode)
    {
        GetMeta()->defaultMode = mode;
    }

    SpriteMode VectorImageAsset::GetDefaultMode() const
    {
        return GetMeta()->defaultMode;
    }

    Ref<VectorImageMesh> VectorImageAsset::GetMesh(const Vec2F& pixelScale, bool pixelSnapped /*= true*/)
    {
        return GetMesh(pixelScale, BorderF(), false, pixelSnapped);
    }

    Ref<VectorImageMesh> VectorImageAsset::GetSlicedMesh(const Vec2F& pixelScale, const BorderF& slices,
                                                         bool pixelSnapped /*= true*/)
    {
        return GetMesh(pixelScale, slices, slices != BorderF(), pixelSnapped);
    }

    bool VectorImageAsset::IsMeshBuilt(const Vec2F& pixelScale, const BorderF& slices /*= BorderF()*/,
                                       bool pixelSnapped /*= true*/) const
    {
        Vec2F scale = QuantizePixelScale(pixelScale);
        bool sliced = slices != BorderF();

        for (auto& cached : mMeshes)
        {
            if (cached->mesh.pixelScale.x == scale.x && cached->mesh.pixelScale.y == scale.y &&
                cached->sliced == sliced && (!sliced || cached->slices == slices) && cached->pixelSnapped == pixelSnapped)
            {
                return true;
            }
        }

        return false;
    }

    Vec2F VectorImageAsset::QuantizePixelScale(const Vec2F& pixelScale)
    {
        Vec2F scale(QuantizeScale(pixelScale.x), QuantizeScale(pixelScale.y));
        float maxScale = Math::Max(scale.x, scale.y);

        return Vec2F(Math::Max(scale.x, maxScale/maxPixelScaleRatio), Math::Max(scale.y, maxScale/maxPixelScaleRatio));
    }

    int VectorImageAsset::GetCachedMeshesCount() const
    {
        return mMeshes.Count();
    }

    void VectorImageAsset::ClearMeshCache()
    {
        mMeshes.Clear();
    }

    UInt VectorImageAsset::GetTessellationsCount()
    {
        return mTessellationsCount;
    }

    double VectorImageAsset::GetTessellationTime()
    {
        return mTessellationTime;
    }

    UInt VectorImageAsset::GetVersion() const
    {
        return mVersion;
    }

    Ref<VectorImageAsset::Meta> VectorImageAsset::GetMeta() const
    {
        return DynamicCast<Meta>(mInfo.meta);
    }

    Vector<String> VectorImageAsset::GetFileExtensions()
    {
        return { "svg" };
    }

    void VectorImageAsset::LoadData(const String& path)
    {
        SetSource(o2FileSystem.ReadFile(path));

        if (!mValid)
            GetAssetsLogStream()->ErrorStr("Failed to load vector image " + mInfo.path + ": " + mError);

        for (auto& warning : mWarnings)
            GetAssetsLogStream()->WarningStr("Vector image " + mInfo.path + ": " + warning);

        if (!mSubscribedOnRebuild)
        {
            o2Assets.onAssetsRebuilt += MakeFunction(this, &VectorImageAsset::OnAssetsRebuilt);
            mSubscribedOnRebuild = true;
        }
    }

    void VectorImageAsset::SaveData(const String& path) const
    {
        o2FileSystem.WriteFile(path, mSource);
    }

    Ref<VectorImageMesh> VectorImageAsset::GetMesh(const Vec2F& pixelScale, const BorderF& slices, bool sliced,
                                                   bool pixelSnapped)
    {
        if (!mValid || mImage.shapes.IsEmpty())
            return nullptr;

        Vec2F scale = QuantizePixelScale(pixelScale);

        for (auto& cached : mMeshes)
        {
            if (cached->mesh.pixelScale.x == scale.x && cached->mesh.pixelScale.y == scale.y &&
                cached->sliced == sliced && (!sliced || cached->slices == slices) && cached->pixelSnapped == pixelSnapped)
            {
                return cached;
            }
        }

        Ref<VectorImageMesh> whole = sliced ? GetMesh(scale, BorderF(), false, pixelSnapped) : nullptr;

        if (mMeshes.Count() >= mMaxCachedMeshes)
        {
            mMeshes.Clear();

            if (whole)
                mMeshes.Add(whole);
        }

        auto res = mmake<VectorImageMesh>();
        res->sliced = sliced;
        res->slices = slices;
        res->pixelSnapped = pixelSnapped;

        if (sliced)
        {
            res->mesh = whole->mesh;
            res->mesh.SplitBySlices(slices);
        }
        else
        {
            VectorTessellationParams params;
            params.pixelScale = scale;
            params.pixelSnapped = pixelSnapped;

            auto start = std::chrono::steady_clock::now();
            VectorTessellator::Tessellate(mImage, res->mesh, params);

            double time = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
            mTessellationsCount++;
            mTessellationTime += time;

            if (time > slowTessellationTime)
            {
                GetAssetsLogStream()->WarningStr(String::Format(
                    "Vector image %s was tessellated in %.0f ms for the scale %.2f x %.2f: %u triangles", mInfo.path.Data(),
                    time*1000.0, scale.x, scale.y, res->mesh.GetTrianglesCount()));
            }
        }

        mMeshes.Add(res);

        return res;
    }

    void VectorImageAsset::OnAssetsRebuilt(const Vector<UID>& changedAssets)
    {
        if (changedAssets.Contains(GetUID()))
            Reload();
    }

    bool VectorImageAsset::Meta::IsEqual(AssetMeta* other) const
    {
        if (!AssetMeta::IsEqual(other))
            return false;

        Meta* otherMeta = (Meta*)other;
        return sliceBorder == otherMeta->sliceBorder && defaultMode == otherMeta->defaultMode;
    }
}

DECLARE_TEMPLATE_CLASS(o2::DefaultAssetMeta<o2::VectorImageAsset>);
DECLARE_TEMPLATE_CLASS(o2::AssetRef<o2::VectorImageAsset>);
// --- META ---

DECLARE_CLASS(o2::VectorImageAsset, o2__VectorImageAsset);

DECLARE_CLASS(o2::VectorImageAsset::Meta, o2__VectorImageAsset__Meta);
// --- END META ---
