#include "o2/stdafx.h"
#include "VectorSprite.h"

#include "o2/Assets/Assets.h"
#include "o2/Render/Render.h"
#include "o2/Utils/Debug/Debug.h"

namespace o2
{
    namespace
    {
        const int maxFrameMeshBuildings = 1;

        // Counts tessellations the sprites start at the frame; false when the frame has taken its share
        bool TakeMeshBuilding()
        {
            static UInt64 frame = 0;
            static int count = 0;

            if (frame != o2Render.GetFrameIndex())
            {
                frame = o2Render.GetFrameIndex();
                count = 0;
            }

            return count++ < maxFrameMeshBuildings;
        }

        // Shifts add up the rounding of float positions, the vertices are rebuilt from the image after this count
        const int maxMeshShifts = 32;
    }

    VectorSprite::Statistics VectorSprite::mStatistics;

    VectorSprite::VectorSprite()
    {
        UpdateColor();
    }

    VectorSprite::VectorSprite(const AssetRef<VectorImageAsset>& image)
    {
        UpdateColor();
        LoadFromImage(image);
    }

    VectorSprite::VectorSprite(const String& imagePath)
    {
        UpdateColor();
        LoadFromImage(imagePath);
    }

    VectorSprite::VectorSprite(const VectorSprite& other):
        IRectDrawable(other), mImageAsset(other.mImageAsset), mMode(other.mMode), mSlices(other.mSlices)
    {
        UpdateColor();
    }

    VectorSprite& VectorSprite::operator=(const VectorSprite& other)
    {
        mImageAsset = other.mImageAsset;
        mMode = other.mMode;
        mSlices = other.mSlices;

        IRectDrawable::operator=(other);

        mMeshDirty = true;

        return *this;
    }

    bool VectorSprite::operator==(const VectorSprite& other) const
    {
        return IRectDrawable::operator==(other) && mImageAsset == other.mImageAsset && mMode == other.mMode &&
            mSlices == other.mSlices;
    }

    bool VectorSprite::operator!=(const VectorSprite& other) const
    {
        return !operator==(other);
    }

    void VectorSprite::LoadFromImage(const AssetRef<VectorImageAsset>& image, bool setSizeByImage /*= true*/)
    {
        mImageAsset = image;
        mMeshDirty = true;

        if (!image)
            return;

        mSlices = image->GetMeta()->sliceBorder;
        mMode = image->GetMeta()->defaultMode;

        if (setSizeByImage)
            SetSize2D(image->GetSize());
    }

    void VectorSprite::LoadFromImage(const String& imagePath, bool setSizeByImage /*= true*/)
    {
        AssetRef<VectorImageAsset> assetRef = DynamicCast<VectorImageAsset>(o2Assets.GetAssetRef(imagePath).GetRef());
        if (assetRef)
            LoadFromImage(assetRef, setSizeByImage);
        else
            o2Debug.LogWarningStr("Can't load vector sprite from image by path (" + imagePath + "): image isn't exist");
    }

    void VectorSprite::LoadFromImage(UID imageId, bool setSizeByImage /*= true*/)
    {
        AssetRef<VectorImageAsset> assetRef = DynamicCast<VectorImageAsset>(o2Assets.GetAssetRef(imageId).GetRef());
        if (assetRef)
            LoadFromImage(assetRef, setSizeByImage);
        else
            o2Debug.LogWarningStr("Can't load vector sprite from image by id (" + imageId.ToString() + "): image isn't exist");
    }

    void VectorSprite::Draw()
    {
        if (!mEnabled)
            return;

        if (mImageAsset)
        {
            Vec2F viewPixelScale = o2Render.GetViewPixelScale();

            // The fringe goes out of the rectangle by less than a pixel; a clipped sprite keeps its vertices as they are
            RectF area = GetBasis().AABB();
            Vec2F fringe(1.0f/Math::Max(viewPixelScale.x, FLT_EPSILON), 1.0f/Math::Max(viewPixelScale.y, FLT_EPSILON));
            if (o2Render.IsClipped(RectF(area.left - fringe.x, area.top + fringe.y, area.right + fringe.x,
                                         area.bottom - fringe.y)))
            {
                mStatistics.culledDraws++;
                OnDrawn();
                return;
            }

            if (mMeshDirty || mBasisDirty || mMeshPostponed || mMeshImageVersion != mImageAsset->GetVersion() ||
                viewPixelScale.x != mMeshViewScale.x || viewPixelScale.y != mMeshViewScale.y)
            {
                UpdateMesh(viewPixelScale);
            }
            else if (mColorDirty)
                UpdateColors();

            if (mSourceMesh && mGeometry && !mGeometry->vertices.IsEmpty())
            {
                Ref<Material> material = GetMaterial();

                // Zero alpha draws nothing with the default material, a custom one may blend it another way
                if (mResultColor.a == 0 && !material)
                    mStatistics.culledDraws++;
                else
                {
                    o2Render.DrawGeometry(mGeometry, material);

                    mStatistics.draws++;
                    mStatistics.drawnTriangles += mGeometry->trianglesCount;
                    mStatistics.drawnVertices += (UInt64)mGeometry->vertices.Count();
                }
            }
        }

        OnDrawn();
    }

    Vec2F VectorSprite::GetOriginalSize() const
    {
        return mImageAsset ? mImageAsset->GetSize() : Vec2F();
    }

    void VectorSprite::SetMode(SpriteMode mode)
    {
        if (mode == mMode)
            return;

        mMode = mode;
        mMeshDirty = true;
    }

    SpriteMode VectorSprite::GetMode() const
    {
        return mMode;
    }

    void VectorSprite::SetSliceBorder(const BorderI& border)
    {
        if (mSlices == border)
            return;

        mSlices = border;
        mMeshDirty = true;
    }

    BorderI VectorSprite::GetSliceBorder() const
    {
        return mSlices;
    }

    void VectorSprite::SetImageAsset(const AssetRef<VectorImageAsset>& asset)
    {
        LoadFromImage(asset, false);
    }

    AssetRef<VectorImageAsset> VectorSprite::GetImageAsset() const
    {
        return mImageAsset;
    }

    const String& VectorSprite::GetImageName() const
    {
        if (mImageAsset)
            return mImageAsset->GetPath();

        return String::empty;
    }

    void VectorSprite::NormalizeSize()
    {
        SetSize2D(GetOriginalSize());
    }

    void VectorSprite::NormalizeAspectByWidth()
    {
        Vec2F imageSize = GetOriginalSize();
        if (imageSize.x > 0.0f)
            SetSize(Vec2F(mSize.x, mSize.x*imageSize.y/imageSize.x));
    }

    void VectorSprite::NormalizeAspectByHeight()
    {
        Vec2F imageSize = GetOriginalSize();
        if (imageSize.y > 0.0f)
            SetSize(Vec2F(mSize.y*imageSize.x/imageSize.y, mSize.y));
    }

    void VectorSprite::NormalizeAspect()
    {
        Vec2F imageSize = GetOriginalSize();
        if (imageSize.x > imageSize.y)
            NormalizeAspectByHeight();
        else
            NormalizeAspectByWidth();
    }

    UInt VectorSprite::GetTrianglesCount() const
    {
        return mSourceMesh && mGeometry ? mGeometry->trianglesCount : 0;
    }

    UInt VectorSprite::GetVerticesCount() const
    {
        return mGeometry ? (UInt)mGeometry->vertices.Count() : 0;
    }

    Vec2F VectorSprite::GetMeshPixelScale() const
    {
        return mSourceMesh ? mSourceMesh->mesh.pixelScale : Vec2F();
    }

    bool VectorSprite::IsMeshPixelSnapped() const
    {
        return mSourceMesh && mSourceMesh->pixelSnapped;
    }

    UInt VectorSprite::GetMeshRebuildsCount() const
    {
        return mMeshRebuildsCount;
    }

    UInt VectorSprite::GetColorUpdatesCount() const
    {
        return mColorUpdatesCount;
    }

    UInt64 VectorSprite::GetTotalDrawnTriangles()
    {
        return mStatistics.drawnTriangles;
    }

    const VectorSprite::Statistics& VectorSprite::GetStatistics()
    {
        return mStatistics;
    }

    void VectorSprite::OnDeserialized(const DataValue& node)
    {
        Transform::OnDeserialized(node);

        UpdateColor();
        mMeshDirty = true;
    }

    void VectorSprite::OnDeserializedDelta(const DataValue& node, const IObject& origin)
    {
        Transform::OnDeserializedDelta(node, origin);

        UpdateColor();
        mMeshDirty = true;
    }

    void VectorSprite::BasisChanged()
    {
        mBasisDirty = true;
    }

    void VectorSprite::OnColorChanged()
    {
        mColorDirty = true;
    }

    void VectorSprite::UpdateMesh(const Vec2F& viewPixelScale)
    {
        bool sameImage = mMeshImageVersion == mImageAsset->GetVersion();
        bool sameView = viewPixelScale == mMeshViewScale;

        mMeshViewScale = viewPixelScale;
        mMeshImageVersion = mImageAsset->GetVersion();

        Vec2F imageSize = mImageAsset->GetSize();
        Vec2F slicedSize;
        Basis basis = GetImageBasis(slicedSize);
        float z = mTransform.origin.z;

        // A layout update sets the same basis again: nothing to rebuild
        if (!mMeshDirty && sameImage && sameView && mGeometry && basis == mMeshBasis && slicedSize == mMeshSlicedSize &&
            z == mMeshDepth)
        {
            mBasisDirty = false;

            if (mColorDirty)
                UpdateColors();

            return;
        }

        mMeshDirty = mMeshDirty || mBasisDirty;
        mBasisDirty = false;

        if (imageSize.x <= 0.0f || imageSize.y <= 0.0f || slicedSize.x <= 0.0f || slicedSize.y <= 0.0f)
        {
            mSourceMesh = nullptr;
            mGeometry = nullptr;
            mMeshDirty = false;
            mColorDirty = false;
            return;
        }

        bool sliced = mMode == SpriteMode::Sliced;
        Vec2F drawSize = imageSize;
        BorderF slices;

        if (sliced)
        {
            slices = mSlices;

            // An axis without borders is stretched as a whole, with borders only the view scales the corners
            if (mSlices.left != 0 || mSlices.right != 0)
                drawSize.x = slicedSize.x;

            if (mSlices.top != 0 || mSlices.bottom != 0)
                drawSize.y = slicedSize.y;

            // Cutting the mesh by the reduced borders drops the middle instead of squeezing the corners into it
            float excessX = slices.left + slices.right - slicedSize.x;
            if (excessX > 0.0f)
            {
                slices.left -= excessX*0.5f;
                slices.right -= excessX*0.5f;
            }

            float excessY = slices.top + slices.bottom - slicedSize.y;
            if (excessY > 0.0f)
            {
                slices.top -= excessY*0.5f;
                slices.bottom -= excessY*0.5f;
            }
        }

        Vec2F pixelScale(basis.xv.Length()/drawSize.x*viewPixelScale.x, basis.yv.Length()/drawSize.y*viewPixelScale.y);

        // The view scale changes continuously when zooming, the cached mesh changes by steps
        Vec2F meshScale = VectorImageAsset::QuantizePixelScale(pixelScale);
        bool pixelSnapped = IsOnWholePixels(basis, viewPixelScale);

        bool sameMesh = sameImage && mSourceMesh && mSourceMesh->mesh.pixelScale.x == meshScale.x &&
            mSourceMesh->mesh.pixelScale.y == meshScale.y && mSourceMesh->sliced == (sliced && slices != BorderF()) &&
            (!mSourceMesh->sliced || mSourceMesh->slices == slices) && mSourceMesh->pixelSnapped == pixelSnapped;

        // Zoom asks new meshes for all sprites at once: they are built by few a frame, the previous ones are drawn so far
        bool sameSlices = mSourceMesh && mSourceMesh->sliced == (sliced && slices != BorderF()) &&
            (!mSourceMesh->sliced || mSourceMesh->slices == slices);

        mMeshPostponed = !sameMesh && !mImageAsset->IsMeshBuilt(pixelScale, sliced ? slices : BorderF(), pixelSnapped) &&
            !TakeMeshBuilding() && sameImage && sameSlices && mGeometry;

        if (mMeshPostponed)
            sameMesh = true;

        if (sameMesh && !mMeshDirty)
        {
            if (mColorDirty)
                UpdateColors();

            return;
        }

        mMeshDirty = false;

        // Scrolling only moves the basis: the vertices are shifted
        if (sameMesh && mGeometry && basis.xv == mMeshBasis.xv && basis.yv == mMeshBasis.yv &&
            slicedSize == mMeshSlicedSize && z == mMeshDepth)
        {
            Vec2F shift = basis.origin - mMeshBasis.origin;
            if (shift == Vec2F() || mMeshShiftsCount < maxMeshShifts)
            {
                if (shift != Vec2F())
                    ShiftVertices(shift);

                mMeshBasis.origin = basis.origin;

                if (mColorDirty)
                    UpdateColors();

                return;
            }
        }

        mColorDirty = false;
        mMeshBasis = basis;
        mMeshSlicedSize = slicedSize;
        mMeshDepth = z;
        mMeshShiftsCount = 0;

        if (!sameMesh)
        {
            mSourceMesh = sliced ? mImageAsset->GetSlicedMesh(pixelScale, slices, pixelSnapped) :
                mImageAsset->GetMesh(pixelScale, pixelSnapped);
        }

        if (!mSourceMesh)
        {
            mGeometry = nullptr;
            return;
        }

        mMeshRebuildsCount++;
        mStatistics.rebuiltVertices += (UInt64)mSourceMesh->mesh.positions.size();

        const VectorMesh& mesh = mSourceMesh->mesh;

        RenderGeometry& geometry = GetChangingGeometry();
        geometry.vertices.resize(mesh.positions.size());
        geometry.indexes = mesh.indexes.Data();
        geometry.trianglesCount = mesh.GetTrianglesCount();
        geometry.indexesOwner = mSourceMesh;

        Vertex* vertices = geometry.vertices.Data();
        int count = geometry.vertices.Count();

        if (sliced)
        {
            mesh.FillSlicedVertices(vertices, basis, slices, slicedSize);

            for (int i = 0; i < count; i++)
                vertices[i].z = z;
        }
        else
        {
            // Same mapping as VectorMesh::FillVertices, without calls per vertex
            const Vec2F* positions = mesh.positions.Data();
            float xvx = basis.xv.x/imageSize.x, xvy = basis.xv.y/imageSize.x;
            float yvx = basis.yv.x/imageSize.y, yvy = basis.yv.y/imageSize.y;
            float originX = basis.origin.x + basis.yv.x, originY = basis.origin.y + basis.yv.y;

            for (int i = 0; i < count; i++)
            {
                Vertex& vertex = vertices[i];
                vertex.x = originX + xvx*positions[i].x - yvx*positions[i].y;
                vertex.y = originY + xvy*positions[i].x - yvy*positions[i].y;
                vertex.z = z;
                vertex.tu = 0.0f;
                vertex.tv = 0.0f;
            }
        }

        if (count > 0)
        {
            float left = vertices[0].x, right = vertices[0].x, bottom = vertices[0].y, top = vertices[0].y;
            for (int i = 1; i < count; i++)
            {
                left = Math::Min(left, vertices[i].x);
                right = Math::Max(right, vertices[i].x);
                bottom = Math::Min(bottom, vertices[i].y);
                top = Math::Max(top, vertices[i].y);
            }

            mBounds = RectF(left, top, right, bottom);
        }

        UpdateColors();
    }

    void VectorSprite::ShiftVertices(const Vec2F& shift)
    {
        RenderGeometry& geometry = GetChangingGeometry();

        Vertex* vertices = geometry.vertices.Data();
        int count = geometry.vertices.Count();
        for (int i = 0; i < count; i++)
        {
            vertices[i].x += shift.x;
            vertices[i].y += shift.y;
        }

        mBounds = mBounds + shift;

        mMeshShiftsCount++;
        mMeshRebuildsCount++;
        mStatistics.shiftedVertices += (UInt64)count;
    }

    RenderGeometry& VectorSprite::GetChangingGeometry()
    {
        if (!mGeometry)
            mGeometry = mmake<RenderGeometry>();
        else if (mGeometry->IsQueued())
        {
            // The sprite is drawn several times a frame: the batch keeps the vertices of the previous drawing
            auto geometry = mmake<RenderGeometry>();
            geometry->vertices = mGeometry->vertices;
            geometry->indexes = mGeometry->indexes;
            geometry->trianglesCount = mGeometry->trianglesCount;
            geometry->indexesOwner = mGeometry->indexesOwner;
            mGeometry = geometry;
        }

        mGeometry->OnChanged();
        return *mGeometry;
    }

    bool VectorSprite::IsOnWholePixels(const Basis& basis, const Vec2F& viewPixelScale)
    {
        const float threshold = 0.01f;
        auto isWhole = [&](float value) { return Math::Abs(value - Math::Round(value)) < threshold; };

        if (Math::Abs(basis.xv.y) > threshold || Math::Abs(basis.yv.x) > threshold)
            return false;

        Vec2F corner = basis.origin + basis.xv + basis.yv;
        return isWhole(basis.origin.x*viewPixelScale.x) && isWhole(basis.origin.y*viewPixelScale.y) &&
            isWhole(corner.x*viewPixelScale.x) && isWhole(corner.y*viewPixelScale.y);
    }

    void VectorSprite::UpdateColors()
    {
        mColorDirty = false;

        if (!mSourceMesh || !mGeometry || mGeometry->vertices.IsEmpty())
            return;

        RenderGeometry& geometry = GetChangingGeometry();

        // Same integer arithmetic as Color4 multiplication and VectorMesh::FillVertices
        const UInt multipliers[4] = { (UInt)mResultColor.r, (UInt)mResultColor.g, (UInt)mResultColor.b,
                                      (UInt)mResultColor.a };

        const Color32Bit* colors = mSourceMesh->mesh.colors.Data();
        Vertex* vertices = geometry.vertices.Data();
        int count = geometry.vertices.Count();

        mColorUpdatesCount++;
        mStatistics.recoloredVertices += (UInt64)count;

        if (mResultColor == Color4::White())
        {
            for (int i = 0; i < count; i++)
                vertices[i].color = colors[i];

            return;
        }

        for (int i = 0; i < count; i++)
        {
            Color32Bit color = colors[i];
            vertices[i].color = ((color & 0xFF)*multipliers[0]/255) |
                ((((color >> 8) & 0xFF)*multipliers[1]/255) << 8) |
                ((((color >> 16) & 0xFF)*multipliers[2]/255) << 16) |
                ((((color >> 24) & 0xFF)*multipliers[3]/255) << 24);
        }
    }

    Basis VectorSprite::GetImageBasis(Vec2F& slicedSize) const
    {
        Basis basis = mTransform.ToBasis();
        slicedSize = Vec2F(Math::Abs(mSize.x*mScale.x), Math::Abs(mSize.y*mScale.y));

        if (mMode != SpriteMode::FixedAspect)
            return basis;

        Vec2F imageSize = mImageAsset->GetSize();
        if (imageSize.x <= 0.0f || imageSize.y <= 0.0f)
            return basis;

        Basis nonSizedBasis = mNonSizedTransform.ToBasis();

        float fitHeight = mSize.x/imageSize.x*imageSize.y;
        if (fitHeight > mSize.y)
        {
            float fitWidth = mSize.y/imageSize.y*imageSize.x;
            Vec2F offset = nonSizedBasis.xv*((mSize.x - fitWidth)*0.5f);
            return Basis(basis.origin + offset, basis.xv - offset*2.0f, basis.yv);
        }

        Vec2F offset = nonSizedBasis.yv*((mSize.y - fitHeight)*0.5f);
        return Basis(basis.origin + offset, basis.xv, basis.yv - offset*2.0f);
    }
}
// --- META ---

DECLARE_CLASS(o2::VectorSprite, o2__VectorSprite);
// --- END META ---
