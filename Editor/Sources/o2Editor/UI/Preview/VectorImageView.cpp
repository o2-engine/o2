#include "o2Editor/stdafx.h"
#include "VectorImageView.h"

#include "o2/Render/Render.h"
#include "o2/Render/Sprite.h"
#include "o2/Render/VectorSprite.h"
#include "o2Editor/UI/Preview/TextureView.h"

namespace Editor
{
    VectorImageView::VectorImageView(RefCounter* refCounter):
        ScrollView(refCounter)
    {
        mSprite = mmake<VectorSprite>();

        horGridEnabled = false;
        verGridEnabled = false;
    }

    VectorImageView::VectorImageView(RefCounter* refCounter, const VectorImageView& other):
        ScrollView(refCounter, other), mMode(other.mMode)
    {
        mSprite = mmake<VectorSprite>();

        horGridEnabled = false;
        verGridEnabled = false;

        SetImage(other.mImage);
    }

    VectorImageView& VectorImageView::operator=(const VectorImageView& other)
    {
        ScrollView::operator=(other);

        mMode = other.mMode;
        SetImage(other.mImage);

        return *this;
    }

    void VectorImageView::SetImage(const AssetRef<VectorImageAsset>& image)
    {
        mImage = image;
        mSprite->SetImageAsset(image);
        mSprite->SetMode(SpriteMode::Default);
        mMeshTriangles = 0;

        mFitPending = true;
        FitToView();
    }

    const AssetRef<VectorImageAsset>& VectorImageView::GetImage() const
    {
        return mImage;
    }

    void VectorImageView::SetMode(Mode mode)
    {
        mMode = mode;
    }

    VectorImageView::Mode VectorImageView::GetMode() const
    {
        return mMode;
    }

    void VectorImageView::FitToView()
    {
        Vec2F viewSize = layout->GetWorldRect().Size();
        Vec2F size = mImage ? mImage->GetSize() : Vec2F();
        if (viewSize.x < 1.0f || viewSize.y < 1.0f || size.x <= 0.0f || size.y <= 0.0f)
            return;

        mFitPending = false;

        float scale = Math::Max(size.x/(viewSize.x*0.9f), size.y/(viewSize.y*0.9f));
        Camera camera = mViewCamera;
        camera.SetPosition2D(size*0.5f);
        camera.SetScale2D(Vec2F(scale, scale));
        SetCamera(camera);
    }

    const Ref<VectorSprite>& VectorImageView::GetSprite() const
    {
        return mSprite;
    }

    UInt VectorImageView::GetMeshTrianglesCount() const
    {
        return mMeshTriangles;
    }

    void VectorImageView::RedrawContent()
    {
        if (!mImage)
            return;

        Vec2F size = mImage->GetSize();
        Basis area(Vec2F(), Vec2F(size.x, 0.0f), Vec2F(0.0f, size.y));

        RectF screen = layout->GetWorldRect();
        Vec2F a = screen.LeftBottom()*mScreenToLocalTransform, b = screen.RightTop()*mScreenToLocalTransform;
        float zoom = mViewCamera.GetScale2D().x > 0.0f ? 1.0f/mViewCamera.GetScale2D().x : 1.0f;
        TextureView::DrawChecker(RectF(0.0f, size.y, size.x, 0.0f), RectF(Math::Min(a.x, b.x), Math::Max(a.y, b.y), Math::Max(a.x, b.x),
                                                                       Math::Min(a.y, b.y)), 8.0f/zoom);

        mSprite->SetBasis(area);

        if (mMode == Mode::Image)
        {
            mSprite->Draw();
        }
        else
        {
            // The triangles are the ones the sprite takes for the zoom, without its raster
            bool rasters = VectorSprite::IsRasterCacheEnabled();
            VectorSprite::SetRasterCacheEnabled(false);

            mSprite->Draw();
            mMeshTriangles = mSprite->GetTrianglesCount();

            o2Render.SetDebugDrawMode(Render::DebugDrawMode::Wireframe, Color4(255, 150, 0, 235));
            mSprite->Draw();
            o2Render.SetDebugDrawMode(Render::DebugDrawMode::Normal);

            VectorSprite::SetRasterCacheEnabled(rasters);
        }

        float pixel = 1.0f/zoom;
        TextureView::DrawFrame(Vec2F(-pixel, -pixel), size + Vec2F(pixel, pixel), pixel, Color4(128, 128, 128, 160));
    }

    void VectorImageView::OnTransformUpdated()
    {
        ScrollView::OnTransformUpdated();

        if (mFitPending)
            FitToView();
    }

    void VectorImageView::OnCursorPressed(const Input::Cursor& cursor)
    {
        mPanning = true;
    }

    void VectorImageView::OnCursorStillDown(const Input::Cursor& cursor)
    {
        if (!mPanning || cursor.delta.Length() < Math::Epsilon)
            return;

        // The view gets the cursor in its local space
        mViewCameraTargetPos -= cursor.delta;
        mViewCamera.SetPosition2D(mViewCameraTargetPos);
        mNeedRedraw = true;
    }

    void VectorImageView::OnCursorReleased(const Input::Cursor& cursor)
    {
        mPanning = false;
    }
}
// --- META ---

ENUM_META(Editor::VectorImageView::Mode, Editor__VectorImageView__Mode)
{
    ENUM_ENTRY(Image);
    ENUM_ENTRY(Mesh);
}
END_ENUM_META;

DECLARE_CLASS(Editor::VectorImageView, Editor__VectorImageView);
// --- END META ---
