#include "o2Editor/stdafx.h"
#include "PipelinePaintEditor.h"

#include "o2/Application/Input.h"
#include "o2/Render/Render.h"
#include "o2/Render/Sprite.h"
#include "o2/Render/Text.h"
#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/WidgetLayer.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Scene/UI/Widgets/Button.h"
#include "o2/Scene/UI/Widgets/Toggle.h"
#include "o2/Utils/Bitmap/Bitmap.h"
#include "o2/Utils/Bitmap/PngFormat.h"
#include "o2Editor/Dialogs/ColorPickerDlg.h"
#include "o2Editor/Pipeline/PipelineImageOps.h"
#include "o2Editor/Pipeline/PipelineUtils.h"
#include "o2Editor/Pipeline/PipelineValue.h"
#include "o2Editor/Windows/PipelineWindow/PipelineControls.h"
#include "o2Editor/Windows/PipelineWindow/PipelinePairViews.h"

#include <cstring>

namespace Editor
{
    const float PipelinePaintEditor::toolbarHeight = 22.0f;
    static const float toolbarBelowGap = 6.0f;      // Between the stage and the tool row under it
    static const float toolbarEndGap = 8.0f;        // Before the end widget on the tool line
    static const float toolbarEndHeight = 20.0f;    // Height of the end widget
    static const float compactSliderWidth = 76.0f;  // Narrowest slider sharing the line with the end widget
    static const float paletteHeight = 20.0f;
    static const float maxCanvasSide = 1536.0f;

    PipelinePaintEditor::PipelinePaintEditor(RefCounter* refCounter):
        Widget(refCounter)
    {
        layout->minHeight = 180;

        mBackgroundSprite = mmake<Sprite>();
        mFrameSprite = mmake<Sprite>("ui/UI4_Editbox_regular.png");
        mFrameSprite->mode = SpriteMode::Sliced;
        mSprite = mmake<Sprite>();

        mHintText = mmake<Text>("stdFont.ttf");
        mHintText->horAlign = HorAlign::Middle;
        mHintText->verAlign = VerAlign::Middle;
        mHintText->color = PipelineControls::dimTextColor;
        mHintText->wordWrap = true;

        mRegionFrame = mmake<FrameHandles>();
        mRegionFrame->SetPivotEnabled(false);
        mRegionFrame->SetRotationEnabled(false);
        mRegionFrame->onTransformed = THIS_FUNC(OnRegionTransformed);
        mRegionFrame->onChangeCompleted = THIS_FUNC(OnRegionCompleted);
        mRegionFrame->onPressed = [this]() { mRegionDragging = true; };
        mRegionFrame->onReleased = [this]() { mRegionDragging = false; };

        mBadgeText = mmake<Text>("stdFont.ttf");
        mBadgeText->horAlign = HorAlign::Middle;
        mBadgeText->verAlign = VerAlign::Middle;
        mBadgeText->color = Color4(255, 255, 255, 255);
        mBadgeText->SetHeight(9);

        BuildToolbar();

        WeakRef<PipelinePaintEditor> weakThis(this);
        mRemoveButton = PipelineControls::MakeIconButton("ui/UI4_small_trash_icon.png", PipelineControls::textColor, Color4(255, 255, 255, 230));
        mRemoveButton->name = "remove region";
        mRemoveButton->enabled = false;
        mRemoveButton->onClick = [weakThis]()
        {
            auto self = weakThis.Lock();
            if (!self)
                return;

            if (self->mOptionalRegion)
                self->RemoveOptionalRegion();
            else if (self->onRegionRemoved)
                self->onRegionRemoved();
        };
        AddChild(mRemoveButton);
    }

    void PipelinePaintEditor::Init(const Ref<PipelineNode>& node, bool withRegion, const String& regionKey /*= "roi"*/,
                                   bool optionalRegion /*= false*/)
    {
        mNode = node;
        mWithRegion = withRegion;
        mRegionKey = regionKey;
        mOptionalRegion = withRegion && optionalRegion;
        mRegionToggle->enabled = withRegion;
        mSelfDrawing = "\x01";
        RefreshFromConfig();
    }

    String PipelinePaintEditor::GetTool() const
    {
        if (!mNode)
            return "brush";

        // Without drawing a stored tool is ignored: the region boxes are always editable
        if (mNoDrawing)
            return mWithRegion ? "roi" : "brush";

        String raw = mNode->GetConfigString("drawTool", "");
        if (raw == "eraser") return "eraser";
        if (raw == "brush") return "brush";
        if (mOptionalRegion)
            return raw == "roi" && HasRegion() ? "roi" : "brush";
        return mWithRegion ? "roi" : "brush";
    }

    void PipelinePaintEditor::SetTool(const String& tool)
    {
        if (!mNode)
            return;

        bool create = tool == "roi" && mOptionalRegion && !HasRegion();
        mNode->SetConfigString("drawTool", tool);
        if (create)
        {
            auto& region = mNode->config[mRegionKey.Data()];
            region.SetObject();
            region["x"] = 0.25f; region["y"] = 0.25f; region["w"] = 0.5f; region["h"] = 0.5f;
        }

        UpdateToolbar();
        // A new region may rebuild the card, so the tool is stored first
        if (onConfigChanged)
        {
            onConfigChanged("drawTool", true);
            if (create)
                onConfigChanged(mRegionKey, true);
        }
    }

    float PipelinePaintEditor::GetBrushSize() const
    {
        return mNode ? Math::Clamp(mNode->GetConfigNumber("brushSize", 14), 1.0f, 80.0f) : 14.0f;
    }

    float PipelinePaintEditor::GetBrushOpacity() const
    {
        return mNode ? Math::Clamp(mNode->GetConfigNumber("brushOpacity", 1), 0.0f, 1.0f) : 1.0f;
    }

    Color4 PipelinePaintEditor::GetBrushColor() const
    {
        Color4 color(255, 59, 48, 255);
        if (mNode)
            PipelineUtils::ParseHexColor(mNode->GetConfigString("brushColor", "#ff3b30"), color);
        color.a = 255;
        return color;
    }

    float PipelinePaintEditor::GetMinHeight() const
    {
        return GetMinHeightForWidth(layout->GetWidth());
    }

    float PipelinePaintEditor::GetMinHeightForWidth(float width) const
    {
        return BarsHeight(width) + 150;
    }

    float PipelinePaintEditor::BarsHeight(float width) const
    {
        if (mNoDrawing)
            return 0.0f;

        bool inlineEnd = mToolbarBelow && mToolbarEnd && IsToolbarInline(width);
        float toolsWidth = inlineEnd ? width - mToolbarEndWidth - toolbarEndGap : width;
        float toolbar = mToolbar ? Math::Max(toolbarHeight, mToolbar->GetHeightForWidth(toolsWidth)) : toolbarHeight;
        float palette = mPaletteOpen && mPaletteRow ? Math::Max(paletteHeight, mPaletteRow->GetHeightForWidth(width)) + 2 : 0.0f;
        if (!mToolbarBelow)
            return toolbar + 4 + palette;

        float endLine = mToolbarEnd && !inlineEnd ? 2 + toolbarEndHeight : 0.0f;
        return toolbarBelowGap + toolbar + palette + endLine;
    }

    Vec2I PipelinePaintEditor::CanvasResolution(const Vec2F& stage)
    {
        Vec2F res = stage * 2.0f;
        float longer = Math::Max(res.x, res.y);
        if (longer > maxCanvasSide)
            res *= maxCanvasSide / longer;

        return Vec2I(Math::Max(1, (int)Math::Round(res.x)), Math::Max(1, (int)Math::Round(res.y)));
    }

    void PipelinePaintEditor::UpdateSelfTransform()
    {
        Widget::UpdateSelfTransform();

        float width = layout->GetWidth();
        if (mToolbar && mPaletteRow && !mToolbarBelow && !mNoDrawing)
        {
            float toolbar = Math::Max(toolbarHeight, mToolbar->GetHeightForWidth(width));
            float palette = Math::Max(paletteHeight, mPaletteRow->GetHeightForWidth(width));
            *mToolbar->layout = WidgetLayout::HorStretch(VerAlign::Top, 0, 0, toolbar, 0);
            *mPaletteRow->layout = WidgetLayout::HorStretch(VerAlign::Top, 0, 0, palette, toolbar + 2);
        }
        else if (mToolbar && mPaletteRow && !mNoDrawing)
        {
            // Under the stage, top down: the tools (with the end widget at their right when it fits), the palette, the end
            // widget's own line
            bool inlineEnd = mToolbarEnd && IsToolbarInline(width);
            float toolsWidth = inlineEnd ? width - mToolbarEndWidth - toolbarEndGap : width;
            float toolbar = Math::Max(toolbarHeight, mToolbar->GetHeightForWidth(toolsWidth));
            float palette = Math::Max(paletteHeight, mPaletteRow->GetHeightForWidth(width));
            float top = BarsHeight(width) - toolbarBelowGap;
            *mToolbar->layout = WidgetLayout(Vec2F(0, 0), Vec2F(1, 0), Vec2F(0, top - toolbar), Vec2F(inlineEnd ? -mToolbarEndWidth - toolbarEndGap : 0.0f, top));
            *mPaletteRow->layout = WidgetLayout(Vec2F(0, 0), Vec2F(1, 0), Vec2F(0, top - toolbar - 2 - palette), Vec2F(0, top - toolbar - 2));
            if (mToolbarEnd)
            {
                float endTop = inlineEnd ? top - (toolbarHeight - toolbarEndHeight)*0.5f : toolbarEndHeight;
                *mToolbarEnd->layout = WidgetLayout(Vec2F(1, 0), Vec2F(1, 0), Vec2F(-mToolbarEndWidth, endTop - toolbarEndHeight), Vec2F(0, endTop));
            }
        }

        if (mBeside)
        {
            float left = PipelinePairLayout::PaneWidth(width) + PipelinePairLayout::paneGap;
            if (mToolbarBelow)
                *mBeside->layout = WidgetLayout(Vec2F(0, 0), Vec2F(0, 1), Vec2F(left, BarsHeight(width)), Vec2F(width, 0));
            else
                *mBeside->layout = WidgetLayout(Vec2F(0, 0), Vec2F(0, 1), Vec2F(left, 0), Vec2F(width, -BarsHeight(width)));
        }
    }

    void PipelinePaintEditor::SetDrawingEnabled(bool enabled)
    {
        mNoDrawing = !enabled;
        mToolbar->SetEnabledForcible(enabled);
        mPaletteRow->SetEnabledForcible(enabled && mPaletteOpen);
        SetLayoutDirty();
    }

    void PipelinePaintEditor::SetToolbarBelow(const Ref<Widget>& end, float endWidth)
    {
        mToolbarBelow = true;
        if (mToolbarEnd)
            RemoveChild(mToolbarEnd);

        mToolbarEnd = end;
        mToolbarEndWidth = endWidth;
        if (mToolbarEnd)
            AddChild(mToolbarEnd);

        // The sliders give up their value text to share the line with the switches
        for (auto& slider : { mSizeSlider, mOpacitySlider })
        {
            slider->SetCompact(true);
            slider->layout->minWidth = compactSliderWidth;
        }
    }

    void PipelinePaintEditor::SetBeside(const Ref<Widget>& widget)
    {
        if (mBeside)
            RemoveChild(mBeside);

        mBeside = widget;
        if (mBeside)
            AddChild(mBeside);
    }

    void PipelinePaintEditor::SetCompare(const Ref<Bitmap>& result, const String& nodeId)
    {
        mCompareNode = nodeId;
        mCompareSize = Vec2I();
        auto display = PipelinePairDraw::DisplayCopy(result);
        if (display)
        {
            if (!mCompareSprite)
            {
                mCompareSprite = mmake<Sprite>();
                mCompareChecker = PipelinePairDraw::MakeChecker();
            }
            mCompareSprite->SetTexture(TextureRef(*display));
            mCompareSize = display->GetSize();
        }

        // The stage takes the whole row while it shows both sides
        if (mBeside)
            mBeside->SetEnabledForcible(!IsComparing());
    }

    float PipelinePaintEditor::GetDividerX() const
    {
        RectF stage = GetStageRect();
        return stage.left + stage.Width()*PipelinePairLayout::GetDivider(mCompareNode);
    }

    bool PipelinePaintEditor::IsOnDivider(const Vec2F& point) const
    {
        RectF stage = GetStageRect();
        float grab = PipelinePairLayout::dividerGrab*PipelinePairDraw::BadgeScale(mPixelSize);
        return IsComparing() && Math::Abs(point.x - GetDividerX()) <= grab && point.y <= stage.top && point.y >= stage.bottom;
    }

    RectF PipelinePaintEditor::GetAreaRect() const
    {
        RectF rect = layout->GetWorldRect();
        if (mToolbarBelow)
            rect.bottom += BarsHeight(layout->GetWidth());
        else
            rect.top -= BarsHeight(layout->GetWidth());
        if (mBeside && !IsComparing())
            rect.right = rect.left + PipelinePairLayout::PaneWidth(rect.Width());
        return rect;
    }

    RectF PipelinePaintEditor::GetStageRect() const
    {
        RectF area = GetAreaRect();
        // The extract source keeps room round the image for the handles of its box
        float pad = mWithRegion && !mOptionalRegion ? 18.0f : 0.0f;
        RectF avail(area.left + pad, area.top - pad, area.right - pad, area.bottom + pad);
        if (avail.Width() < 2 || avail.Height() < 2)
            return avail;

        if (mBackground && mBackground->GetSize().x > 0 && mBackground->GetSize().y > 0)
        {
            float ar = (float)mBackground->GetSize().x / mBackground->GetSize().y;
            float w = avail.Width(), h = avail.Height();
            if (w / h > ar) w = Math::Round(h * ar);
            else h = Math::Round(w / ar);
            Vec2F center = avail.Center();
            return RectF(center.x - w * 0.5f, center.y + h * 0.5f, center.x + w * 0.5f, center.y - h * 0.5f);
        }

        return avail;
    }

    Vec2F PipelinePaintEditor::ToImage(const Vec2F& canvasPoint) const
    {
        RectF stage = GetStageRect();
        float w = Math::Max(1.0f, stage.Width()), h = Math::Max(1.0f, stage.Height());
        return Vec2F((canvasPoint.x - stage.left) / w * mResolution.x, (stage.top - canvasPoint.y) / h * mResolution.y);
    }

    bool PipelinePaintEditor::IsUnderPoint(const Vec2F& point)
    {
        if (IsOnDivider(point))
            return true;

        // With the region tool the selected box belongs to its handles; a click on another box picks that part
        if (GetTool() == "roi")
            return mWithRegion && OtherRegionAt(point) != nullptr;

        return GetStageRect().IsInside(point);
    }

    void PipelinePaintEditor::SetBackground(const Ref<Bitmap>& bitmap)
    {
        bool changed = (bitmap != nullptr) != (mBackground != nullptr) ||
            (bitmap && mBackground && bitmap->GetSize() != mBackground->GetSize());
        mBackground = bitmap;
        if (bitmap)
        {
            mBackgroundTexture = TextureRef(*bitmap);
            mBackgroundSprite->SetTexture(mBackgroundTexture);
            mBackgroundSprite->SetTextureSrcRect(RectI(Vec2I(), bitmap->GetSize()));
            SetResolution(bitmap->GetSize(), true);
        }
        else if (changed)
            mCommittedArea = Vec2F();

        if (changed && !bitmap)
            mResizeTimer = 0.0f;
    }

    void PipelinePaintEditor::ClearBitmap(Bitmap& bitmap)
    {
        std::memset(bitmap.GetData(), 0, bitmap.GetSize().x * bitmap.GetSize().y * 4);
    }

    void PipelinePaintEditor::EnsureBuffers()
    {
        if (mResolution.x < 1 || mResolution.y < 1)
            return;

        if (!mBase || mBase->GetSize() != mResolution)
            mBase = PipelineImageOps::Blank(mResolution.x, mResolution.y);
        if (!mStroke || mStroke->GetSize() != mResolution)
            mStroke = PipelineImageOps::Blank(mResolution.x, mResolution.y);
        if (!mComposed || mComposed->GetSize() != mResolution)
        {
            mComposed = PipelineImageOps::Blank(mResolution.x, mResolution.y);
            mTexture = TextureRef(*mComposed);
            mSprite->SetTexture(mTexture);
            mSprite->SetTextureSrcRect(RectI(Vec2I(), mResolution));
        }
    }

    void PipelinePaintEditor::SetResolution(const Vec2I& resolution, bool rescale)
    {
        Vec2I res(Math::Max(1, resolution.x), Math::Max(1, resolution.y));
        if (res == mResolution && mBase)
            return;

        Ref<Bitmap> previous = mBase;
        mResolution = res;
        mBase = nullptr;
        EnsureBuffers();

        if (rescale && previous && previous->GetSize().x > 0 && previous->GetSize().y > 0)
        {
            auto scaled = PipelineImageOps::Resize(*previous, res);
            std::memcpy(mBase->GetData(), scaled->GetData(), res.x * res.y * 4);
        }
        else
            LoadDrawing(mNode ? mNode->GetConfigString("drawing", "") : String());

        ComposePreview();
    }

    void PipelinePaintEditor::LoadDrawing(const String& dataUrl)
    {
        EnsureBuffers();
        if (!mBase)
            return;

        ClearBitmap(*mBase);
        if (dataUrl.IsEmpty())
        {
            ComposePreview();
            return;
        }

        auto decoded = DecodeImageBytes(PipelineUtils::DataUrlToBytes(dataUrl));
        if (!decoded)
        {
            ComposePreview();
            return;
        }

        // The strokes keep their own pixels; only a background dictates the size, as the node scales the overlay to it
        if (!mBackground && decoded->GetSize() != mResolution)
        {
            mResolution = decoded->GetSize();
            mBase = nullptr;
            EnsureBuffers();
        }

        auto fitted = PipelineImageOps::Resize(*decoded, mResolution);
        std::memcpy(mBase->GetData(), fitted->GetData(), mResolution.x * mResolution.y * 4);
        ComposePreview();
    }

    void PipelinePaintEditor::RefreshFromConfig()
    {
        if (!mNode)
            return;

        String drawing = mNode->GetConfigString("drawing", "");
        if (drawing != mSelfDrawing)
        {
            mSelfDrawing = drawing;
            mUndo.Clear();
            mRedo.Clear();
            if (mBase)
                LoadDrawing(drawing);
        }
        UpdateToolbar();
    }

    void PipelinePaintEditor::StampDisc(const Vec2F& center, float radius)
    {
        float r = Math::Max(0.5f, radius);
        int x0 = Math::Clamp((int)Math::Floor(center.x - r - 1), 0, mResolution.x - 1);
        int x1 = Math::Clamp((int)Math::Ceil(center.x + r + 1), 0, mResolution.x - 1);
        int y0 = Math::Clamp((int)Math::Floor(center.y - r - 1), 0, mResolution.y - 1);
        int y1 = Math::Clamp((int)Math::Ceil(center.y + r + 1), 0, mResolution.y - 1);
        Color4 color = GetBrushColor();

        for (int y = y0; y <= y1; y++)
        {
            for (int x = x0; x <= x1; x++)
            {
                float d = (Vec2F(x + 0.5f, y + 0.5f) - center).Length();
                float coverage = Math::Clamp(r + 0.5f - d, 0.0f, 1.0f);
                if (coverage <= 0.0f)
                    continue;

                UInt8* p = PipelineImageOps::Pixel(*mStroke, x, y);
                UInt8 a = (UInt8)Math::Round(coverage * 255.0f);
                if (a > p[3])
                {
                    p[0] = (UInt8)color.r; p[1] = (UInt8)color.g; p[2] = (UInt8)color.b; p[3] = a;
                }
            }
        }

        if (mDirtyMaxX <= mDirtyMinX)
        {
            mDirtyMinX = x0; mDirtyMinY = y0; mDirtyMaxX = x1 + 1; mDirtyMaxY = y1 + 1;
        }
        else
        {
            mDirtyMinX = Math::Min(mDirtyMinX, x0);
            mDirtyMinY = Math::Min(mDirtyMinY, y0);
            mDirtyMaxX = Math::Max(mDirtyMaxX, x1 + 1);
            mDirtyMaxY = Math::Max(mDirtyMaxY, y1 + 1);
        }
    }

    void PipelinePaintEditor::PaintSegment(const Vec2F& from, const Vec2F& to)
    {
        RectF stage = GetStageRect();
        float k = mResolution.x / Math::Max(1.0f, stage.Width());
        float radius = GetBrushSize() * k * 0.5f;
        float length = (to - from).Length();
        float step = Math::Max(0.5f, radius * 0.35f);
        int count = Math::Max(1, (int)Math::Ceil(length / step));
        for (int i = 0; i <= count; i++)
            StampDisc(Math::Lerp(from, to, (float)i / count), radius);
    }

    void PipelinePaintEditor::ComposePreview()
    {
        if (!mBase || !mComposed)
            return;

        std::memcpy(mComposed->GetData(), mBase->GetData(), mResolution.x * mResolution.y * 4);

        if (mPainting && mStroke && mDirtyMaxX > mDirtyMinX)
        {
            bool eraser = GetTool() == "eraser";
            float opacity = GetBrushOpacity();
            int x0 = Math::Clamp(mDirtyMinX, 0, mResolution.x), x1 = Math::Clamp(mDirtyMaxX, 0, mResolution.x);
            int y0 = Math::Clamp(mDirtyMinY, 0, mResolution.y), y1 = Math::Clamp(mDirtyMaxY, 0, mResolution.y);
            for (int y = y0; y < y1; y++)
            {
                for (int x = x0; x < x1; x++)
                {
                    const UInt8* s = PipelineImageOps::Pixel(*mStroke, x, y);
                    if (s[3] == 0)
                        continue;

                    UInt8* d = PipelineImageOps::Pixel(*mComposed, x, y);
                    float sa = s[3] / 255.0f * opacity;
                    float da = d[3] / 255.0f;
                    if (eraser)
                    {
                        d[3] = (UInt8)Math::Round(da * (1.0f - sa) * 255.0f);
                        continue;
                    }

                    float outA = sa + da * (1.0f - sa);
                    if (outA <= 0.0001f)
                        continue;

                    for (int c = 0; c < 3; c++)
                        d[c] = (UInt8)Math::Round((s[c] * sa + d[c] * da * (1.0f - sa)) / outA);
                    d[3] = (UInt8)Math::Round(outA * 255.0f);
                }
            }
        }

        mTextureDirty = true;
    }

    void PipelinePaintEditor::CommitStroke()
    {
        if (!mPainting)
            return;

        mPainting = false;
        String previous = mNode ? mNode->GetConfigString("drawing", "") : String();

        // The preview already holds the stroke blended at the brush alpha
        bool eraser = GetTool() == "eraser";
        mPainting = true;
        ComposePreview();
        mPainting = false;
        std::memcpy(mBase->GetData(), mComposed->GetData(), mResolution.x * mResolution.y * 4);
        ClearBitmap(*mStroke);
        mDirtyMinX = mDirtyMinY = mDirtyMaxX = mDirtyMaxY = 0;
        (void)eraser;

        PushHistory(previous);
        WriteDrawing(EncodeBase());
        ComposePreview();
    }

    String PipelinePaintEditor::EncodeBase() const
    {
        if (!mBase)
            return "";

        String png;
        if (!SavePngImageToMemory(mBase.Get(), png))
            return "";

        return PipelineUtils::BytesToDataUrl(png, "image/png");
    }

    void PipelinePaintEditor::WriteDrawing(const String& dataUrl)
    {
        if (!mNode)
            return;

        mSelfDrawing = dataUrl;
        mNode->SetConfigString("drawing", dataUrl);
        mNode->SetConfigNumber("dw", (float)mResolution.x);
        mNode->SetConfigNumber("dh", (float)mResolution.y);
        if (onConfigChanged)
            onConfigChanged("drawing", true);
        UpdateToolbar();
    }

    void PipelinePaintEditor::PushHistory(const String& previous)
    {
        mUndo.Add(previous);
        if (mUndo.Count() > maxHistory)
            mUndo.RemoveAt(0);
        mRedo.Clear();
    }

    void PipelinePaintEditor::OnUndo()
    {
        if (mUndo.IsEmpty() || !mNode)
            return;

        mRedo.Add(mNode->GetConfigString("drawing", ""));
        String previous = mUndo.Last();
        mUndo.RemoveAt(mUndo.Count() - 1);
        LoadDrawing(previous);
        WriteDrawing(previous);
    }

    void PipelinePaintEditor::OnRedo()
    {
        if (mRedo.IsEmpty() || !mNode)
            return;

        mUndo.Add(mNode->GetConfigString("drawing", ""));
        String next = mRedo.Last();
        mRedo.RemoveAt(mRedo.Count() - 1);
        LoadDrawing(next);
        WriteDrawing(next);
    }

    void PipelinePaintEditor::OnClear()
    {
        if (!mNode || mNode->GetConfigString("drawing", "").IsEmpty())
            return;

        PushHistory(mNode->GetConfigString("drawing", ""));
        if (mBase) ClearBitmap(*mBase);
        if (mStroke) ClearBitmap(*mStroke);
        ComposePreview();
        WriteDrawing("");
    }

    void PipelinePaintEditor::Update(float dt)
    {
        Widget::Update(dt);

        // The remove tab follows the selected box, at its top-right corner; an optional region offers it with any tool
        bool showRemove = mOptionalRegion ? HasRegion() && !PipelineControls::IsFarView() : mWithRegion && mRegionRemovable && GetTool() == "roi";
        const Basis& b = mRegionFrame->GetCurrentBasis();
        RectF frame(b.origin, b.origin + b.xv + b.yv);
        RectF box(Math::Min(frame.left, frame.right), Math::Max(frame.top, frame.bottom), Math::Max(frame.left, frame.right), Math::Min(frame.top, frame.bottom));
        RectF want(box.right - 18.0f, box.top, box.right, box.top - 18.0f);

        // Without the region tool the divider stays on top: a tab over its strip gives way
        float grab = PipelinePairLayout::dividerGrab*PipelinePairDraw::BadgeScale(mPixelSize);
        if (showRemove && IsComparing() && GetTool() != "roi" && want.right >= GetDividerX() - grab && want.left <= GetDividerX() + grab)
            showRemove = false;

        if (mRemoveButton->IsEnabled() != showRemove)
            mRemoveButton->enabled = showRemove;
        if (showRemove)
        {
            if (want.left != mRemovePlaced.left || want.top != mRemovePlaced.top || want.right != mRemovePlaced.right || want.bottom != mRemovePlaced.bottom)
            {
                RectF parent = layout->GetWorldRect();
                *mRemoveButton->layout = WidgetLayout(Vec2F(0, 0), Vec2F(0, 0), Vec2F(want.left - parent.left, want.bottom - parent.bottom),
                                                      Vec2F(want.right - parent.left, want.top - parent.bottom));
                mRemovePlaced = want;
            }
        }

        if (!mBackground)
        {
            RectF stage = GetStageRect();
            Vec2F area(Math::Round(stage.Width()), Math::Round(stage.Height()));
            if (area.x >= 1 && area.y >= 1 && area != mCommittedArea)
            {
                if (mResizeTimer < 0.0f)
                    mResizeTimer = mCommittedArea.x < 1 ? 0.0f : 0.15f;
                else
                    mResizeTimer -= dt;

                if (mResizeTimer <= 0.0f)
                {
                    mResizeTimer = -1.0f;
                    bool first = mCommittedArea.x < 1;
                    mCommittedArea = area;
                    // A stored drawing keeps its pixels: the first commit loads it at its own size, later ones leave it alone
                    bool blank = !mNode || mNode->GetConfigString("drawing", "").IsEmpty();
                    if (first || blank)
                        SetResolution(CanvasResolution(area), false);
                }
            }
        }
    }

    void PipelinePaintEditor::Draw()
    {
        if (!mResEnabledInHierarchy || mIsClipped)
            return;

        bool farView = PipelineControls::IsFarView();
        mPixelSize = PipelinePairDraw::PixelSize();
        DrawLayers();
        Widget::OnDrawn();

        RectF stage = GetStageRect();
        if (stage.Width() < 2 || stage.Height() < 2)
        {
            if (!farView)
            {
                DrawInheritedDepthChildren();
                DrawInternalChildren();
            }
            DrawTopLayers();
            return;
        }

        RectF area = GetAreaRect();
        mFrameSprite->rect = RectF(area.left - 9.0f, area.top + 9.0f, area.right + 9.0f, area.bottom - 9.0f);
        mFrameSprite->Draw();

        // Comparing, the input and the drawing end at the divider and the result takes the rest of the stage
        float cut = IsComparing() ? GetDividerX() : stage.right;
        o2Render.DrawFilledPolygon({ stage.LeftBottom(), Vec2F(stage.left, stage.top), Vec2F(cut, stage.top), Vec2F(cut, stage.bottom) }, Color4(255, 255, 255, 255));
        if (mBackground)
            PipelinePairDraw::DrawClipped(*mBackgroundSprite, mBackground->GetSize(), stage, stage.left, cut);

        if (mComposed && mTexture && !mNoDrawing)
        {
            if (mTextureDirty)
            {
                mTexture->SetData(*mComposed);
                mTextureDirty = false;
            }
            PipelinePairDraw::DrawClipped(*mSprite, mResolution, stage, stage.left, cut);
        }

        if (IsComparing())
        {
            PipelinePairDraw::DrawChecker(*mCompareChecker, stage, cut, stage.right);
            PipelinePairDraw::DrawClipped(*mCompareSprite, mCompareSize, PipelinePairDraw::FitRect(mCompareSize, stage), cut, stage.right);
        }

        o2Render.DrawAARectFrame(stage, Color4(96, 125, 139, 140), 1.0f);

        CursorAreaEventsListener::OnDrawn();

        // Comparing, the box spans both halves and marks the same place on the result. With the region tool it lies above
        // the divider, so its handles on the divider stay usable; with another tool the divider is on top
        bool regionTool = GetTool() == "roi";
        auto drawDivider = [&]()
        {
            if (IsComparing() && !farView)
                PipelinePairDraw::DrawDivider(stage, cut, mDividerDragging || (mHovered && IsOnDivider(mHoverPoint)));
        };
        if (regionTool)
            drawDivider();

        if (mWithRegion && HasRegion() && !farView)
        {
            if (!mRegionDragging)
                SyncRegionFromConfig();

            const Basis& b = mRegionFrame->GetCurrentBasis();
            RectF frame(b.origin, b.origin + b.xv + b.yv);

            // Everything outside the box is not sent to the model, or not changed by it: shade it
            RectF box(Math::Clamp(Math::Min(frame.left, frame.right), stage.left, stage.right),
                      Math::Clamp(Math::Max(frame.top, frame.bottom), stage.bottom, stage.top),
                      Math::Clamp(Math::Max(frame.left, frame.right), stage.left, stage.right),
                      Math::Clamp(Math::Min(frame.top, frame.bottom), stage.bottom, stage.top));
            Color4 shade(0, 0, 0, 128);
            auto fill = [&](const RectF& r)
            {
                if (r.Width() > 0.0f && r.Height() > 0.0f)
                    o2Render.DrawFilledPolygon({ r.LeftBottom(), Vec2F(r.left, r.top), r.RightTop(), Vec2F(r.right, r.bottom) }, shade);
            };
            fill(RectF(stage.left, stage.top, stage.right, box.top));
            fill(RectF(stage.left, box.bottom, stage.right, stage.bottom));
            fill(RectF(stage.left, box.top, box.left, box.bottom));
            fill(RectF(box.right, box.top, stage.right, box.bottom));
            o2Render.DrawAARectFrame(frame, GetTool() == "roi" ? Color4(0, 150, 136, 255) : Color4(0, 150, 136, 150), 1.5f);

            // The other parts are outlined and numbered; only the selected one has handles
            for (auto& other : mOtherRegions)
            {
                RectF r = BoxRect(other.x, other.y, other.w, other.h, stage);
                o2Render.DrawAARectFrame(r, Color4(255, 255, 255, 190), 1.0f);
                DrawBadge(r, other.index, Color4(96, 125, 139, 230));
            }
            if (mSelectedIndex > 0)
                DrawBadge(box, mSelectedIndex, Color4(0, 150, 136, 255));

            if (regionTool)
                mRegionFrame->Draw();
        }

        bool brushOff = IsComparing() && (IsOnDivider(mHoverPoint) || mHoverPoint.x > cut);
        if (mHovered && !farView && GetTool() != "roi" && !brushOff)
        {
            bool eraser = GetTool() == "eraser";
            o2Render.DrawAACircle(mHoverPoint, GetBrushSize() * 0.5f, eraser ? Color4(96, 125, 139, 255) : GetBrushColor(), 28, 1.0f);
        }

        // The toolbar and the remove tab are drawn over the stage
        if (!farView)
        {
            DrawInheritedDepthChildren();
            DrawInternalChildren();
        }

        if (!regionTool)
            drawDivider();

        DrawTopLayers();
    }

    void PipelinePaintEditor::OnCursorPressed(const Input::Cursor& cursor)
    {
        if (IsComparing())
        {
            // Only the strip on the divider drags it; the result side takes no strokes
            mDividerDragging = IsOnDivider(cursor.position);
            if (mDividerDragging || cursor.position.x > GetDividerX())
                return;
        }

        if (GetTool() == "roi")
        {
            if (auto box = OtherRegionAt(cursor.position))
                if (onRegionPicked) onRegionPicked(box->id);
            return;
        }

        if (!mBase)
            return;

        mPainting = true;
        mDirtyMinX = mDirtyMinY = mDirtyMaxX = mDirtyMaxY = 0;
        ClearBitmap(*mStroke);
        mLastPoint = ToImage(cursor.position);
        PaintSegment(mLastPoint, mLastPoint);
        ComposePreview();
    }

    void PipelinePaintEditor::OnCursorStillDown(const Input::Cursor& cursor)
    {
        mHoverPoint = cursor.position;
        if (mDividerDragging)
        {
            RectF stage = GetStageRect();
            if (stage.Width() > 0.0f)
                PipelinePairLayout::SetDivider(mCompareNode, (cursor.position.x - stage.left)/stage.Width());
            return;
        }

        if (!mPainting)
            return;

        Vec2F point = ToImage(cursor.position);
        if ((point - mLastPoint).Length() < 0.25f)
            return;

        PaintSegment(mLastPoint, point);
        mLastPoint = point;
        ComposePreview();
    }

    void PipelinePaintEditor::OnCursorReleased(const Input::Cursor& cursor)
    {
        mDividerDragging = false;
        CommitStroke();
    }

    void PipelinePaintEditor::OnCursorPressBreak(const Input::Cursor& cursor)
    {
        CommitStroke();
    }

    void PipelinePaintEditor::OnCursorMoved(const Input::Cursor& cursor)
    {
        mHoverPoint = cursor.position;
    }

    void PipelinePaintEditor::OnCursorEnter(const Input::Cursor& cursor)
    {
        mHovered = true;
        mHoverPoint = cursor.position;
    }

    void PipelinePaintEditor::OnCursorExit(const Input::Cursor& cursor)
    {
        mHovered = false;
    }
}
// --- META ---

DECLARE_CLASS(Editor::PipelinePaintEditor, Editor__PipelinePaintEditor);
// --- END META ---
