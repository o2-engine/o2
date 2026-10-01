#include "o2Editor/stdafx.h"
#include "PipelinePairViews.h"

#include "o2/Render/Render.h"
#include "o2/Render/Sprite.h"
#include "o2/Render/Text.h"
#include "o2/Scene/UI/WidgetLayer.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Utils/Bitmap/Bitmap.h"
#include "o2Editor/Windows/PipelineWindow/PipelineControls.h"

namespace Editor
{
    namespace PipelinePairDraw
    {
        RectF FitRect(const Vec2I& imageSize, const RectF& box)
        {
            if (imageSize.x <= 0 || imageSize.y <= 0 || box.Width() <= 0.0f || box.Height() <= 0.0f)
                return box;

            float k = Math::Min(box.Width()/imageSize.x, box.Height()/imageSize.y);
            Vec2F size(imageSize.x*k, imageSize.y*k);
            Vec2F center = box.Center();
            return RectF(center.x - size.x*0.5f, center.y + size.y*0.5f, center.x + size.x*0.5f, center.y - size.y*0.5f);
        }

        void DrawClipped(Sprite& sprite, const Vec2I& textureSize, const RectF& imageRect, float fromX, float toX)
        {
            float width = imageRect.Width();
            if (width <= 0.0f || textureSize.x <= 0 || textureSize.y <= 0)
                return;

            float left = Math::Max(fromX, imageRect.left), right = Math::Min(toX, imageRect.right);
            if (right - left < 0.5f)
                return;

            int x0 = Math::Clamp((int)Math::Round((left - imageRect.left)/width*textureSize.x), 0, textureSize.x);
            int x1 = Math::Clamp((int)Math::Round((right - imageRect.left)/width*textureSize.x), x0, textureSize.x);
            if (x1 <= x0)
                return;

            sprite.mode = SpriteMode::Default;
            sprite.SetTextureSrcRect(RectI(x0, 0, x1, textureSize.y));
            sprite.rect = RectF(imageRect.left + width*x0/textureSize.x, imageRect.top, imageRect.left + width*x1/textureSize.x, imageRect.bottom);
            sprite.Draw();
        }

        Ref<Sprite> MakeChecker()
        {
            auto checker = mmake<Sprite>("ui/pipeline/checker.png");
            checker->mode = SpriteMode::Tiled;
            checker->transparency = 0.45f;
            return checker;
        }

        void DrawChecker(Sprite& checker, const RectF& rect, float fromX, float toX)
        {
            float left = Math::Max(fromX, rect.left), right = Math::Min(toX, rect.right);
            if (right - left < 0.5f)
                return;

            checker.rect = RectF(left, rect.top, right, rect.bottom);
            checker.Draw();
        }

        float PixelSize()
        {
            float scale = o2Render.GetCamera().GetScale2D().x;
            return scale > 0.0f ? scale : 1.0f;
        }

        float BadgeScale(float pixelSize)
        {
            return Math::Min(pixelSize, maxBadgeScale);
        }

        static Vector<Vec2F> RoundedRectPoints(const RectF& rect, float radius)
        {
            float r = Math::Clamp(radius, 0.0f, Math::Min(rect.Width(), rect.Height())*0.5f);
            const int segments = 6;
            Vector<Vec2F> points;
            auto corner = [&](const Vec2F& center, float fromAngle)
            {
                for (int i = 0; i <= segments; i++)
                {
                    float a = fromAngle + Math::PI()*0.5f*i/segments;
                    points.Add(center + Vec2F(Math::Cos(a), Math::Sin(a))*r);
                }
            };
            corner(Vec2F(rect.right - r, rect.top - r), 0.0f);
            corner(Vec2F(rect.left + r, rect.top - r), Math::PI()*0.5f);
            corner(Vec2F(rect.left + r, rect.bottom + r), Math::PI());
            corner(Vec2F(rect.right - r, rect.bottom + r), Math::PI()*1.5f);
            return points;
        }

        RectF GripRect(const RectF& box, float x, float pixelSize)
        {
            float k = BadgeScale(pixelSize);
            Vec2F half(gripWidth*0.5f*k, gripHeight*0.5f*k);
            float y = box.Center().y;
            return RectF(x - half.x, y + half.y, x + half.x, y - half.y);
        }

        void DrawDivider(const RectF& box, float x, bool active)
        {
            float pixel = PixelSize(), k = BadgeScale(pixel);
            Vector<Vec2F> line = { Vec2F(x, box.bottom), Vec2F(x, box.top) };
            // A faint light halo keeps the dark line visible on a dark picture
            o2Render.DrawAALine(line, Color4(255, 255, 255, 50), 4.0f);
            o2Render.DrawAALine(line, Color4(14, 15, 20, 217), 1.0f);

            // A light ring one screen pixel wide round the dark pill
            float opacity = active ? 1.0f : 0.6f;
            RectF grip = GripRect(box, x, pixel);
            Vector<Vec2F> points = RoundedRectPoints(grip, 3.0f*k);
            o2Render.DrawFilledPolygon(points, Color4(14, 15, 20, (int)(230*opacity)));
            points.Add(points[0]);
            o2Render.DrawAALine(points, Color4(255, 255, 255, (int)(89*opacity)), 1.0f);
        }

        Ref<Bitmap> DisplayCopy(const Ref<Bitmap>& bitmap)
        {
            if (!bitmap)
                return nullptr;

            const int maxSide = 1024;
            Vec2I size = bitmap->GetSize();
            if (size.x <= maxSide && size.y <= maxSide)
                return bitmap;

            float k = (float)maxSide/Math::Max(size.x, size.y);
            return bitmap->Resized(Vec2I(Math::Max(1, (int)(size.x*k)), Math::Max(1, (int)(size.y*k))));
        }
    }

    PipelineCompareView::PipelineCompareView(RefCounter* refCounter):
        Widget(refCounter)
    {
        mInputSprite = mmake<Sprite>();
        mResultSprite = mmake<Sprite>();
        mChecker = PipelinePairDraw::MakeChecker();
        AddLayer("frame", mmake<Sprite>("ui/UI4_Editbox_regular.png"), Layout::BothStretch(-9, -9, -9, -9), -1.0f);
    }

    void PipelineCompareView::SetNode(const String& nodeId)
    {
        mNodeId = nodeId;
    }

    void PipelineCompareView::SetImages(const Ref<Bitmap>& input, const Ref<Bitmap>& result)
    {
        auto set = [](Sprite& sprite, Vec2I& size, const Ref<Bitmap>& bitmap)
        {
            size = Vec2I();
            auto display = PipelinePairDraw::DisplayCopy(bitmap);
            if (!display)
                return;

            size = display->GetSize();
            sprite.SetTexture(TextureRef(*display));
        };
        set(*mInputSprite, mInputSize, input);
        set(*mResultSprite, mResultSize, result);
    }

    float PipelineCompareView::GetDividerX() const
    {
        RectF box = layout->GetWorldRect();
        return box.left + box.Width()*PipelinePairLayout::GetDivider(mNodeId);
    }

    void PipelineCompareView::Draw()
    {
        if (!mResEnabledInHierarchy || mIsClipped)
            return;

        DrawLayers();
        RectF box = layout->GetWorldRect();
        box = RectF(box.left + 1, box.top - 1, box.right - 1, box.bottom + 1);
        float divider = GetDividerX();

        PipelinePairDraw::DrawChecker(*mChecker, box, box.left, box.right);
        if (mInputSize.x > 0)
            PipelinePairDraw::DrawClipped(*mInputSprite, mInputSize, PipelinePairDraw::FitRect(mInputSize, box), box.left, divider);
        if (mResultSize.x > 0)
            PipelinePairDraw::DrawClipped(*mResultSprite, mResultSize, PipelinePairDraw::FitRect(mResultSize, box), divider, box.right);

        if (!PipelineControls::IsFarView())
            PipelinePairDraw::DrawDivider(box, divider, mHovered || IsPressed());

        CursorAreaEventsListener::OnDrawn();
        DrawTopLayers();
    }

    bool PipelineCompareView::IsUnderPoint(const Vec2F& point)
    {
        return layout->IsPointInside(point);
    }

    void PipelineCompareView::OnCursorPressed(const Input::Cursor& cursor)
    {
        MoveDivider(cursor.position.x);
    }

    void PipelineCompareView::OnCursorStillDown(const Input::Cursor& cursor)
    {
        MoveDivider(cursor.position.x);
    }

    void PipelineCompareView::OnCursorEnter(const Input::Cursor& cursor)
    {
        mHovered = true;
    }

    void PipelineCompareView::OnCursorExit(const Input::Cursor& cursor)
    {
        mHovered = false;
    }

    void PipelineCompareView::MoveDivider(float x)
    {
        RectF box = layout->GetWorldRect();
        if (box.Width() > 0.0f)
            PipelinePairLayout::SetDivider(mNodeId, (x - box.left)/box.Width());
    }

    PipelineIoPair::PipelineIoPair(RefCounter* refCounter):
        Widget(refCounter)
    {
        mInputView = mmake<PipelineImageView>();
        mInputView->name = "input";
        mInputView->SetHint("no image - connect the input");
        AddChild(mInputView);

        mCompare = mmake<PipelineCompareView>();
        mCompare->name = "compare";
        AddChild(mCompare);
        mCompare->SetEnabledForcible(false);
    }

    void PipelineIoPair::Setup(const String& nodeId, const String& nodeType, const Ref<Widget>& result)
    {
        mNodeId = nodeId;
        mNodeType = nodeType;
        mCompare->SetNode(nodeId);
        mResult = result;
        if (mResult)
        {
            mResult->name = "result";
            AddChild(mResult);
        }
        UpdateMode();
    }

    void PipelineIoPair::SetImages(const Ref<Bitmap>& input, const Ref<Bitmap>& result)
    {
        mHasInput = input != nullptr;
        mHasResult = result != nullptr;
        mInputView->SetBitmap(input);
        if (PipelinePairLayout::ShowsCompare(PipelinePairLayout::GetIoView(), mNodeType, mHasInput, mHasResult, mCropOn))
            mCompare->SetImages(input, result);
        UpdateMode();
    }

    void PipelineIoPair::SetCropOn(bool cropOn)
    {
        mCropOn = cropOn;
        UpdateMode();
    }

    float PipelineIoPair::GetMinHeight() const
    {
        return PipelinePairLayout::PairRowHeight(mHasInput || mHasResult);
    }

    bool PipelineIoPair::IsComparing() const
    {
        return mCompare->IsEnabled();
    }

    void PipelineIoPair::UpdateMode()
    {
        bool compare = PipelinePairLayout::ShowsCompare(PipelinePairLayout::GetIoView(), mNodeType, mHasInput, mHasResult, mCropOn);
        mCompare->SetEnabledForcible(compare);
        mInputView->SetEnabledForcible(!compare);
        if (mResult)
            mResult->SetEnabledForcible(!compare);
    }

    void PipelineIoPair::UpdateSelfTransform()
    {
        Widget::UpdateSelfTransform();

        float width = layout->GetWidth();
        float pane = PipelinePairLayout::PaneWidth(width);
        *mInputView->layout = WidgetLayout(Vec2F(0, 0), Vec2F(0, 1), Vec2F(0, 0), Vec2F(pane, 0));
        if (mResult)
            *mResult->layout = WidgetLayout(Vec2F(0, 0), Vec2F(0, 1), Vec2F(pane + PipelinePairLayout::paneGap, 0), Vec2F(width, 0));
        *mCompare->layout = WidgetLayout::BothStretch(0, 0, 0, 0);
    }
}
// --- META ---

DECLARE_CLASS(Editor::PipelineCompareView, Editor__PipelineCompareView);

DECLARE_CLASS(Editor::PipelineIoPair, Editor__PipelineIoPair);
// --- END META ---
