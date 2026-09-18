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

#include <cstring>

namespace Editor
{
    const float PipelinePaintEditor::toolbarHeight = 22.0f;
    static const float paletteHeight = 20.0f;
    static const float sliderMinWidth = 170.0f;
    static const float maxCanvasSide = 1536.0f;

    static const Vector<String> drawPalette = {
        "#ff3b30", "#ff9500", "#ffcc00", "#34c759", "#00c7be", "#007aff", "#5856d6", "#af52de", "#ffffff", "#8e8e93", "#000000"
    };

    PipelinePaintEditor::PipelinePaintEditor(RefCounter* refCounter):
        Widget(refCounter)
    {
        layout->minHeight = 180;

        mBackgroundSprite = mmake<Sprite>();
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
            if (auto self = weakThis.Lock())
                if (self->onRegionRemoved) self->onRegionRemoved();
        };
        AddChild(mRemoveButton);
    }

    void PipelinePaintEditor::SetRegions(const Vector<RegionBox>& others, int selectedIndex, bool removable)
    {
        mOtherRegions = others;
        mSelectedIndex = selectedIndex;
        mRegionRemovable = removable;
    }

    RectF PipelinePaintEditor::BoxRect(float x, float y, float w, float h, const RectF& stage) const
    {
        float left = stage.left + stage.Width() * x;
        float top = stage.top - stage.Height() * y;
        return RectF(left, top, left + stage.Width() * w, top - stage.Height() * h);
    }

    const PipelinePaintEditor::RegionBox* PipelinePaintEditor::OtherRegionAt(const Vec2F& point) const
    {
        RectF stage = GetStageRect();
        for (int i = mOtherRegions.Count() - 1; i >= 0; i--)
        {
            auto& box = mOtherRegions[i];
            if (BoxRect(box.x, box.y, box.w, box.h, stage).IsInside(point))
                return &mOtherRegions[i];
        }
        return nullptr;
    }

    void PipelinePaintEditor::DrawBadge(const RectF& rect, int index, const Color4& color)
    {
        RectF badge(rect.left, rect.top, rect.left + 14.0f, rect.top - 12.0f);
        o2Render.DrawFilledPolygon({ badge.LeftBottom(), Vec2F(badge.left, badge.top), badge.RightTop(), Vec2F(badge.right, badge.bottom) }, color);
        mBadgeText->text = (String)index;
        mBadgeText->rect = badge;
        mBadgeText->Draw();
    }

    void PipelinePaintEditor::Init(const Ref<PipelineNode>& node, bool withRegion)
    {
        mNode = node;
        mWithRegion = withRegion;
        mRegionToggle->enabled = withRegion;
        mSelfDrawing = "\x01";
        RefreshFromConfig();
    }

    String PipelinePaintEditor::GetTool() const
    {
        if (!mNode)
            return "brush";

        String raw = mNode->GetConfigString("drawTool", "");
        if (raw == "eraser") return "eraser";
        if (raw == "brush") return "brush";
        return mWithRegion ? "roi" : "brush";
    }

    void PipelinePaintEditor::SetTool(const String& tool)
    {
        if (!mNode)
            return;

        mNode->SetConfigString("drawTool", tool);
        if (onConfigChanged)
            onConfigChanged("drawTool", true);
        UpdateToolbar();
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

    void PipelinePaintEditor::BuildToolbar()
    {
        WeakRef<PipelinePaintEditor> weakThis(this);

        auto toolbar = mmake<PipelineWrapRow>();
        toolbar->name = "toolbar";
        toolbar->spacing = 2;
        toolbar->lineHeight = toolbarHeight;
        *toolbar->layout = WidgetLayout::HorStretch(VerAlign::Top, 0, 0, toolbarHeight, 0);
        AddChild(toolbar);
        mToolbar = toolbar;

        auto makeTool = [&](const String& icon, const String& tool, const String& name)
        {
            auto toggle = o2UI.CreateWidget<Toggle>("pipeline segment");
            toggle->name = name;
            toggle->caption = "";
            auto sprite = mmake<Sprite>(icon);
            sprite->color = PipelineControls::textColor;
            toggle->AddLayer("icon", sprite, Layout::Based(BaseCorner::Center, Vec2F(14, 14)));
            toggle->layout->minWidth = 24;
            toggle->layout->maxWidth = 24;
            toggle->onToggleByUser = [weakThis, tool](bool) { if (auto self = weakThis.Lock()) self->SetTool(tool); };
            toolbar->AddChild(toggle);
            return toggle;
        };

        mBrushToggle = makeTool("ui/pipeline/btn_brush.png", "brush", "brush");
        mEraserToggle = makeTool("ui/pipeline/btn_eraser.png", "eraser", "eraser");
        mRegionToggle = makeTool("ui/pipeline/btn_region.png", "roi", "region");

        mColorButton = o2UI.CreateButton("");
        mColorButton->name = "color";
        mColorButton->layout->minWidth = 24;
        mColorButton->layout->maxWidth = 24;
        mColorButton->onClick = [weakThis]() { if (auto self = weakThis.Lock()) self->TogglePalette(); };
        toolbar->AddChild(mColorButton);

        mSizeSlider = mmake<PipelineSlider>();
        mSizeSlider->name = "size";
        mSizeSlider->layout->minWidth = sliderMinWidth;
        mSizeSlider->onChanged = [weakThis](float value, bool completed)
        {
            if (auto self = weakThis.Lock())
            {
                self->mNode->SetConfigNumber("brushSize", value);
                if (self->onConfigChanged) self->onConfigChanged("brushSize", completed);
            }
        };
        toolbar->AddChild(mSizeSlider);

        mOpacitySlider = mmake<PipelineSlider>();
        mOpacitySlider->name = "opacity";
        mOpacitySlider->layout->minWidth = sliderMinWidth;
        mOpacitySlider->onChanged = [weakThis](float value, bool completed)
        {
            if (auto self = weakThis.Lock())
            {
                self->mNode->SetConfigNumber("brushOpacity", value);
                if (self->onConfigChanged) self->onConfigChanged("brushOpacity", completed);
            }
        };
        toolbar->AddChild(mOpacitySlider);

        auto makeAction = [&](const String& icon, const String& name, const Function<void()>& action)
        {
            auto button = PipelineControls::MakeIconButton(icon, PipelineControls::textColor, Color4(0, 0, 0, 0));
            button->name = name;
            button->layout->minWidth = 20;
            button->layout->maxWidth = 20;
            button->onClick = action;
            toolbar->AddChild(button);
            return button;
        };
        mUndoButton = makeAction("ui/pipeline/btn_undo.png", "undo", [weakThis]() { if (auto self = weakThis.Lock()) self->OnUndo(); });
        mRedoButton = makeAction("ui/pipeline/btn_redo.png", "redo", [weakThis]() { if (auto self = weakThis.Lock()) self->OnRedo(); });
        mClearButton = makeAction("ui/UI4_small_trash_icon.png", "clear", [weakThis]() { if (auto self = weakThis.Lock()) self->OnClear(); });

        mPaletteRow = mmake<PipelineWrapRow>();
        mPaletteRow->name = "palette";
        mPaletteRow->spacing = 3;
        mPaletteRow->lineHeight = paletteHeight;
        *mPaletteRow->layout = WidgetLayout::HorStretch(VerAlign::Top, 0, 0, paletteHeight, toolbarHeight + 2);
        mPaletteRow->enabled = false;
        AddChild(mPaletteRow);

        for (auto& hex : drawPalette)
        {
            Color4 color;
            PipelineUtils::ParseHexColor(hex, color);
            auto swatch = o2UI.CreateButton("");
            swatch->layout->minWidth = 20;
            swatch->layout->maxWidth = 20;
            if (auto regular = swatch->GetLayerDrawable<Sprite>("regular"))
                regular->color = color;
            String value = hex;
            swatch->onClick = [weakThis, value]()
            {
                if (auto self = weakThis.Lock())
                {
                    self->mNode->SetConfigString("brushColor", value);
                    if (self->onConfigChanged) self->onConfigChanged("brushColor", true);
                    self->UpdateToolbar();
                }
            };
            mPaletteRow->AddChild(swatch);
        }

        auto pick = o2UI.CreateButton("...");
        pick->layout->minWidth = 28;
        pick->layout->maxWidth = 28;
        pick->onClick = [weakThis]()
        {
            auto self = weakThis.Lock();
            if (!self)
                return;

            ColorPickerDlg::Show(self->GetBrushColor(), [weakThis](const Color4& value, bool)
            {
                if (auto self = weakThis.Lock())
                {
                    self->mNode->SetConfigString("brushColor", PipelineUtils::ColorToHex(Color4(value.r, value.g, value.b, 255)));
                    if (self->onConfigChanged) self->onConfigChanged("brushColor", false);
                    self->UpdateToolbar();
                }
            }, [weakThis]()
            {
                if (auto self = weakThis.Lock())
                    if (self->onConfigChanged) self->onConfigChanged("brushColor", true);
            });
        };
        mPaletteRow->AddChild(pick);
    }

    void PipelinePaintEditor::UpdateToolbar()
    {
        String tool = GetTool();
        mBrushToggle->SetValue(tool == "brush");
        mEraserToggle->SetValue(tool == "eraser");
        mRegionToggle->SetValue(tool == "roi");

        if (auto regular = mColorButton->GetLayerDrawable<Sprite>("regular"))
            regular->color = GetBrushColor();

        mSizeSlider->Setup("Size", 1, 80, 1, GetBrushSize());
        mOpacitySlider->Setup("Alpha", 0, 1, 0.05f, GetBrushOpacity());

        mUndoButton->interactable = !mUndo.IsEmpty();
        mRedoButton->interactable = !mRedo.IsEmpty();
        mClearButton->interactable = mNode && !mNode->GetConfigString("drawing", "").IsEmpty();
        mUndoButton->transparency = mUndo.IsEmpty() ? 0.4f : 1.0f;
        mRedoButton->transparency = mRedo.IsEmpty() ? 0.4f : 1.0f;
        mClearButton->transparency = mClearButton->interactable ? 1.0f : 0.4f;
    }

    void PipelinePaintEditor::TogglePalette()
    {
        mPaletteOpen = !mPaletteOpen;
        mPaletteRow->enabled = mPaletteOpen;
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
        float toolbar = mToolbar ? Math::Max(toolbarHeight, mToolbar->GetHeightForWidth(width)) : toolbarHeight;
        float palette = mPaletteOpen && mPaletteRow ? Math::Max(paletteHeight, mPaletteRow->GetHeightForWidth(width)) + 2 : 0.0f;
        return toolbar + 4 + palette;
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

        if (mToolbar && mPaletteRow)
        {
            float width = layout->GetWidth();
            float toolbar = Math::Max(toolbarHeight, mToolbar->GetHeightForWidth(width));
            float palette = Math::Max(paletteHeight, mPaletteRow->GetHeightForWidth(width));
            *mToolbar->layout = WidgetLayout::HorStretch(VerAlign::Top, 0, 0, toolbar, 0);
            *mPaletteRow->layout = WidgetLayout::HorStretch(VerAlign::Top, 0, 0, palette, toolbar + 2);
        }
    }

    RectF PipelinePaintEditor::GetAreaRect() const
    {
        RectF rect = layout->GetWorldRect();
        rect.top -= BarsHeight(layout->GetWidth());
        return rect;
    }

    RectF PipelinePaintEditor::GetStageRect() const
    {
        RectF area = GetAreaRect();
        float pad = mWithRegion ? 18.0f : 0.0f;
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

    void PipelinePaintEditor::SyncRegionFromConfig()
    {
        if (!mNode)
            return;

        PipelineImageOps::CropRect roi;
        if (!PipelineImageOps::ParseCrop(mNode->GetConfigValue("roi"), roi))
        {
            roi.x = 0.1f; roi.y = 0.1f; roi.w = 0.8f; roi.h = 0.8f;
        }

        RectF stage = GetStageRect();
        float left = stage.left + stage.Width() * roi.x;
        float top = stage.top - stage.Height() * roi.y;
        float w = stage.Width() * roi.w;
        float h = stage.Height() * roi.h;
        mRegionSyncing = true;
        mRegionFrame->SetBasis(Basis(Vec2F(left, top - h), Vec2F(w, 0), Vec2F(0, h)));
        mRegionSyncing = false;
    }

    void PipelinePaintEditor::OnRegionTransformed(const Basis& basis)
    {
        if (mRegionSyncing || !mNode)
            return;

        RectF stage = GetStageRect();
        if (stage.Width() <= 0 || stage.Height() <= 0)
            return;

        RectF frame(basis.origin, basis.origin + basis.xv + basis.yv);
        float x = Math::Clamp((frame.left - stage.left) / stage.Width(), 0.0f, 1.0f);
        float right = Math::Clamp((frame.right - stage.left) / stage.Width(), 0.0f, 1.0f);
        float y = Math::Clamp((stage.top - frame.top) / stage.Height(), 0.0f, 1.0f);
        float bottom = Math::Clamp((stage.top - frame.bottom) / stage.Height(), 0.0f, 1.0f);

        auto& roi = mNode->config["roi"];
        roi.SetObject();
        roi["x"] = x;
        roi["y"] = y;
        roi["w"] = Math::Max(0.005f, right - x);
        roi["h"] = Math::Max(0.005f, bottom - y);

        if (onConfigChanged)
            onConfigChanged("roi", false);
    }

    void PipelinePaintEditor::OnRegionCompleted()
    {
        if (onConfigChanged)
            onConfigChanged("roi", true);
    }

    void PipelinePaintEditor::Update(float dt)
    {
        Widget::Update(dt);

        // The remove tab follows the selected box, at its top-right corner
        bool showRemove = mWithRegion && mRegionRemovable && GetTool() == "roi";
        if (mRemoveButton->IsEnabled() != showRemove)
            mRemoveButton->enabled = showRemove;
        if (showRemove)
        {
            const Basis& b = mRegionFrame->GetCurrentBasis();
            RectF frame(b.origin, b.origin + b.xv + b.yv);
            RectF box(Math::Min(frame.left, frame.right), Math::Max(frame.top, frame.bottom), Math::Max(frame.left, frame.right), Math::Min(frame.top, frame.bottom));
            RectF want(box.right - 18.0f, box.top, box.right, box.top - 18.0f);
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

        o2Render.DrawFilledPolygon({ stage.LeftBottom(), Vec2F(stage.left, stage.top), stage.RightTop(), Vec2F(stage.right, stage.bottom) }, Color4(255, 255, 255, 255));
        if (mBackground)
        {
            mBackgroundSprite->rect = stage;
            mBackgroundSprite->Draw();
        }

        if (mComposed && mTexture)
        {
            if (mTextureDirty)
            {
                mTexture->SetData(*mComposed);
                mTextureDirty = false;
            }
            mSprite->rect = stage;
            mSprite->Draw();
        }

        o2Render.DrawAARectFrame(stage, Color4(96, 125, 139, 140), 1.0f);

        CursorAreaEventsListener::OnDrawn();

        if (mWithRegion && !farView)
        {
            if (!mRegionDragging)
                SyncRegionFromConfig();

            const Basis& b = mRegionFrame->GetCurrentBasis();
            RectF frame(b.origin, b.origin + b.xv + b.yv);

            // Everything outside the box is not sent to the model: shade it
            RectF box(Math::Clamp(Math::Min(frame.left, frame.right), stage.left, stage.right),
                      Math::Clamp(Math::Max(frame.top, frame.bottom), stage.bottom, stage.top),
                      Math::Clamp(Math::Max(frame.left, frame.right), stage.left, stage.right),
                      Math::Clamp(Math::Min(frame.top, frame.bottom), stage.bottom, stage.top));
            Color4 shade(0, 0, 0, 120);
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

            if (GetTool() == "roi")
                mRegionFrame->Draw();
        }

        if (mHovered && !farView && GetTool() != "roi")
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
        DrawTopLayers();
    }

    void PipelinePaintEditor::OnCursorPressed(const Input::Cursor& cursor)
    {
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
