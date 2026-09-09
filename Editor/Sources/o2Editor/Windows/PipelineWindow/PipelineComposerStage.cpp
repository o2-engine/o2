#include "o2Editor/stdafx.h"
#include "PipelineComposerStage.h"

#include "o2/Application/Input.h"
#include "o2/Render/Render.h"
#include "o2/Render/Sprite.h"
#include "o2/Render/Text.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Utils/Bitmap/Bitmap.h"
#include "o2Editor/Pipeline/PipelineImageOps.h"
#include "o2Editor/Pipeline/PipelineUtils.h"
#include "o2Editor/Windows/PipelineWindow/PipelineControls.h"

namespace Editor
{
    static const float stagePad = 14.0f;

    PipelineComposerStage::PipelineComposerStage(RefCounter* refCounter):
        Widget(refCounter)
    {
        layout->minSize = Vec2F(120, 120);

        mCheckerSprite = mmake<Sprite>("ui/pipeline/checker.png");
        mCheckerSprite->mode = SpriteMode::Tiled;
        mCheckerSprite->transparency = 0.45f;

        mNameText = mmake<Text>("stdFont.ttf");
        mNameText->horAlign = HorAlign::Middle;
        mNameText->verAlign = VerAlign::Middle;
        mNameText->color = PipelineControls::textColor;
        mNameText->wordWrap = true;

        mFrame = mmake<FrameHandles>();
        mFrame->SetPivotEnabled(false);
        mFrame->SetRotationEnabled(true);
        mFrame->onTransformed = THIS_FUNC(OnFrameTransformed);
        mFrame->onChangeCompleted = THIS_FUNC(OnFrameCompleted);
        mFrame->onPressed = [this]() { mFrameDragging = true; };
        mFrame->onReleased = [this]() { mFrameDragging = false; };
    }

    void PipelineComposerStage::Init(const Ref<PipelineNode>& node)
    {
        mNode = node;
        Refresh();
    }

    int PipelineComposerStage::GetCanvasW() const
    {
        return mNode ? Math::Clamp((int)Math::Round(mNode->GetConfigNumber("canvasW", 1024)), 16, 8192) : 1024;
    }

    int PipelineComposerStage::GetCanvasH() const
    {
        return mNode ? Math::Clamp((int)Math::Round(mNode->GetConfigNumber("canvasH", 1024)), 16, 8192) : 1024;
    }

    Vector<ComposerLayerRef> PipelineComposerStage::GetLayers() const
    {
        return mNode ? ResolveComposerLayers(*mNode) : Vector<ComposerLayerRef>();
    }

    Ref<Bitmap> PipelineComposerStage::GetLayerImage(const String& portId) const
    {
        for (auto& view : mLayers)
        {
            if (view.ref.portId == portId)
                return view.source;
        }
        return nullptr;
    }

    Vec2I PipelineComposerStage::GetNaturalSize(const String& portId) const
    {
        auto image = GetLayerImage(portId);
        return image ? image->GetSize() : Vec2I();
    }

    ComposerLayerPlacement PipelineComposerStage::DefaultPlacement(const String& portId) const
    {
        ComposerLayerPlacement p;
        float cw = (float)GetCanvasW(), ch = (float)GetCanvasH();
        Vec2I natural = GetNaturalSize(portId);
        if (natural.x <= 0 || natural.y <= 0)
        {
            float s = Math::Min(cw, ch) * 0.4f;
            p.x = (cw - s) / 2; p.y = (ch - s) / 2; p.w = s; p.h = s;
            return p;
        }

        float scale = Math::Min(1.0f, Math::Min(cw / natural.x, ch / natural.y));
        p.w = natural.x * scale;
        p.h = natural.y * scale;
        p.x = (cw - p.w) / 2;
        p.y = (ch - p.h) / 2;
        return p;
    }

    ComposerLayerPlacement PipelineComposerStage::GetPlacement(const ComposerLayerRef& layer) const
    {
        auto layers = mNode ? mNode->GetConfigValue("layers") : nullptr;
        const DataValue* stored = layers && layers->IsObject() ? layers->FindMember(layer.id.Data()) : nullptr;
        if (!stored || !stored->IsObject())
            return DefaultPlacement(layer.portId);

        auto num = [&](const char* key, float def)
        {
            auto m = stored->FindMember(key);
            return m ? PipelineUtils::ValueToNumber(*m, def) : def;
        };
        auto flag = [&](const char* key, bool def)
        {
            auto m = stored->FindMember(key);
            if (!m) return def;
            return m->IsBoolean() ? (bool)*m : PipelineUtils::ValueToNumber(*m, def ? 1.0f : 0.0f) != 0.0f;
        };

        ComposerLayerPlacement p;
        p.stored = true;
        p.x = num("x", 0); p.y = num("y", 0);
        p.w = Math::Max(2.0f, num("w", 2)); p.h = Math::Max(2.0f, num("h", 2));
        p.rot = num("rot", 0);
        p.hidden = flag("hidden", false);
        p.flipH = flag("flipH", false);
        p.flipV = flag("flipV", false);
        p.opacity = Math::Clamp(num("opacity", 1), 0.0f, 1.0f);
        p.nine = flag("nine", false);
        if (auto slice = stored->FindMember("slice"))
        {
            auto sn = [&](const char* key) { auto m = slice->IsObject() ? slice->FindMember(key) : nullptr; return m ? Math::Max(0, (int)PipelineUtils::ValueToNumber(*m, 0)) : 0; };
            p.slice = { sn("l"), sn("t"), sn("r"), sn("b") };
        }
        p.sliceScale = Math::Clamp(num("sliceScale", 1), 0.1f, 4.0f);
        p.lockAspect = flag("lockAspect", true);
        return p;
    }

    void PipelineComposerStage::WritePlacement(const String& layerId, const ComposerLayerPlacement& placement, bool completed)
    {
        if (!mNode)
            return;

        auto& layers = mNode->config["layers"];
        if (!layers.IsObject())
            layers.SetObject();

        auto& L = layers[layerId.Data()];
        L.SetObject();
        L["x"] = placement.x;
        L["y"] = placement.y;
        L["w"] = placement.w;
        L["h"] = placement.h;
        L["rot"] = placement.rot;
        L["hidden"] = placement.hidden;
        L["flipH"] = placement.flipH;
        L["flipV"] = placement.flipV;
        L["opacity"] = placement.opacity;
        L["nine"] = placement.nine;
        auto& slice = L["slice"];
        slice.SetObject();
        slice["l"] = placement.slice.l;
        slice["t"] = placement.slice.t;
        slice["r"] = placement.slice.r;
        slice["b"] = placement.slice.b;
        L["sliceScale"] = placement.sliceScale;
        L["lockAspect"] = placement.lockAspect;

        if (onConfigChanged)
            onConfigChanged("layers", completed);
    }

    void PipelineComposerStage::SelectLayer(const String& layerId)
    {
        if (!mNode || GetSelectedLayer() == layerId)
            return;

        mNode->SetConfigString("selectedLayer", layerId);
        if (onConfigChanged)
            onConfigChanged("selectedLayer", true);
    }

    String PipelineComposerStage::GetSelectedLayer() const
    {
        return mNode ? mNode->GetConfigString("selectedLayer", "") : String();
    }

    void PipelineComposerStage::Refresh()
    {
        Vector<LayerView> next;
        for (auto& ref : GetLayers())
        {
            LayerView view;
            view.ref = ref;
            view.source = imageOfPort ? imageOfPort(ref.portId) : nullptr;
            if (auto previous = FindView(ref.id))
            {
                if (previous->source == view.source)
                {
                    view.sprite = previous->sprite;
                    view.texture = previous->texture;
                    view.cacheKey = previous->cacheKey;
                }
            }
            next.Add(view);
        }
        mLayers = next;
    }

    PipelineComposerStage::LayerView* PipelineComposerStage::FindView(const String& layerId)
    {
        for (auto& view : mLayers)
        {
            if (view.ref.id == layerId)
                return &view;
        }
        return nullptr;
    }

    Ref<Bitmap> PipelineComposerStage::RenderLayer(const ComposerLayerRef& layer) const
    {
        auto source = GetLayerImage(layer.portId);
        if (!source)
            return nullptr;

        auto p = GetPlacement(layer);
        int w = Math::Max(1, (int)Math::Round(p.w)), h = Math::Max(1, (int)Math::Round(p.h));
        Ref<Bitmap> buf = p.nine ? PipelineImageOps::NineSliceResize(*source, w, h, p.slice, p.sliceScale)
            : PipelineImageOps::Resize(*source, Vec2I(w, h));
        return PipelineImageOps::Flip(*buf, p.flipH, p.flipV);
    }

    void PipelineComposerStage::EnsureLayerSprite(LayerView& view, const ComposerLayerPlacement& p)
    {
        if (!view.source)
            return;

        String key = p.nine ? String("nine:") + (String)(int)Math::Round(p.w) + "x" + (String)(int)Math::Round(p.h) + ":" +
            (String)p.slice.l + "," + (String)p.slice.t + "," + (String)p.slice.r + "," + (String)p.slice.b + ":" + (String)p.sliceScale : String("plain");

        if (!view.sprite || view.cacheKey != key)
        {
            Ref<Bitmap> image = view.source;
            if (p.nine)
                image = PipelineImageOps::NineSliceResize(*view.source, Math::Max(1, (int)Math::Round(p.w)), Math::Max(1, (int)Math::Round(p.h)), p.slice, p.sliceScale);

            view.texture = TextureRef(*image);
            if (!view.sprite)
                view.sprite = mmake<Sprite>();
            view.sprite->SetTexture(view.texture);
            view.sprite->SetTextureSrcRect(RectI(Vec2I(), image->GetSize()));
            view.cacheKey = key;
        }
    }

    RectF PipelineComposerStage::GetStageRect() const
    {
        RectF area = layout->GetWorldRect();
        float cw = (float)GetCanvasW(), ch = (float)GetCanvasH();
        float vs = GetViewScale();
        Vec2F size(cw * vs, ch * vs);
        Vec2F center = area.Center();
        return RectF(center.x - size.x * 0.5f, center.y + size.y * 0.5f, center.x + size.x * 0.5f, center.y - size.y * 0.5f);
    }

    float PipelineComposerStage::GetViewScale() const
    {
        RectF area = layout->GetWorldRect();
        float cw = (float)GetCanvasW(), ch = (float)GetCanvasH();
        float availW = Math::Max(1.0f, area.Width() - 2 * stagePad);
        float availH = Math::Max(1.0f, area.Height() - 2 * stagePad);
        float fit = Math::Max(0.0001f, Math::Min(availW / cw, availH / ch));
        float zoom = mNode ? Math::Clamp(mNode->GetConfigNumber("viewZoom", 1), 0.1f, 8.0f) : 1.0f;
        return fit * zoom;
    }

    Vec2F PipelineComposerStage::ToWork(const Vec2F& canvasPoint) const
    {
        RectF stage = GetStageRect();
        float vs = Math::Max(0.0001f, GetViewScale());
        return Vec2F((canvasPoint.x - stage.left) / vs, (stage.top - canvasPoint.y) / vs);
    }

    Basis PipelineComposerStage::PlacementBasis(const ComposerLayerPlacement& p) const
    {
        RectF stage = GetStageRect();
        float vs = GetViewScale();
        Vec2F center(stage.left + (p.x + p.w * 0.5f) * vs, stage.top - (p.y + p.h * 0.5f) * vs);
        float angle = Math::Deg2rad(p.rot);
        Vec2F ex(Math::Cos(angle), -Math::Sin(angle));
        Vec2F ey(Math::Sin(angle), Math::Cos(angle));
        Vec2F xv = ex * (p.w * vs);
        Vec2F yv = ey * (p.h * vs);
        return Basis(center - xv * 0.5f - yv * 0.5f, xv, yv);
    }

    bool PipelineComposerStage::IsPointInLayer(const ComposerLayerPlacement& p, const Vec2F& canvasPoint) const
    {
        return PlacementBasis(p).IsPointInside(canvasPoint);
    }

    bool PipelineComposerStage::IsUnderPoint(const Vec2F& point)
    {
        return layout->GetWorldRect().IsInside(point);
    }

    void PipelineComposerStage::SyncFrame()
    {
        String selected = GetSelectedLayer();
        auto view = FindView(selected);
        if (!view)
            return;

        mFrameSyncing = true;
        mFrame->SetBasis(PlacementBasis(GetPlacement(view->ref)));
        mFrameSyncing = false;
    }

    void PipelineComposerStage::OnFrameTransformed(const Basis& basis)
    {
        if (mFrameSyncing)
            return;

        String selected = GetSelectedLayer();
        auto view = FindView(selected);
        if (!view)
            return;

        const Basis& b = mFrame->GetCurrentBasis();
        RectF stage = GetStageRect();
        float vs = Math::Max(0.0001f, GetViewScale());

        auto p = GetPlacement(view->ref);
        p.w = Math::Max(2.0f, b.xv.Length() / vs);
        p.h = Math::Max(2.0f, b.yv.Length() / vs);
        p.rot = Math::Rad2deg(Math::Atan2F(-b.xv.y, b.xv.x));
        Vec2F center = b.origin + b.xv * 0.5f + b.yv * 0.5f;
        p.x = (center.x - stage.left) / vs - p.w * 0.5f;
        p.y = (stage.top - center.y) / vs - p.h * 0.5f;
        WritePlacement(selected, p, false);
    }

    void PipelineComposerStage::OnFrameCompleted()
    {
        String selected = GetSelectedLayer();
        auto view = FindView(selected);
        if (!view)
            return;

        WritePlacement(selected, GetPlacement(view->ref), true);
    }

    void PipelineComposerStage::Update(float dt)
    {
        Widget::Update(dt);
    }

    void PipelineComposerStage::Draw()
    {
        if (!mResEnabledInHierarchy || mIsClipped)
            return;

        Widget::Draw();

        RectF area = layout->GetWorldRect();
        RectF stage = GetStageRect();
        float vs = GetViewScale();

        o2Render.EnableScissorTest((RectI)area);

        Color4 bg;
        bool bgEnabled = mNode && mNode->GetConfigBool("cmpBgEnabled", false) && PipelineUtils::ParseHexColor(mNode->GetConfigString("cmpBg", "#3a6ea5"), bg);
        bool checker = !mNode || mNode->GetConfigBool("checker", true);
        Vector<Vec2F> quad = { stage.LeftBottom(), Vec2F(stage.left, stage.top), stage.RightTop(), Vec2F(stage.right, stage.bottom) };
        if (bgEnabled)
            o2Render.DrawFilledPolygon(quad, Color4(bg.r, bg.g, bg.b, 255));
        else
        {
            o2Render.DrawFilledPolygon(quad, Color4::White());
            if (checker)
            {
                mCheckerSprite->rect = stage;
                mCheckerSprite->Draw();
            }
        }

        String selected = GetSelectedLayer();
        for (auto& view : mLayers)
        {
            auto p = GetPlacement(view.ref);
            if (p.hidden)
                continue;

            Basis basis = PlacementBasis(p);
            if (!view.source)
            {
                o2Render.DrawAABasis(basis, Color4(96, 125, 139, 200), Color4(96, 125, 139, 200), Color4(96, 125, 139, 200));
                Vec2F center = basis.origin + basis.xv * 0.5f + basis.yv * 0.5f;
                mNameText->text = (view.ref.name.IsEmpty() ? String("layer") : view.ref.name) + "\nrun to load";
                mNameText->rect = RectF(center - Vec2F(p.w * vs * 0.5f, 20), center + Vec2F(p.w * vs * 0.5f, 20));
                mNameText->Draw();
                continue;
            }

            EnsureLayerSprite(view, p);
            auto& sprite = view.sprite;
            sprite->pivot = Vec2F(0.5f, 0.5f);
            sprite->size = Vec2F(p.w * vs, p.h * vs);
            sprite->position = basis.origin + basis.xv * 0.5f + basis.yv * 0.5f;
            sprite->angleDegree = -p.rot;
            sprite->scale = Vec2F(p.flipH ? -1.0f : 1.0f, p.flipV ? -1.0f : 1.0f);
            sprite->transparency = p.opacity;
            sprite->Draw();
        }

        o2Render.DisableScissorTest();
        o2Render.DrawAARectFrame(stage, Color4(96, 125, 139, 120), 1.0f);

        CursorAreaEventsListener::OnDrawn();
        if (PipelineControls::IsFarView())
            return;

        if (auto view = FindView(selected))
        {
            auto p = GetPlacement(view->ref);
            if (!p.hidden)
            {
                if (!mFrameDragging)
                    SyncFrame();

                const Basis& b = mFrame->GetCurrentBasis();
                if (p.nine)
                {
                    Vec2F xu = b.xv.Normalized(), yu = b.yv.Normalized();
                    float l = p.slice.l * p.sliceScale * vs, r = p.slice.r * p.sliceScale * vs;
                    float t = p.slice.t * p.sliceScale * vs, bt = p.slice.b * p.sliceScale * vs;
                    Color4 guide(0, 150, 136, 160);
                    o2Render.DrawAALine(b.origin + xu * l, b.origin + xu * l + b.yv, guide, 1.0f);
                    o2Render.DrawAALine(b.origin + b.xv - xu * r, b.origin + b.xv - xu * r + b.yv, guide, 1.0f);
                    o2Render.DrawAALine(b.origin + yu * bt, b.origin + yu * bt + b.xv, guide, 1.0f);
                    o2Render.DrawAALine(b.origin + b.yv - yu * t, b.origin + b.yv - yu * t + b.xv, guide, 1.0f);
                }

                mFrame->Draw();

                String badge = (String)(int)Math::Round(p.w) + "x" + (String)(int)Math::Round(p.h);
                if (Math::Abs(p.rot) > 0.5f)
                    badge += " . " + (String)(int)Math::Round(p.rot) + " deg";
                Vec2F center = b.origin + b.xv * 0.5f + b.yv * 0.5f;
                float below = Math::Min(b.origin.y, Math::Min((b.origin + b.xv).y, Math::Min((b.origin + b.yv).y, (b.origin + b.xv + b.yv).y)));
                mNameText->text = badge;
                mNameText->rect = RectF(center.x - 80, below - 2, center.x + 80, below - 18);
                mNameText->Draw();
            }
        }

        mNameText->text = "Select a layer to transform . Shift: move on one axis";
        mNameText->rect = RectF(area.left, area.bottom + 16, area.right, area.bottom + 2);
        mNameText->Draw();
    }

    void PipelineComposerStage::OnCursorPressed(const Input::Cursor& cursor)
    {
        for (int i = mLayers.Count() - 1; i >= 0; i--)
        {
            auto& view = mLayers[i];
            auto p = GetPlacement(view.ref);
            if (p.hidden || !IsPointInLayer(p, cursor.position))
                continue;

            SelectLayer(view.ref.id);
            mMoving = true;
            mMovingId = view.ref.id;
            mMoveStart = ToWork(cursor.position);
            mMoveOrigin = p;
            return;
        }

        SelectLayer("");
    }

    void PipelineComposerStage::OnCursorStillDown(const Input::Cursor& cursor)
    {
        if (!mMoving)
            return;

        Vec2F delta = ToWork(cursor.position) - mMoveStart;
        if (o2Input.IsKeyDown(VK_SHIFT))
        {
            if (Math::Abs(delta.x) >= Math::Abs(delta.y)) delta.y = 0;
            else delta.x = 0;
        }

        if (delta.Length() < 0.01f)
            return;

        auto p = mMoveOrigin;
        p.x += delta.x;
        p.y += delta.y;
        WritePlacement(mMovingId, p, false);
    }

    void PipelineComposerStage::OnCursorReleased(const Input::Cursor& cursor)
    {
        if (!mMoving)
            return;

        mMoving = false;
        if (auto view = FindView(mMovingId))
            WritePlacement(mMovingId, GetPlacement(view->ref), true);
    }

    void PipelineComposerStage::OnCursorPressBreak(const Input::Cursor& cursor)
    {
        OnCursorReleased(cursor);
    }
}
// --- META ---

DECLARE_CLASS(Editor::PipelineComposerStage, Editor__PipelineComposerStage);
// --- END META ---
