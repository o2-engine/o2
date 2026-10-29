#include "o2/stdafx.h"
#include "VectorImageAsset.h"

#include <atomic>
#include <chrono>

#include "o2/Assets/Assets.h"
#include "o2/Render/VectorGraphics/SvgParser.h"
#include "o2/Render/VectorGraphics/VectorTessellator.h"
#include "o2/Utils/Debug/Log/LogStream.h"
#include "o2/Utils/FileSystem/File.h"
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

        VectorTessellationParams GetTessellationParams(const Vec2F& scale, bool pixelSnapped, bool scalable)
        {
            VectorTessellationParams params;
            params.pixelScale = scale;
            params.pixelSnapped = pixelSnapped;
            params.scalable = scalable;
            return params;
        }

        // Header of the baked mesh file; the version goes up with every change of the tessellation or of the format
        struct BakedMeshHeader
        {
            char   magic[4] = { 'o', '2', 'v', 'm' };
            UInt32 version = 3;
            UInt64 sourceHash = 0;
            float  size[2] = { 0, 0 };
            float  pixelScale[2] = { 1, 1 };
            UInt32 verticesCount = 0;
            UInt32 indexesCount = 0;
        };

        const BakedMeshHeader bakedMeshFormat;
        const UInt bakedVertexBytes = sizeof(Vec2F)*2 + sizeof(float) + sizeof(Color32Bit);

        static_assert(sizeof(Vec2F) == sizeof(float)*2, "positions are written as float pairs");

        // Reads the header of the baked mesh file; false for another format or a size that does not fit the header
        bool ReadBakedMeshHeader(InFile& file, BakedMeshHeader& header)
        {
            if (!file.IsOpened())
                return false;

            UInt size = file.GetDataSize();
            if (size < sizeof(header))
                return false;

            file.ReadData(&header, sizeof(header));

            UInt64 expectedSize = sizeof(header) + (UInt64)header.verticesCount*bakedVertexBytes +
                (UInt64)header.indexesCount*sizeof(VertexIndex);

            return memcmp(header.magic, bakedMeshFormat.magic, sizeof(header.magic)) == 0 &&
                header.version == bakedMeshFormat.version && expectedSize == size;
        }

        // FNV-1a of the SVG text: the baked mesh belongs to the source it is built from
        UInt64 HashSource(const String& source)
        {
            UInt64 hash = 14695981039346656037ull;
            const char* data = source.Data();
            for (int i = 0, count = source.Length(); i < count; i++)
            {
                hash ^= (UInt8)data[i];
                hash *= 1099511628211ull;
            }

            return hash;
        }
    }

    UInt VectorImageAsset::mTessellationsCount = 0;
    double VectorImageAsset::mTessellationTime = 0.0;
    bool VectorImageAsset::mScalableMeshes = false;

    bool VectorImageMesh::GetBounds(Vec2F& min, Vec2F& max)
    {
        if (mBoundsState == 0)
            mBoundsState = mesh.GetBounds(mBoundsMin, mBoundsMax) ? 1 : -1;

        min = mBoundsMin;
        max = mBoundsMax;
        return mBoundsState > 0;
    }

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
        mMeshBaked = false;

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
        mMeshBaked = false;

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

    void VectorImageAsset::SetScalableMesh(bool scalable)
    {
        GetMeta()->scalableMesh = scalable;
    }

    bool VectorImageAsset::IsScalableMesh() const
    {
        return GetMeta()->scalableMesh;
    }

    void VectorImageAsset::SetRasterized(bool rasterized)
    {
        GetMeta()->rasterized = rasterized;
    }

    bool VectorImageAsset::IsRasterized() const
    {
        return GetMeta()->rasterized;
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
        bool scalable = IsMeshScalable();
        Vec2F scale = scalable ? Vec2F(1, 1) : QuantizePixelScale(pixelScale);
        bool sliced = slices != BorderF();
        pixelSnapped = pixelSnapped && !scalable;

        for (auto& cached : mMeshes)
        {
            if (cached->mesh.pixelScale.x == scale.x && cached->mesh.pixelScale.y == scale.y &&
                cached->sliced == sliced && (!sliced || cached->slices == slices) &&
                cached->pixelSnapped == pixelSnapped && cached->mesh.IsScalable() == scalable)
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

    bool VectorImageAsset::IsMeshScalable() const
    {
        if (mScalableMeshes)
            return true;

        auto meta = GetMeta();
        return meta && meta->scalableMesh;
    }

    bool VectorImageAsset::IsMeshBaked() const
    {
        return mMeshBaked;
    }

    String VectorImageAsset::GetBakedMeshPath(const String& builtPath)
    {
        return builtPath + ".mesh";
    }

    bool VectorImageAsset::IsBakedMeshCurrent(const String& meshPath)
    {
        if (!o2FileSystem.IsFileExist(meshPath))
            return false;

        InFile file(meshPath);
        BakedMeshHeader header;
        return ReadBakedMeshHeader(file, header);
    }

    bool VectorImageAsset::BakeMesh(const String& svg, const String& meshPath)
    {
        VectorImage image;
        String error;
        Vector<String> warnings;
        if (!SvgParser::Parse(svg, image, error, warnings) || image.shapes.IsEmpty())
            return false;

        VectorMesh mesh;
        VectorTessellator::Tessellate(image, mesh, GetTessellationParams(Vec2F(1, 1), false, true));
        if (!mesh.IsScalable())
            return false;

        BakedMeshHeader header;
        header.sourceHash = HashSource(svg);
        header.size[0] = mesh.size.x;
        header.size[1] = mesh.size.y;
        header.pixelScale[0] = mesh.pixelScale.x;
        header.pixelScale[1] = mesh.pixelScale.y;
        header.verticesCount = (UInt32)mesh.anchors.Count();
        header.indexesCount = (UInt32)mesh.indexes.Count();

        OutFile file(meshPath);
        if (!file.IsOpened())
            return false;

        file.WriteData(&header, sizeof(header));
        file.WriteData(mesh.anchors.Data(), header.verticesCount*sizeof(Vec2F));
        file.WriteData(mesh.normals.Data(), header.verticesCount*sizeof(Vec2F));
        file.WriteData(mesh.limits.Data(), header.verticesCount*sizeof(float));
        file.WriteData(mesh.baseColors.Data(), header.verticesCount*sizeof(Color32Bit));
        file.WriteData(mesh.indexes.Data(), header.indexesCount*sizeof(VertexIndex));

        return true;
    }

    bool VectorImageAsset::LoadBakedMesh(const String& meshPath)
    {
        if (!o2FileSystem.IsFileExist(meshPath))
            return false;

        InFile file(meshPath);
        BakedMeshHeader header;
        if (!ReadBakedMeshHeader(file, header) || header.sourceHash != HashSource(mSource))
        {
            GetAssetsLogStream()->WarningStr("Vector image " + mInfo.path +
                                             ": the baked mesh is of another source or engine version, rebuild the assets");
            return false;
        }

        if (header.indexesCount%3 != 0 || header.verticesCount == 0)
            return false;

        auto res = mmake<VectorImageMesh>();
        res->pixelSnapped = false;

        VectorMesh& mesh = res->mesh;
        mesh.size = Vec2F(header.size[0], header.size[1]);
        mesh.anchors.resize(header.verticesCount);
        mesh.normals.resize(header.verticesCount);
        mesh.limits.resize(header.verticesCount);
        mesh.baseColors.resize(header.verticesCount);
        mesh.indexes.resize(header.indexesCount);

        file.ReadData(mesh.anchors.Data(), header.verticesCount*sizeof(Vec2F));
        file.ReadData(mesh.normals.Data(), header.verticesCount*sizeof(Vec2F));
        file.ReadData(mesh.limits.Data(), header.verticesCount*sizeof(float));
        file.ReadData(mesh.baseColors.Data(), header.verticesCount*sizeof(Color32Bit));
        file.ReadData(mesh.indexes.Data(), header.indexesCount*sizeof(VertexIndex));

        for (VertexIndex index : mesh.indexes)
        {
            if (index >= header.verticesCount)
                return false;
        }

        mesh.positions.resize(header.verticesCount);
        mesh.colors.resize(header.verticesCount);
        mesh.Rescale(Vec2F(header.pixelScale[0], header.pixelScale[1]));

        mMeshes.Add(res);
        mMeshBaked = true;

        return true;
    }

    int VectorImageAsset::GetCachedMeshesCount() const
    {
        return mMeshes.Count();
    }

    void VectorImageAsset::ClearMeshCache()
    {
        mMeshes.Clear();
        mMeshBaked = false;
    }

    UInt VectorImageAsset::GetTessellationsCount()
    {
        return mTessellationsCount;
    }

    double VectorImageAsset::GetTessellationTime()
    {
        return mTessellationTime;
    }

    Function<bool(VectorImage&, const String&)>& VectorImageAsset::ImageRemap()
    {
        static Function<bool(VectorImage&, const String&)> remap;
        return remap;
    }

    void VectorImageAsset::SetImageRemap(const Function<bool(VectorImage&, const String&)>& remap)
    {
        ImageRemap() = remap;
    }

    void VectorImageAsset::SetScalableMeshes(bool scalable)
    {
        mScalableMeshes = scalable;
    }

    bool VectorImageAsset::IsScalableMeshes()
    {
        return mScalableMeshes;
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

        bool remapped = false;
        auto& remap = ImageRemap();
        if (remap && mValid && remap(mImage, path))
        {
            remapped = true;
            mVersion = NextVersion();
            mMeshes.Clear();
        }

        // The baked mesh is of the image as the file has it
        if (mValid && !remapped && !mImage.shapes.IsEmpty() && IsMeshScalable())
            LoadBakedMesh(GetBakedMeshPath(path));

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

        // A scalable mesh is one for all the scales, it is kept for the scale 1
        bool scalable = IsMeshScalable();
        Vec2F scale = scalable ? Vec2F(1, 1) : QuantizePixelScale(pixelScale);
        pixelSnapped = pixelSnapped && !scalable;

        for (auto& cached : mMeshes)
        {
            if (cached->mesh.pixelScale.x == scale.x && cached->mesh.pixelScale.y == scale.y &&
                cached->sliced == sliced && (!sliced || cached->slices == slices) &&
                cached->pixelSnapped == pixelSnapped && cached->mesh.IsScalable() == scalable)
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
            auto start = std::chrono::steady_clock::now();
            VectorTessellator::Tessellate(mImage, res->mesh, GetTessellationParams(scale, pixelSnapped, scalable));

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
        return sliceBorder == otherMeta->sliceBorder && defaultMode == otherMeta->defaultMode &&
            scalableMesh == otherMeta->scalableMesh && rasterized == otherMeta->rasterized;
    }
}

DECLARE_TEMPLATE_CLASS(o2::DefaultAssetMeta<o2::VectorImageAsset>);
DECLARE_TEMPLATE_CLASS(o2::AssetRef<o2::VectorImageAsset>);
// --- META ---

DECLARE_CLASS(o2::VectorImageAsset, o2__VectorImageAsset);

DECLARE_CLASS(o2::VectorImageAsset::Meta, o2__VectorImageAsset__Meta);
// --- END META ---
