#include "o2Editor/stdafx.h"
#include "UiWsStylePicker.h"

#include <cmath>
#include <map>

#include "o2/Render/Render.h"
#include "o2Editor/Pipeline/UiDoc/UiDocRender.h"
#include "o2Editor/Windows/PipelineWindow/PipelineDraw.h"

namespace Editor
{
    using namespace UiWsDraw;
    namespace Ws = UiDoc::Ws;

    namespace
    {
        const float rowHeight = 52.0f; // .ulw-stylerow: a 40 px preview, 5 px padding, a 1 px border
        const float rowGap = 2.0f;
        const float listPad = 6.0f;

        // ----------------------------------------------------------
        // A preview drawn once: its texture and size, the frame it was used
        // ----------------------------------------------------------
        struct Drawn
        {
            TextureRef texture;
            Vec2I      size;
            Vec2F      doc;   // The sample screen's size (design pixels)
        };

        std::map<std::string, Drawn>& Cache()
        {
            static auto* cache = new std::map<std::string, Drawn>();
            return *cache;
        }

        // The style drawn by the layout renderer on its tight sample screen, at about two pixels per point of the box
        const Drawn& PreviewOf(const UiDoc::TextStyle& style, const String& text, const Vec2F& box)
        {
            UiDoc::UiDoc doc = Ws::TightStyleDoc(style, text);
            float k = Math::Min(box.x/(float)doc.width, box.y/(float)doc.height)*2.0f;
            int pixelLong = Math::Clamp((int)std::ceil(Math::Max((float)doc.width, (float)doc.height)*k), 16, 1024);
            std::string key = std::string(UiDoc::TextStyleToJson(style).Dump().Data()) + "|" + text.Data() + "|" + std::to_string(pixelLong);
            auto& cache = Cache();
            auto it = cache.find(key);
            if (it != cache.end())
                return it->second;

            if (cache.size() > 160)
                cache.clear();

            UiDoc::UiKit kit;
            kit.name = "k";
            kit.textStyles = { style };
            UiDoc::UiRenderOptions o;
            o.mode = "kit";
            o.kit = &kit;
            o.pixelLong = pixelLong;
            UiLayout::UiImage image = UiDoc::RenderLayout(doc, o);
            Drawn drawn;
            drawn.doc = Vec2F((float)doc.width, (float)doc.height);
            if (!image.IsEmpty())
            {
                auto bitmap = image.ToBitmap();
                drawn.texture = TextureRef(*bitmap);
                drawn.texture->SetFilter(Texture::Filter::Linear);
                drawn.size = bitmap->GetSize();
            }
            return cache[key] = drawn;
        }
    }

    namespace UiWsStylePicker
    {
        void DrawPreview(const UiDoc::TextStyle& style, const String& text, const RectF& plate, float radius, float pad)
        {
            // .ulw-stylepic: the dark paper whatever the theme, a faint inner edge
            Fill(plate, radius, Color4(0x20, 0x23, 0x2B, 255));
            Border(plate, radius, Color4(255, 255, 255, 15));
            RectF inner(plate.left + pad + 1, plate.top - pad*0.75f - 1, plate.right - pad - 1, plate.bottom + pad*0.75f + 1);
            if (inner.Width() < 2.0f || inner.Height() < 2.0f)
                return;

            const Drawn& drawn = PreviewOf(style, text, Vec2F(inner.Width(), inner.Height()));
            if (!drawn.texture)
                return;

            // The sample screen fitted into the plate (letterboxed)
            float k = Math::Min(inner.Width()/drawn.doc.x, inner.Height()/drawn.doc.y);
            Vec2F size(drawn.doc.x*k, drawn.doc.y*k), c = inner.Center();
            Picture(drawn.texture, drawn.size, RectF(c.x - size.x*0.5f, c.y + size.y*0.5f, c.x + size.x*0.5f, c.y - size.y*0.5f));
        }

        void DrawButton(const RectF& rect, const UiDoc::UiNode& node, const UiDoc::UiKit* kit, bool hover, bool enabled)
        {
            auto& t = T();
            const UiDoc::TextStyle* cur = node.style ? UiDoc::KitStyle(kit, *node.style) : nullptr;
            UiDoc::TextStyle autoStyle = Ws::AutoStyleOf(node, kit);
            const UiDoc::TextStyle& shown = cur ? *cur : autoStyle;

            // .ulw-stylepick: the field, a 66 x 20 preview, the name, the chevron
            Fill(rect, 8.0f, t.field);
            Border(rect, 8.0f, hover && enabled ? t.accent : t.line);
            float cy = rect.Center().y;
            RectF plate(rect.left + 4, cy + 10, rect.left + 4 + 66, cy - 10);
            DrawPreview(shown, Ws::StyleSampleText(node, shown), plate, 4.0f, 2.0f);
            String label = cur ? cur->name : node.style ? *node.style : "auto \xC2\xB7 " + autoStyle.name;
            Color4 color = !cur && node.style ? t.yellow : t.fg;
            Text(label, RectF(plate.right + 7, rect.top, rect.right - 24, rect.bottom), TextStyle(11.5f, 400, A(color, enabled ? 1.0f : 0.5f)));
            UiWsDraw::Icon("uiws_chevron_down", Vec2F(rect.right - 14, cy), 12.0f, t.fg3);
        }

        int CachedPreviews()
        {
            return (int)Cache().size();
        }
    }

    // --- the list -------------------------------------------------------------------------------------

    const UiDoc::UiNode* UiWsStyleListPanel::Block() const
    {
        if (!ctx)
            return nullptr;
        const auto& doc = ctx->Doc();
        return blockId.IsEmpty() ? nullptr : UiDoc::FindNode(doc, blockId);
    }

    float UiWsStyleListPanel::ContentHeight() const
    {
        int n = 1 + (ctx && ctx->hasKit ? ctx->kit.textStyles.Count() : 0);
        float h = listPad*2 + n*rowHeight + (n - 1)*rowGap;
        if (!ctx || !ctx->hasKit || ctx->kit.textStyles.IsEmpty())
            h += 28;
        return h;
    }

    Vector<String> UiWsStyleListPanel::RowIds() const
    {
        Vector<String> ids = { "auto" };
        if (ctx && ctx->hasKit)
        {
            for (auto& s : ctx->kit.textStyles)
                ids.Add(s.name);
        }
        return ids;
    }

    void UiWsStyleListPanel::OnScrolled(float scroll)
    {
        mScroll = Math::Clamp(mScroll - scroll*0.5f, 0.0f, Math::Max(0.0f, ContentHeight() - layout->GetHeight()));
    }

    void UiWsStyleListPanel::DrawPanel(const Box& box)
    {
        const UiDoc::UiNode* n = Block();
        if (!n)
            return;

        auto& t = T();
        const UiDoc::UiKit* kit = ctx->hasKit ? &ctx->kit : nullptr;
        const UiDoc::TextStyle* cur = n->style ? UiDoc::KitStyle(kit, *n->style) : nullptr;
        float W = box.W();
        mScroll = Math::Clamp(mScroll, 0.0f, Math::Max(0.0f, ContentHeight() - box.H()));
        o2Render.EnableScissorTest((RectI)box.world);
        float y = listPad - mScroll;
        auto row = [&](const String& id, const UiDoc::TextStyle& style, const String& title, const String& sub, bool on, bool sparkle)
        {
            RectF r = box.R(listPad, y, W - listPad*2, rowHeight);
            String hot = "style:" + id;
            if (on)
            {
                Fill(r, 8.0f, t.accentSoft);
                Border(r, 8.0f, t.accent);
            }
            else if (IsHover(hot))
                Fill(r, 8.0f, t.active);
            RectF plate(r.left + 6, r.top - 6, r.left + 6 + 132, r.top - 6 - 40);
            UiWsStylePicker::DrawPreview(style, Ws::StyleSampleText(*n, style), plate, 6.0f, 4.0f);
            float x = plate.right + 10;
            if (sparkle)
            {
                UiWsDraw::Icon("uiws_sparkle", Vec2F(x + 5.5f, r.Center().y + 8), 11.0f, t.accent);
                x += 11 + 4;
            }
            Text(title, RectF(x, r.Center().y + 16, r.right - 6, r.Center().y), TextStyle(11.5f, 600, t.fg));
            Text(sub, RectF(plate.right + 10, r.Center().y, r.right - 6, r.Center().y - 16), TextStyle(10.5f, 400, t.fgDim));
            if (r.bottom < box.world.top && r.top > box.world.bottom)
                AddHot(r, hot);
            y += rowHeight + rowGap;
        };

        UiDoc::TextStyle autoStyle = Ws::AutoStyleOf(*n, kit);
        row("", autoStyle, "auto", autoStyle.name + " \xC2\xB7 " + autoStyle.role, !n->style, true);
        if (kit)
        {
            for (auto& s : kit->textStyles)
                row(s.name, s, s.name, s.role + " \xC2\xB7 " + s.font + " \xC2\xB7 " + UiDoc::JsNum(s.size) + "px", cur && cur->name == s.name, false);
        }
        if (!kit || kit->textStyles.IsEmpty())
            Text("The kit has no text styles", box.R(listPad + 8, y + 2, W - listPad*2 - 16, 24), TextStyle(11.5f, 400, t.fg3));
        o2Render.DisableScissorTest();
    }

    void UiWsStyleListPanel::OnHotClick(const String& id, const Input::Cursor& cursor)
    {
        if (id.StartsWith("style:") && onPick)
            onPick(id.SubStr(6));
    }
}
