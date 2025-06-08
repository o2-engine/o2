#include "o2Editor/stdafx.h"
#include "CurvePreview.h"

#include "o2/Render/Camera.h"
#include "o2/Render/Render.h"
#include "o2/Render/Sprite.h"
#include "o2/Scene/UI/WidgetLayer.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Utils/Math/Curve.h"

namespace Editor
{
    CurvePreview::CurvePreview(RefCounter* refCounter):
        Widget(refCounter)
    {
        mSprite = mmake<Sprite>();
        AddLayer("image", mSprite);

        mMesh = mmake<Mesh>();
        mMesh->Resize(4, 2);
    }

    CurvePreview::CurvePreview(RefCounter* refCounter, const CurvePreview& other):
        Widget(refCounter, other), mBackColor(other.mBackColor), mCurveColor(other.mCurveColor)
    {
        mSprite = GetLayerDrawable<Sprite>("image");
        RetargetStatesAnimations();

        mMesh = mmake<Mesh>();
        mMesh->Resize(4, 2);
    }

    CurvePreview& CurvePreview::operator=(const CurvePreview& other)
    {
        Widget::operator=(other);

        mBackColor = other.mBackColor;
        mCurveColor = other.mCurveColor;

        mSprite = GetLayerDrawable<Sprite>("image");
        RetargetStatesAnimations();

        mNeedRedraw = true;

        return *this;
    }

    void CurvePreview::SetCurve(const Ref<Curve>& curve)
    {
        if (mCurve == curve)
            return;

        if (mCurve)
            mCurve->onKeysChanged -= THIS_FUNC(OnCurveChanged);

        mCurve = curve;

        if (mCurve)
            mCurve->onKeysChanged += THIS_FUNC(OnCurveChanged);
    }

    void CurvePreview::Draw()
    {
        if (mNeedRedraw)
            Redraw();

        Widget::Draw();
    }

    void CurvePreview::SetBackColor(const Color4& color)
    {
        mBackColor = color;
        mNeedRedraw = true;
    }

    Color4 CurvePreview::GetBackColor() const
    {
        return mBackColor;
    }

    void CurvePreview::SetCurveColor(const Color4& color)
    {
        mCurveColor = color;
        mNeedRedraw = true;
    }

    Color4 CurvePreview::GetCurveColor() const
    {
        return mCurveColor;
    }

    String CurvePreview::GetCreateMenuCategory()
    {
        return "UI/Editor";
    }

    void CurvePreview::UpdateLayersLayouts()
    {
        mNeedRedraw = true;
        Widget::UpdateLayersLayouts();
    }

    void CurvePreview::Redraw()
    {
        if (!mCurve)
            return;

        TextureRef texture = mSprite->GetTexture();
        if (!texture || texture->GetSize() != (Vec2I)layout->GetSize())
        {
            texture = TextureRef(layout->GetSize(), TextureFormat::R8G8B8A8, Texture::Usage::RenderTarget);
            mSprite->SetTexture(texture);
            mSprite->SetTextureSrcRect(RectI(Vec2I(), texture->GetSize()));
        }

        o2Render.BindRenderTexture(texture);
        o2Render.Clear(mBackColor);

        Camera camera; camera.SetRect(mCurve->GetRect());
        camera.SetScale(camera.GetScale());

        Camera defaultCamera = o2Render.GetCamera();
        defaultCamera.leftBottom += Vec2F(10.0f, 11.0f);
        defaultCamera.rightTop   -= Vec2F(10.0f, 11.0f);

        Basis transform = camera.GetBasis().Inverted()*defaultCamera.GetBasis();

        for (auto& key : mCurve->GetKeys())
        {
            auto topPoints = key.GetTopApproximatedPoints();
            auto bottomPoints = key.GetBottomApproximatedPoints();

            auto lastTopPoint = topPoints[0]*transform;
            auto lastBottomPoint = bottomPoints[0]*transform;

            for (int i = 1; i < key.GetApproximatedPointsCount(); i++)
            {
                auto topPoint = topPoints[i]*transform;
                auto bottomPoint = bottomPoints[i]*transform;

                Color4 meshColor = mCurveColor; meshColor.a /= 2;

                mMesh->vertices[0] = Vertex(lastTopPoint, meshColor.ABGR(), 0, 0);
                mMesh->vertices[1] = Vertex(lastBottomPoint, meshColor.ABGR(), 0, 0);
                mMesh->vertices[2] = Vertex(topPoint, meshColor.ABGR(), 0, 0);
                mMesh->vertices[3] = Vertex(bottomPoint, meshColor.ABGR(), 0, 0);

                mMesh->indexes[0] = 0; mMesh->indexes[1] = 1; mMesh->indexes[2] = 2;
                mMesh->indexes[3] = 2; mMesh->indexes[4] = 1; mMesh->indexes[5] = 3;

                mMesh->vertexCount = 4;
                mMesh->polyCount = 2;

                mMesh->Draw();

                o2Render.DrawAALine(lastTopPoint, topPoint, mCurveColor, 1.5f);
                o2Render.DrawAALine(lastBottomPoint, bottomPoint, mCurveColor, 1.5f);

                lastTopPoint = topPoint;
                lastBottomPoint = bottomPoint;
            }
        }

        o2Render.UnbindRenderTexture();

        mNeedRedraw = false;
    }

    void CurvePreview::OnCurveChanged()
    {
        mNeedRedraw = true;
    }
}

DECLARE_TEMPLATE_CLASS(o2::LinkRef<Editor::CurvePreview>);
// --- META ---

DECLARE_CLASS(Editor::CurvePreview, Editor__CurvePreview);
// --- END META ---
