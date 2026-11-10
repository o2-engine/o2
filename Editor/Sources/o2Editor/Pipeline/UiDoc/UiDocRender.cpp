#include "o2Editor/stdafx.h"
#include "UiDocRender.h"

#include <cmath>
#include <map>
#include <mutex>

#include "o2Editor/Pipeline/PipelineUtils.h"
#include "o2Editor/Pipeline/UiDoc/UiDocFonts.h"
#include "o2Editor/Pipeline/UiDoc/UiDocRenderer.h"
#include "o2Editor/Pipeline/UiDoc/UiDocFp.h"

namespace Editor::UiDoc::Render
{
    const char* const INK = "#566074";
    const char* const INK_SOFT = "#9BA3B2";
    const char* const PAPER = "#F3F4F7";
    const char* const FILL = "#E2E5EC";
    const char* const FILL2 = "#D2D7E0";
    const char* const WHITE = "#FFFFFF";

    // --- kit pictures ---------------------------------------------------------------------------------------

    namespace
    {
        struct PictureCache
        {
            std::mutex                                               lock;
            std::map<std::string, std::shared_ptr<const UiPicture>> pictures;
        };

        PictureCache& Pictures()
        {
            static PictureCache cache;
            return cache;
        }

        // A key for a picture without reading all of it: its length and samples along it
        std::string PictureKey(const UiKitElement& el)
        {
            const std::string& png = el.png;
            std::string key = (std::string)el.name + "|" + std::to_string(png.size()) + "|";
            size_t step = std::max<size_t>(1, png.size()/64);
            for (size_t i = 0; i < png.size(); i += step)
                key += png[i];
            if (!png.empty())
                key += png.substr(png.size() - std::min<size_t>(32, png.size()));
            return key;
        }
    }

    std::shared_ptr<const UiPicture> KitPicture(const UiKitElement& el)
    {
        std::string key = PictureKey(el);
        {
            std::lock_guard<std::mutex> guard(Pictures().lock);
            auto it = Pictures().pictures.find(key);
            if (it != Pictures().pictures.end())
                return it->second;
        }

        UiImage img;
        std::shared_ptr<const UiPicture> pic;
        if (UiImage::Decode(PipelineUtils::Base64Decode(el.png), img))
            pic = std::make_shared<UiPicture>(UiPicture::Of(img));

        std::lock_guard<std::mutex> guard(Pictures().lock);
        if (Pictures().pictures.size() > 256)
            Pictures().pictures.clear();
        Pictures().pictures[key] = pic;
        return pic;
    }

    void ClearKitPictures()
    {
        std::lock_guard<std::mutex> guard(Pictures().lock);
        Pictures().pictures.clear();
    }

    // --- the renderer -------------------------------------------------------------------------------------------

    Renderer::Buffers& Renderer::ThreadBuffers()
    {
        thread_local Buffers buffers;
        return buffers;
    }

    Renderer::Renderer(const UiDoc& d, const UiRenderOptions& options):
        doc(d), o(options), buffers(ThreadBuffers()), layers(buffers.layers), mask(buffers.mask)
    {
        kitMode = o.mode == "kit";
        W = o.width > 0 ? o.width : doc.width;
        H = o.height > 0 ? o.height : doc.height;
        rects = o.rects ? *o.rects : SolveLayout(doc, W, H);
        dpi = DocDpi(doc);
        sw = std::max(1.5, std::min(W, H)/360);
        idSize = std::max(12.0, std::min(W, H)/54);
        RenderSize(doc, o, cw, ch);
        if (o.pixelLong > 0)
        {
            // Like <svg width height viewBox>: the screen fitted into the canvas, centred
            s = std::min(cw/W, ch/H);
            ox = (cw - W*s)/2;
            oy = (ch - H*s)/2;
        }
        else
            s = o.scale > 0 ? o.scale : 1;
    }

    void Renderer::PushLayer()
    {
        depth++;
        if ((int)layers.size() <= depth)
            layers.push_back(std::make_unique<UiCanvas>());
        layers[depth]->Reset(cw, ch);
    }

    void Renderer::PopLayer(float opacity)
    {
        UiCanvas& layer = *layers[depth];
        depth--;
        Top().Composite(layer, opacity);
    }

    void Renderer::FillShape(const UiShape& shape, const UiRgba& color, float opacity /*= 1*/)
    {
        mask.Fill(shape);
        Top().Paint(mask, color, opacity);
        mask.Clear();
    }

    // The stroke of a closed contour in pixels: a ring (outer and inner offsets) or dashes
    static void StrokeOutline(UiMask& m, const UiPath& path, const UiPath& outer, const UiPath& inner, double widthPx,
                              const std::vector<double>& dashesPx)
    {
        if (!dashesPx.empty())
        {
            m.Stroke(path, true, widthPx, dashesPx);
            return;
        }
        UiShape ring = { outer };
        if (!inner.empty())
            ring.push_back(Reversed(inner));
        m.Fill(ring);
    }

    static std::vector<double> Scaled(const std::vector<double>& v, double s)
    {
        std::vector<double> out;
        for (double d : v)
            out.push_back(d*s);
        return out;
    }

    void Renderer::RectEl(const UiRect& r, double rx, const UiRgba* fill, const Stroke* stroke)
    {
        double x = F(r.x), y = F(r.y), w = F(std::max(0.0, r.w)), h = F(std::max(0.0, r.h));
        double rr = rx ? F(std::min(rx, std::min(r.w/2, r.h/2))) : 0;
        double px = X(x), py = Y(y), pw = w*s, ph = h*s, pr = std::max(0.0, std::min(rr*s, std::min(pw/2, ph/2)));
        if (fill)
            FillShape({ RoundRectPath(px, py, pw, ph, pr) }, *fill);
        if (stroke && stroke->width > 0)
        {
            double hw = stroke->width*s/2;
            UiPath outer = RoundRectPath(px - hw, py - hw, pw + 2*hw, ph + 2*hw, pr > 0 ? pr + hw : 0);
            UiPath inner = pw > 2*hw && ph > 2*hw ? RoundRectPath(px + hw, py + hw, pw - 2*hw, ph - 2*hw, std::max(0.0, pr - hw)) : UiPath();
            StrokeOutline(mask, RoundRectPath(px, py, pw, ph, pr), outer, inner, stroke->width*s, Scaled(stroke->dashes, s));
            Top().Paint(mask, stroke->color);
            mask.Clear();
        }
    }

    void Renderer::CircleEl(double cx, double cy, double r, const UiRgba* fill, const Stroke* stroke)
    {
        double px = X(F(cx)), py = Y(F(cy)), pr = F(r)*s;
        if (fill)
            FillShape({ CirclePath(px, py, pr) }, *fill);
        if (stroke && stroke->width > 0)
        {
            double hw = stroke->width*s/2;
            StrokeOutline(mask, CirclePath(px, py, pr), CirclePath(px, py, pr + hw), pr > hw ? CirclePath(px, py, pr - hw) : UiPath(),
                          stroke->width*s, Scaled(stroke->dashes, s));
            Top().Paint(mask, stroke->color);
            mask.Clear();
        }
    }

    void Renderer::LineEl(double x1, double y1, double x2, double y2, const Stroke& stroke)
    {
        mask.Stroke({ { X(F(x1)), Y(F(y1)) }, { X(F(x2)), Y(F(y2)) } }, false, stroke.width*s, Scaled(stroke.dashes, s));
        Top().Paint(mask, stroke.color);
        mask.Clear();
    }

    void Renderer::PathEl(const UiPath& path, bool closed, const UiRgba* fill, const Stroke* stroke)
    {
        UiPath px;
        for (auto& p : path)
            px.push_back({ X(p.x), Y(p.y) });
        if (fill && closed)
            FillShape({ px }, *fill);
        if (stroke && stroke->width > 0)
        {
            mask.Stroke(px, closed, stroke->width*s, Scaled(stroke->dashes, s));
            Top().Paint(mask, stroke->color);
            mask.Clear();
        }
    }

    // --- kit pictures into boxes ------------------------------------------------------------------------------

    void Renderer::Sprite(const UiKitElement& el, const UiRect& r, const String& fit, const std::optional<String>& tint,
                          const UiRect* clip /*= nullptr*/)
    {
        if (r.w <= 0 || r.h <= 0 || el.width <= 0 || el.height <= 0)
            return;
        auto pic = KitPicture(el);
        if (!pic || pic->w <= 0 || pic->h <= 0)
            return;

        UiPictureDraw base;
        if (tint)
        {
            // The tint filter's matrix, with its numbers as the SVG writes them
            UiRgba t = UiColorOf(*tint);
            base.tinted = true;
            base.tint[0] = (float)F(std::round(t.r*255)/255.0);
            base.tint[1] = (float)F(std::round(t.g*255)/255.0);
            base.tint[2] = (float)F(std::round(t.b*255)/255.0);
        }

        // The picture is stretched to the element's width x height (its user units)
        double ux = el.width/pic->w, uy = el.height/pic->h;
        auto piece = [&](double vx, double vy, double vw, double vh, double dx, double dy, double dw, double dh, double scaleX,
                         double scaleY) {
            UiPictureDraw d = base;
            d.sx = ux*scaleX*s;
            d.sy = uy*scaleY*s;
            d.tx = X(dx) - vx*scaleX*s;
            d.ty = Y(dy) - vy*scaleY*s;
            d.cx0 = X(dx); d.cy0 = Y(dy); d.cx1 = X(dx + dw); d.cy1 = Y(dy + dh);
            if (clip)
            {
                d.cx0 = std::max(d.cx0, X(clip->x)); d.cy0 = std::max(d.cy0, Y(clip->y));
                d.cx1 = std::min(d.cx1, X(clip->x + clip->w)); d.cy1 = std::min(d.cy1, Y(clip->y + clip->h));
            }
            Top().Draw(*pic, d);
        };

        // The graphic's own density against the layout's: its corners and tiles come out at the size they were drawn for
        double k = dpi/GraphicDpi(el.dpi);
        if (el.assetType == "sliced" && el.nineSlice)
        {
            double L = el.nineSlice->left, T = el.nineSlice->top, R = el.nineSlice->right, B = el.nineSlice->bottom;
            double Wd = el.width, Hd = el.height;
            // A box smaller than the corners: the corners shrink together
            double kx = (L + R)*k > r.w ? r.w/(L + R) : k, ky = (T + B)*k > r.h ? r.h/(T + B) : k;
            double xs[4] = { 0, L, Wd - R, Wd }, ys[4] = { 0, T, Hd - B, Hd };
            double dxs[4] = { r.x, r.x + L*kx, r.x + r.w - R*kx, r.x + r.w };
            double dys[4] = { r.y, r.y + T*ky, r.y + r.h - B*ky, r.y + r.h };
            // Each piece reaches a little under its right / bottom neighbour, so no hairline seam shows between them
            double ov = std::max(0.6, std::min(r.w, r.h)/400);
            for (int j = 0; j < 3; j++)
            {
                for (int i = 0; i < 3; i++)
                {
                    double swd = xs[i + 1] - xs[i], shd = ys[j + 1] - ys[j];
                    double tw = dxs[i + 1] - dxs[i], th = dys[j + 1] - dys[j];
                    if (swd <= 0 || shd <= 0 || tw <= 0.01 || th <= 0.01)
                        continue;
                    if (i < 2 && dxs[i + 2] - dxs[i + 1] > 0.01)
                        tw += ov;
                    if (j < 2 && dys[j + 2] - dys[j + 1] > 0.01)
                        th += ov;
                    double dx = F(dxs[i]), dy = F(dys[j]), dw = F(tw), dh = F(th);
                    piece(xs[i], ys[j], swd, shd, dx, dy, dw, dh, dw/swd, dh/shd);
                }
            }
            return;
        }

        if (el.assetType == "tiled")
        {
            // A pattern from the screen's origin: tiles of the picture at its density
            double kr = F(k);
            UiPictureDraw d = base;
            d.sx = ux*kr*s;
            d.sy = uy*kr*s;
            d.tx = ox;
            d.ty = oy;
            d.repeat = true;
            d.cx0 = X(F(r.x)); d.cy0 = Y(F(r.y)); d.cx1 = X(F(r.x) + F(r.w)); d.cy1 = Y(F(r.y) + F(r.h));
            if (clip)
            {
                d.cx0 = std::max(d.cx0, X(clip->x)); d.cy0 = std::max(d.cy0, Y(clip->y));
                d.cx1 = std::min(d.cx1, X(clip->x + clip->w)); d.cy1 = std::min(d.cy1, Y(clip->y + clip->h));
            }
            Top().Draw(*pic, d);
            return;
        }

        double vx = F(r.x), vy = F(r.y), vw = F(r.w), vh = F(r.h);
        if (fit == "stretch")
        {
            piece(0, 0, el.width, el.height, vx, vy, vw, vh, vw/el.width, vh/el.height);
            return;
        }

        // xMidYMid meet (contain) or slice (cover), clipped to the box
        double sc = fit == "cover" ? std::max(vw/el.width, vh/el.height) : std::min(vw/el.width, vh/el.height);
        double offX = (vw - el.width*sc)/2, offY = (vh - el.height*sc)/2;
        UiPictureDraw d = base;
        d.sx = ux*sc*s;
        d.sy = uy*sc*s;
        d.tx = X(vx + offX);
        d.ty = Y(vy + offY);
        d.cx0 = X(vx); d.cy0 = Y(vy); d.cx1 = X(vx + vw); d.cy1 = Y(vy + vh);
        if (clip)
        {
            d.cx0 = std::max(d.cx0, X(clip->x)); d.cy0 = std::max(d.cy0, Y(clip->y));
            d.cx1 = std::min(d.cx1, X(clip->x + clip->w)); d.cy1 = std::min(d.cy1, Y(clip->y + clip->h));
        }
        Top().Draw(*pic, d);
    }

    // --- the tree ----------------------------------------------------------------------------------------------

    void Renderer::Node(const UiNode& n)
    {
        if (!n.IsShown())
            return;
        const UiRect* found = rects.Find(n.id);
        if (!found)
            return;
        UiRect r = *found;

        bool layer = n.opacity && *n.opacity < 1;
        if (layer)
            PushLayer();

        const UiKit* kit = o.kit;
        if (n.type != "screen" && n.type != "group")
        {
            if (kitMode)
            {
                UiSkin skin = SkinOf(n, kit);
                if (skin.el || n.type == "text")
                    Skinned(n, r, skin);
                else
                    LineArt(n, r, skin.missing);
            }
            else
                LineArt(n, r, false);
        }
        else if (n.type == "screen" && n.kit && !n.kit->IsEmpty() && kitMode)
        {
            if (const UiKitElement* el = KitElement(kit, *n.kit))
                Sprite(*el, r, "cover", n.color);
        }
        else if (n.type == "group" && !kitMode)
            LineArt(n, r, false);

        for (auto& c : n.children)
            Node(c);

        if (o.showIds && n.type != "screen")
            PlainText(n.id, "Roboto-Regular.ttf", F(idSize), F(r.x + 3), F(r.y + idSize), UiColorOf("#7C6FF0"), 0.9f);

        if (layer)
            PopLayer((float)F(*n.opacity));
    }

    void Renderer::LineArt(const UiNode& n, const UiRect& r, bool missing)
    {
        Stroke stroke{ UiColorOf(missing ? "#E0803A" : INK), F(sw), {} };
        if (missing)
            stroke.dashes = { F(sw*4), F(sw*3) };
        Stroke soft{ UiColorOf(INK_SOFT), F(sw*0.8), {} };
        auto fillOf = [&](const char* d) { return UiColorOf(n.color ? *n.color : String(d)); };
        const UiKit* kit = o.kit;
        auto label = [&](const std::optional<String>& text, const char* role, const char* align = "center", double padX = 0,
                         const char* color = INK) {
            if (!text || text->IsEmpty())
                return;
            TextBox t;
            t.text = *text;
            t.rect = r;
            t.style = StyleForRole(n, kit, role);
            t.style.stroke.reset();
            t.style.shadow.reset();
            t.style.gradient.reset();
            t.align = n.textAlign ? *n.textAlign : String(align);
            t.color = n.textColor ? *n.textColor : String(color);
            t.size = n.fontSize ? *n.fontSize : 0;
            t.plain = true;
            t.padX = padX;
            Text(t);
        };
        auto cross = [&]() {
            LineEl(r.x, r.y, r.x + r.w, r.y + r.h, soft);
            LineEl(r.x + r.w, r.y, r.x, r.y + r.h, soft);
        };
        double rr = std::min(r.w, r.h);
        const String& type = n.type;

        if (type == "background")
        {
            UiRgba f = fillOf(FILL);
            RectEl(r, 0, &f, nullptr);
            cross();
        }
        else if (type == "panel")
        {
            UiRgba f = fillOf("#EBEDF2");
            RectEl(r, std::max(6.0, rr*0.06), &f, &stroke);
        }
        else if (type == "image")
        {
            UiRgba f = fillOf(FILL2);
            RectEl(r, 4, &f, &stroke);
            cross();
        }
        else if (type == "icon")
        {
            double d = rr*0.9;
            UiRgba f = fillOf(FILL2);
            CircleEl(r.x + r.w/2, r.y + r.h/2, d/2, &f, &stroke);
            double cx = F(r.x + r.w/2), cy = F(r.y + r.h/2), arm = F(d*0.44);
            LineEl(F(r.x + r.w/2 - d*0.22), cy, F(r.x + r.w/2 - d*0.22) + arm, cy, soft);
            LineEl(cx, F(r.y + r.h/2 - d*0.22), cx, F(r.y + r.h/2 - d*0.22) + arm, soft);
        }
        else if (type == "button")
        {
            UiRgba f = fillOf("#CDD3DE");
            RectEl(r, std::min(r.h/2, 22.0), &f, &stroke);
            label(n.text, "button");
        }
        else if (type == "tab")
        {
            UiPath p = { { F(r.x), F(r.y + r.h) }, { F(r.x), F(r.y + 12) } };
            QuadTo(p, { F(r.x), F(r.y) }, { F(r.x + 12), F(r.y) });
            p.push_back({ F(r.x + r.w - 12), F(r.y) });
            QuadTo(p, { F(r.x + r.w), F(r.y) }, { F(r.x + r.w), F(r.y + 12) });
            p.push_back({ F(r.x + r.w), F(r.y + r.h) });
            UiRgba f = fillOf(n.on && *n.on ? "#BFC7D6" : FILL);
            PathEl(p, true, &f, &stroke);
            label(n.text, "button");
        }
        else if (type == "text")
        {
            TextStyle st = StyleOf(n, kit);
            if (n.text && !n.text->IsEmpty())
            {
                TextBox t;
                t.text = *n.text;
                t.rect = r;
                t.style = st;
                t.style.stroke.reset();
                t.style.shadow.reset();
                t.style.gradient.reset();
                t.align = n.textAlign ? *n.textAlign : st.align;
                t.color = n.textColor ? *n.textColor : String(INK);
                t.size = n.fontSize ? *n.fontSize : 0;
                t.plain = true;
                Text(t);
            }
            else
            {
                double lh = std::min(r.h, 18.0);
                UiRgba soft2 = UiColorOf(INK_SOFT);
                for (double y = r.y + lh/2; y < r.y + r.h - lh/3; y += lh*1.6)
                {
                    double x = F(r.x), yy = F(y), w = F(r.w*0.8), h = F(lh/2), rx = F(lh/4);
                    FillShape({ RoundRectPath(X(x), Y(yy), w*s, h*s, rx*s) }, soft2);
                }
            }
        }
        else if (type == "progress")
        {
            double v = n.value ? *n.value : 0.6;
            UiRgba f = fillOf(FILL);
            RectEl(r, r.h/2, &f, &stroke);
            double inset = sw*2;
            UiRgba bar = UiColorOf("#97A3BA");
            RectEl(UiRect(r.x + inset, r.y + inset, std::max(0.0, (r.w - inset*2)*v), r.h - inset*2), (r.h - inset*2)/2, &bar, nullptr);
            label(n.text, "number");
        }
        else if (type == "toggle")
        {
            bool on = n.on && *n.on;
            UiRgba f = fillOf(on ? "#AEB9CE" : FILL);
            RectEl(r, r.h/2, &f, &stroke);
            UiRgba white = UiColorOf(WHITE);
            CircleEl(on ? r.x + r.w - r.h/2 : r.x + r.h/2, r.y + r.h/2, r.h*0.38, &white, &stroke);
        }
        else if (type == "slider")
        {
            double v = n.value ? *n.value : 0.5;
            double cy = r.y + r.h/2;
            UiRgba f = fillOf(FILL2);
            RectEl(UiRect(r.x, cy - r.h*0.12, r.w, r.h*0.24), r.h*0.12, &f, &stroke);
            UiRgba white = UiColorOf(WHITE);
            CircleEl(r.x + r.w*v, cy, r.h*0.42, &white, &stroke);
        }
        else if (type == "input")
        {
            UiRgba white = UiColorOf(WHITE);
            RectEl(r, 8, &white, &stroke);
            bool hasText = n.text && !n.text->IsEmpty();
            label(hasText ? n.text : n.placeholder, "body", "left", std::min(24.0, r.w*0.06), hasText ? INK : INK_SOFT);
        }
        else if (type == "badge")
        {
            UiRgba f = fillOf("#AEB6C5");
            CircleEl(r.x + r.w/2, r.y + r.h/2, rr/2, &f, &stroke);
            label(n.text, "number", "center", 0, WHITE);
        }
        else if (type == "divider")
        {
            if (r.w >= r.h)
                LineEl(r.x, r.y + r.h/2, r.x + r.w, r.y + r.h/2, stroke);
            else
                LineEl(r.x + r.w/2, r.y, r.x + r.w/2, r.y + r.h, stroke);
        }
        else if (type == "slot")
        {
            UiRgba f = fillOf(FILL);
            RectEl(r, std::max(4.0, rr*0.1), &f, &stroke);
        }
        else if (type == "group" && o.showIds)
        {
            Stroke dashed{ UiColorOf(INK_SOFT), F(sw*0.6), { F(sw*2), F(sw*2) } };
            RectEl(r, 0, nullptr, &dashed);
        }
    }

    void Renderer::Skinned(const UiNode& n, const UiRect& r, const UiSkin& skin)
    {
        const UiKit* kit = o.kit;
        const UiKitElement* el = skin.el;
        String fit = n.fit ? *n.fit : n.type == "icon" || n.type == "image" ? String("contain") : n.type == "background" ? String("cover")
                                                                                                                    : String("stretch");
        if (n.type == "progress" && el)
        {
            Sprite(*el, r, "stretch", n.color);
            double v = n.value ? *n.value : 0.6;
            if (skin.fill && v > 0)
            {
                UiRect clip(F(r.x), F(r.y), F(r.w*v), F(r.h));
                Sprite(*skin.fill, r, "stretch", std::nullopt, &clip);
            }
            if (n.text && !n.text->IsEmpty())
            {
                TextBox t;
                t.text = *n.text;
                t.rect = r;
                t.style = StyleForRole(n, kit, "number");
                t.align = "center";
                if (n.textColor)
                    t.color = *n.textColor;
                Text(t);
            }
            return;
        }
        if (n.type == "slider" && el)
        {
            double cy = r.y + r.h/2;
            Sprite(*el, UiRect(r.x, cy - r.h*0.2, r.w, r.h*0.4), "stretch", n.color);
            if (skin.handle)
            {
                double hs = r.h;
                Sprite(*skin.handle, UiRect(r.x + r.w*(n.value ? *n.value : 0.5) - hs/2, cy - hs/2, hs, hs), "contain", std::nullopt);
            }
            return;
        }
        if (el)
            Sprite(*el, r, fit, n.color);

        // Labels on skinned blocks: the button's text inside its nine-slice centre
        double padX = 0;
        if (el && el->nineSlice)
        {
            double k = dpi/GraphicDpi(el->dpi);
            padX = std::min(std::min(el->nineSlice->left*k, el->nineSlice->right*k), r.w/4);
        }

        TextBox t;
        t.rect = r;
        if (n.textColor)
            t.color = *n.textColor;
        t.size = n.fontSize ? *n.fontSize : 0;
        if (n.type == "text")
        {
            TextStyle st = StyleOf(n, kit);
            if (n.text && !n.text->IsEmpty())
            {
                t.text = *n.text;
                t.style = st;
                t.align = n.textAlign ? *n.textAlign : st.align;
                Text(t);
            }
        }
        else if ((n.type == "button" || n.type == "tab" || n.type == "badge") && n.text && !n.text->IsEmpty())
        {
            t.text = *n.text;
            t.style = StyleOf(n, kit, &n);
            t.align = n.textAlign ? *n.textAlign : String("center");
            t.padX = padX;
            Text(t);
        }
        else if (n.type == "input")
        {
            bool hasText = n.text && !n.text->IsEmpty();
            String text = hasText ? *n.text : n.placeholder ? *n.placeholder : String();
            if (!text.IsEmpty())
            {
                t.text = text;
                t.style = StyleOf(n, kit, &n);
                if (!hasText)
                {
                    t.style.color = "#9AA2B1";
                    t.style.stroke.reset();
                    t.style.shadow.reset();
                }
                t.align = n.textAlign ? *n.textAlign : String("left");
                t.padX = std::max(padX, 16.0);
                Text(t);
            }
        }
    }

    void Renderer::Begin()
    {
        if (layers.empty())
            layers.push_back(std::make_unique<UiCanvas>());
        depth = 0;
        layers[0]->Reset(cw, ch);
        mask.Reset(cw, ch);
    }

    UiImage Renderer::Finish()
    {
        UiImage img = layers[0]->ToImage();
        // Full-size pictures (the node's output) do not keep their buffers: only card-sized ones are worth keeping
        if ((size_t)cw*ch > 1500000)
        {
            layers.clear();
            mask = UiMask();
        }
        return img;
    }

    void Renderer::FillPattern(const UiShape& shape, const std::function<UiRgba(int x, int y)>& colorAt)
    {
        mask.Fill(shape);
        UiCanvas& c = Top();
        for (int y = mask.y0; y < mask.y1; y++)
        {
            for (int x = mask.lo[y]; x < mask.hi[y]; x++)
            {
                float k = mask.a[(size_t)y*mask.w + x];
                if (k <= 0)
                    continue;
                UiRgba col = colorAt(x, y);
                float a = col.a*k;
                float* d = c.px.data() + ((size_t)y*c.w + x)*4;
                float inv = 1 - a;
                d[0] = col.r*a + d[0]*inv;
                d[1] = col.g*a + d[1]*inv;
                d[2] = col.b*a + d[2]*inv;
                d[3] = a + d[3]*inv;
            }
        }
        c.Touch(mask.x0, mask.y0, mask.x1, mask.y1);
        mask.Clear();
    }

    UiImage Renderer::Run()
    {
        Begin();

        if (!kitMode || !doc.root.kit || doc.root.kit->IsEmpty())
        {
            UiRgba paper = UiColorOf(!kitMode ? String(PAPER) : doc.root.color ? *doc.root.color : String("#20232B"));
            FillShape({ RectPath(X(0), Y(0), W*s, H*s) }, paper);
        }
        Node(doc.root);
        return Finish();
    }
}

namespace Editor::UiDoc
{
    void RenderSize(const UiDoc& doc, const UiRenderOptions& o, int& width, int& height)
    {
        double W = o.width > 0 ? o.width : doc.width, H = o.height > 0 ? o.height : doc.height;
        if (o.pixelLong > 0)
        {
            width = W >= H ? o.pixelLong : std::max(1, (int)JsRound(W*o.pixelLong/H));
            height = W >= H ? std::max(1, (int)JsRound(H*o.pixelLong/W)) : o.pixelLong;
            return;
        }
        double s = o.scale > 0 ? o.scale : 1;
        width = std::max(1, (int)JsRound(W*s));
        height = std::max(1, (int)JsRound(H*s));
    }

    UiImage RenderLayout(const UiDoc& doc, const UiRenderOptions& options)
    {
        Render::Renderer r(doc, options);
        return r.Run();
    }

    Ref<Bitmap> RenderLayoutBitmap(const UiDoc& doc, const UiRenderOptions& options)
    {
        return RenderLayout(doc, options).ToBitmap();
    }

    UiDoc TextStyleSampleDoc(const TextStyle& style, const String& text, bool dark)
    {
        double stroke = style.stroke ? style.stroke->width*2 : 0;
        double w = std::max(680.0, std::ceil(EstimateTextWidth(text, style)*1.03 + 36 + stroke*2 + 8));
        UiDoc d = EmptyDoc("Sample", w, 224);
        d.root.color = dark ? "#20232B" : "#F3F4F7";
        UiNode sample;
        sample.id = "sample";
        sample.type = "text";
        sample.anchor = "stretch";
        sample.margin = Box4{ 12, 18, 12, 18 };
        sample.style = style.name;
        sample.text = text;
        sample.keys = { "anchor", "margin", "style", "text" };
        d.root.SetChildren({ sample });
        return d;
    }

    UiImage RenderTextStyleSample(const TextStyle& style, const String& text, bool dark, double scale /*= 1*/)
    {
        UiDoc doc = TextStyleSampleDoc(style, text, dark);
        UiKit kit;
        kit.name = "k";
        kit.textStyles = { style };
        UiRenderOptions o;
        o.mode = "kit";
        o.kit = &kit;
        o.scale = scale;
        return RenderLayout(doc, o);
    }

    void ClearUiRenderCache()
    {
        Render::ClearKitPictures();
        ClearUiGlyphCache();
    }
}
