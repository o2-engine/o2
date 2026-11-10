#include "o2Editor/stdafx.h"
#include "UiWsDraw.h"

#include <unordered_map>

#include "o2/Assets/Types/FontAsset.h"
#include "o2/Render/Render.h"
#include "o2/Render/Sprite.h"
#include "o2/Render/Text.h"
#include "o2/Render/VectorSprite.h"
#include "o2Editor/Pipeline/PipelineUtils.h"
#include "o2Editor/Pipeline/UiLayout/UiLayoutPixels.h"
#include "o2Editor/Windows/PipelineWindow/PipelineDraw.h"

namespace Editor::UiWsDraw
{
    namespace
    {
        int frameStamp = 0;

        struct CachedText
        {
            Ref<o2::Text> text;
            int           used = 0;
        };

        struct CachedIcon
        {
            Ref<VectorSprite> sprite;
            int               used = 0;
        };

        struct CachedKit
        {
            TextureRef  texture;
            Vec2I       size;
            Ref<Sprite> sprite;
            bool        failed = false;
            int         used = 0;
        };

        std::unordered_map<std::string, CachedText>& Texts() { static auto* m = new std::unordered_map<std::string, CachedText>(); return *m; }
        std::unordered_map<std::string, CachedIcon>& Icons() { static auto* m = new std::unordered_map<std::string, CachedIcon>(); return *m; }
        std::unordered_map<std::string, CachedKit>& Kits() { static auto* m = new std::unordered_map<std::string, CachedKit>(); return *m; }
        std::unordered_map<std::string, float>& Widths() { static auto* m = new std::unordered_map<std::string, float>(); return *m; }

        String FontOf(int weight, bool mono)
        {
            return mono ? PipelineTheme::MonoFont() : PipelineTheme::Font(weight >= 600 ? 600 : weight >= 500 ? 500 : 400);
        }

        Ref<Font> FontRef(const String& path)
        {
            static Map<String, Ref<Font>>* fonts = new Map<String, Ref<Font>>();
            Ref<Font> font;
            if (fonts->TryGetValue(path, font) && font)
                return font;

            AssetRef<FontAsset> asset(path);
            font = asset ? asset->GetFont() : nullptr;
            (*fonts)[path] = font;
            return font;
        }

        // A key of a long picture: its name, length and a sample of its characters
        std::string KitKey(const UiDoc::UiKitElement& e)
        {
            UInt32 h = 2166136261u;
            const String& png = e.png;
            int step = Math::Max(1, png.Length()/512);
            for (int i = 0; i < png.Length(); i += step)
            {
                h ^= (UInt8)png[i];
                h *= 16777619u;
            }
            return std::string(e.name.Data()) + "|" + std::to_string(png.Length()) + "|" + std::to_string(h);
        }
    }

    const PipelinePalette& T()
    {
        return PipelineTheme::Get();
    }

    Color4 A(const Color4& color, float alpha)
    {
        return PipelineTheme::Alpha(color, alpha);
    }

    Color4 SnapColor()
    {
        return Color4(255, 95, 162, 255);
    }

    // --- shapes -------------------------------------------------------------------------------

    void Fill(const RectF& rect, float radius, const Color4& color)
    {
        if (color.a == 0 || rect.Width() <= 0.0f || rect.Height() <= 0.0f)
            return;

        PipelineDraw::Fill(rect, PipelineDraw::Corners(Math::Min(radius, Math::Min(rect.Width(), rect.Height())*0.5f)), color);
    }

    void Border(const RectF& rect, float radius, const Color4& color, float width /*= 1.0f*/)
    {
        if (color.a == 0)
            return;

        PipelineDraw::Stroke(rect, PipelineDraw::Corners(Math::Min(radius, Math::Min(rect.Width(), rect.Height())*0.5f)), 0.0f, width,
                             PipelineDraw::Gradient(color));
    }

    void Frame(const RectF& rect, float radius, const Color4& color, float width)
    {
        if (color.a == 0)
            return;

        PipelineDraw::Frame(rect, PipelineDraw::Corners(radius), width, color);
    }

    void DashedLine(const Vec2F& a, const Vec2F& b, const Color4& color, float dash, float gap, float width /*= 1.0f*/)
    {
        Vec2F d = b - a;
        float length = d.Length();
        if (length < 0.01f)
            return;

        Vec2F dir = d/length;
        for (float s = 0.0f; s < length; s += dash + gap)
            PipelineDraw::Line(a + dir*s, a + dir*Math::Min(length, s + dash), width, color);
    }

    void DashedFrame(const RectF& rect, const Color4& color, float dash, float gap, float width /*= 1.0f*/)
    {
        Vec2F lt(rect.left, rect.top), rt(rect.right, rect.top), rb(rect.right, rect.bottom), lb(rect.left, rect.bottom);
        DashedLine(lt, rt, color, dash, gap, width);
        DashedLine(rt, rb, color, dash, gap, width);
        DashedLine(rb, lb, color, dash, gap, width);
        DashedLine(lb, lt, color, dash, gap, width);
    }

    void Line(const Vec2F& a, const Vec2F& b, const Color4& color, float width /*= 1.0f*/)
    {
        if (color.a == 0)
            return;

        PipelineDraw::Line(a, b, width, color);
    }

    void Shadow(const RectF& rect, float radius, const Color4& color, float blur)
    {
        PipelineDraw::Shadow(rect, PipelineDraw::Corners(radius), color, blur);
    }

    void Circle(const Vec2F& center, float radius, const Color4& color)
    {
        PipelineDraw::Circle(center, radius, color);
    }

    void Polygon(const Vector<Vec2F>& points, const Color4& color)
    {
        PipelineDraw::Polygon(points, color);
    }

    void Checker(const RectF& rect, float radius, float cell, const Color4& a, const Color4& b)
    {
        PipelineDraw::Checker(rect, PipelineDraw::Corners(radius), cell, a, b, false);
    }

    void Island(const RectF& rect, float radius, bool glass)
    {
        auto& t = T();
        // --shadow-pop: 0 12px 32px rgba(0,0,0,.45); the glass is --glass (the surface translucent), a popover --surface-solid
        Shadow(RectF(rect.left, rect.top - 10.0f, rect.right, rect.bottom - 10.0f), radius, t.dark ? Color4(0, 0, 0, 105) : Color4(38, 50, 56, 40), 28.0f);
        Color4 fill = glass ? (t.dark ? Color4(24, 26, 34, 235) : A(t.cardSolid, 0.94f)) : t.cardSolid;
        Fill(rect, radius, fill);
        Border(rect, radius, glass ? t.line : t.border);
        if (t.dark)
            Line(Vec2F(rect.left + radius, rect.top - 1.0f), Vec2F(rect.right - radius, rect.top - 1.0f), Color4(255, 255, 255, 20), 1.0f);
    }

    // --- text ---------------------------------------------------------------------------------

    void Text(const String& text, const RectF& rect, const TextStyle& style)
    {
        if (text.IsEmpty() || rect.Width() <= 0.0f)
            return;

        String font = FontOf(style.weight, style.mono);
        int height = PipelineTheme::Size(style.size);
        std::string key = std::string(text.Data()) + "\x01" + font.Data() + "\x01" + std::to_string(height) + "\x01" +
            std::to_string((int)style.hor) + std::to_string((int)style.ver) + (style.wrap ? "w" : "") + (style.dots ? "d" : "") +
            (style.wrap ? std::to_string((int)Math::Round(rect.Width())) : std::string()) + "\x01" + std::to_string((int)(style.lineHeight*10));

        auto& cache = Texts();
        auto it = cache.find(key);
        if (it == cache.end())
        {
            auto drawable = mmake<o2::Text>(font);
            drawable->height = height;
            drawable->horAlign = style.hor;
            drawable->verAlign = style.ver;
            drawable->wordWrap = style.wrap;
            drawable->dotsEngings = style.dots && !style.wrap;
            if (style.lineHeight > 0.0f)
            {
                if (auto f = drawable->GetFont())
                {
                    float natural = f->GetLineHeightPx(height);
                    drawable->linesDistanceCoef = natural > 0.0f ? style.lineHeight/natural : 1.0f;
                }
            }
            drawable->text = text;
            it = cache.emplace(key, CachedText{ drawable, 0 }).first;
        }

        auto& entry = it->second;
        entry.used = frameStamp;
        RectF current = entry.text->GetRect();
        if (!Math::Equals(current.left, rect.left) || !Math::Equals(current.right, rect.right) ||
            !Math::Equals(current.top, rect.top) || !Math::Equals(current.bottom, rect.bottom))
        {
            entry.text->SetRect(rect);
        }
        entry.text->color = style.color;
        entry.text->Draw();
    }

    float TextWidth(const String& text, float size, int weight /*= 400*/, bool mono /*= false*/)
    {
        if (text.IsEmpty())
            return 0.0f;

        String font = FontOf(weight, mono);
        int height = PipelineTheme::Size(size);
        std::string key = std::string(text.Data()) + "\x01" + font.Data() + "\x01" + std::to_string(height);
        auto& widths = Widths();
        auto it = widths.find(key);
        if (it != widths.end())
            return it->second;

        float width = text.Length()*size*0.55f;
        if (auto f = FontRef(font))
        {
            f->CheckCharacters(text, height);
            width = o2::Text::GetTextSize(text, f, height, Vec2F(), HorAlign::Left, VerAlign::Top, false).x;
        }
        if (widths.size() > 4000)
            widths.clear();
        widths[key] = width;
        return width;
    }

    float TextHeight(const String& text, float width, const TextStyle& style)
    {
        if (text.IsEmpty())
            return 0.0f;

        String font = FontOf(style.weight, style.mono);
        int height = PipelineTheme::Size(style.size);
        std::string key = std::string(text.Data()) + "\x01" + font.Data() + "\x01" + std::to_string(height) + "\x01" +
            std::to_string((int)Math::Round(width)) + "\x01" + std::to_string((int)(style.lineHeight*10));
        static auto* heights = new std::unordered_map<std::string, float>();
        auto hit = heights->find(key);
        if (hit != heights->end())
            return hit->second;
        if (heights->size() > 3000)
            heights->clear();

        auto f = FontRef(font);
        if (!f)
            return style.size*1.4f;

        f->CheckCharacters(text, height);
        float lines = o2::Text::GetTextSize(text, f, height, Vec2F(width, 0.0f), HorAlign::Left, VerAlign::Top, true).y;
        if (style.lineHeight > 0.0f)
        {
            float natural = f->GetLineHeightPx(height);
            if (natural > 0.0f)
            {
                int count = Math::Max(1, (int)Math::Round(lines/natural));
                (*heights)[key] = count*style.lineHeight;
                return count*style.lineHeight;
            }
        }
        (*heights)[key] = lines;
        return lines;
    }

    // --- icons ---------------------------------------------------------------------------------

    void Icon(const String& name, const Vec2F& center, float size, const Color4& color)
    {
        std::string key = std::string(name.Data()) + "|" + std::to_string((int)Math::Round(size*4));
        auto& cache = Icons();
        auto it = cache.find(key);
        if (it == cache.end())
            it = cache.emplace(key, CachedIcon{ mmake<VectorSprite>("ui/pipeline/" + name + ".svg"), 0 }).first;

        auto& entry = it->second;
        entry.used = frameStamp;
        entry.sprite->SetRect(RectF(center.x - size*0.5f, center.y + size*0.5f, center.x + size*0.5f, center.y - size*0.5f));
        entry.sprite->color = color;
        entry.sprite->Draw();
    }

    String TypeIcon(const String& typeWord)
    {
        static const Vector<String> known = { "screen", "group", "row", "column", "grid", "panel", "background", "image", "icon",
                                              "button", "text", "progress", "toggle", "slider", "input", "tab", "badge", "divider", "slot" };
        return "uiws_type_" + (known.Contains(typeWord) ? typeWord : String("group"));
    }

    String DeviceIcon(const String& kind)
    {
        static const Vector<String> known = { "phone", "tablet", "desktop", "reference", "custom" };
        return "uiws_dev_" + (known.Contains(kind) ? kind : String("design"));
    }

    // --- kit graphics ---------------------------------------------------------------------------

    TextureRef KitTexture(const UiDoc::UiKitElement& element, Vec2I& size)
    {
        auto& cache = Kits();
        std::string key = KitKey(element);
        auto it = cache.find(key);
        if (it == cache.end())
        {
            CachedKit entry;
            UiLayout::UiImage image;
            if (UiLayout::UiImage::Decode(PipelineUtils::Base64Decode(element.png), image) && !image.IsEmpty())
            {
                auto bitmap = image.ToBitmap();
                entry.texture = TextureRef(*bitmap);
                entry.size = Vec2I(image.w, image.h);
                entry.sprite = mmake<Sprite>();
                entry.sprite->SetTexture(entry.texture);
                entry.sprite->SetTextureSrcRect(RectI(Vec2I(), entry.size));
            }
            else
                entry.failed = true;

            if (cache.size() > 400)
                cache.clear();
            it = cache.emplace(key, entry).first;
        }

        it->second.used = frameStamp;
        size = it->second.size;
        return it->second.texture;
    }

    void KitThumb(const UiDoc::UiKitElement& element, const RectF& rect, float checkerCell /*= 3.0f*/, bool checker /*= true*/)
    {
        auto& t = T();
        if (checker)
            Checker(rect, 3.0f, checkerCell, t.checkerA, t.checkerB);

        Vec2I size;
        KitTexture(element, size);
        auto it = Kits().find(KitKey(element));
        if (it == Kits().end() || !it->second.sprite || size.x <= 0 || size.y <= 0)
            return;

        float scale = Math::Min(rect.Width()/size.x, rect.Height()/size.y);
        float w = size.x*scale, h = size.y*scale;
        Vec2F c = rect.Center();
        it->second.sprite->SetRect(RectF(c.x - w*0.5f, c.y + h*0.5f, c.x + w*0.5f, c.y - h*0.5f));
        it->second.sprite->transparency = 1.0f;
        it->second.sprite->Draw();
    }

    void Picture(const TextureRef& texture, const Vec2I& size, const RectF& rect, float alpha /*= 1.0f*/)
    {
        static Ref<Sprite>* sprite = new Ref<Sprite>(mmake<Sprite>());
        (*sprite)->SetTexture(texture);
        (*sprite)->SetTextureSrcRect(RectI(Vec2I(), size));
        (*sprite)->SetRect(rect);
        (*sprite)->transparency = alpha;
        (*sprite)->Draw();
    }

    void Sweep()
    {
        frameStamp++;
        auto sweep = [](auto& cache, int age, size_t limit)
        {
            if (cache.size() < limit)
                return;

            for (auto it = cache.begin(); it != cache.end();)
            {
                if (frameStamp - it->second.used > age)
                    it = cache.erase(it);
                else
                    ++it;
            }
        };
        sweep(Texts(), 90, 600);
        sweep(Icons(), 300, 300);
    }
}
