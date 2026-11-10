#include "o2Editor/stdafx.h"
#include "UiDocStructure.h"

#include <cmath>

#include "o2Editor/Pipeline/PipelineUtils.h"
#include "o2Editor/Pipeline/UiDoc/UiDocFp.h"

namespace Editor::UiDoc
{
    static double R2(double n)
    {
        return JsRound(n*100.0)/100.0;
    }

    // {x, y} / {width, height} / {horizontal, vertical}
    static Json Pair2(const char* a, const Json& va, const char* b, const Json& vb)
    {
        Json out = Json::Object();
        out[a] = va;
        out[b] = vb;
        return out;
    }

    static Json XY(double x, double y) { return Pair2("x", Json(x), "y", Json(y)); }
    static Json Size(double w, double h) { return Pair2("width", Json(w), "height", Json(h)); }
    static Json AnchorJson(const char* h, const char* v) { return Pair2("horizontal", Json(h), "vertical", Json(v)); }

    // {min, max, offsetMin, offsetMax}
    static Json Responsive(double minX, double minY, double maxX, double maxY, double oMinX, double oMinY, double oMaxX, double oMaxY)
    {
        Json out = Json::Object();
        out["min"] = XY(minX, minY);
        out["max"] = XY(maxX, maxY);
        out["offsetMin"] = XY(oMinX, oMinY);
        out["offsetMax"] = XY(oMaxX, oMaxY);
        return out;
    }

    // Placeholder colours of blocks the kit does not skin ("" = none)
    static String Placeholder(const String& type)
    {
        static const std::pair<const char*, const char*> colors[] = {
            { "background", "#2B2F3A" }, { "panel", "#3A4050" }, { "image", "#4A5060" }, { "icon", "#5A6275" }, { "button", "#5F6B85" },
            { "tab", "#4F5A70" }, { "progress", "#3A4050" }, { "toggle", "#5F6B85" }, { "slider", "#4A5060" }, { "input", "#F2F3F6" },
            { "badge", "#C04A4A" }, { "divider", "#8A92A5" }, { "slot", "#3A4050" },
        };
        for (auto& c : colors)
        {
            if (type == c.first)
                return c.second;
        }
        return "";
    }

    static Json AnchorMeta(const UiNode& n)
    {
        Anchor a = ParseAnchor(n.anchor);
        Json out = Json::Object();
        out["horizontal"] = a.h == "stretch" ? String("center") : a.h;
        out["vertical"] = a.v == "stretch" ? String("middle") : a.v;
        return out;
    }

    // The anchor preset as edge constraints (top-left origin, offsets in pixels at the design size)
    static Json ResponsiveOf(const UiNode& n, const UiRect& rel, const UiRect& parent)
    {
        Anchor a = ParseAnchor(n.anchor);
        struct Axis { double min, max, oMin, oMax; };
        auto axis = [](const String& mode, double pos, double size, double extent, const char* lo, const char* hi) {
            if (mode == "stretch")
                return Axis{ 0, 1, pos, pos + size - extent };
            double k = mode == lo ? 0 : mode == hi ? 1 : 0.5;
            return Axis{ k, k, pos - extent*k, pos + size - extent*k };
        };
        Axis h = axis(a.h, rel.x, rel.w, parent.w, "left", "right");
        Axis v = axis(a.v, rel.y, rel.h, parent.h, "top", "bottom");
        return Responsive(h.min, v.min, h.max, v.max, R2(h.oMin), R2(v.oMin), R2(h.oMax), R2(v.oMax));
    }

    // The layout group the schema can express for a block, null when it cannot
    static Json ExportableLayout(const UiNode& n, const std::vector<const UiNode*>& kids, const UiRect& box)
    {
        if (!n.layout || n.layout->IsEmpty())
            return Json();

        Box4 p = n.pad ? *n.pad : Box4{ 0, 0, 0, 0 };
        double t = p[0], r = p[1], b = p[2], l = p[3];
        if (t != b || l != r)
            return Json();
        Json padding = XY(l, t);

        if (*n.layout == "grid")
        {
            UiRect inner(0, 0, box.w - l - r, box.h - t - b);
            UiGridGeometry g = GridGeometry(n, inner, (int)kids.size());
            for (auto k : kids)
            {
                if ((k->w && *k->w != g.cw) || (k->h && *k->h != g.ch) || (k->x && *k->x != 0) || (k->y && *k->y != 0))
                    return Json();
            }
            Json out = Json::Object();
            out["type"] = "grid";
            out["columns"] = g.cols;
            out["cellSize"] = Size(R2(g.cw), R2(g.ch));
            out["spacing"] = XY(g.gx, g.gy);
            out["padding"] = padding;
            String align = n.align ? *n.align : String(), justify = n.justify ? *n.justify : String();
            out["align"] = align == "center" ? "center" : align == "end" ? "end" : "start";
            out["vAlign"] = justify == "center" ? "center" : justify == "end" ? "end" : "start";
            return out;
        }

        bool row = *n.layout == "row";
        for (auto k : kids)
        {
            if ((k->grow && *k->grow != 0) || (k->x && *k->x != 0) || (k->y && *k->y != 0))
                return Json();
        }
        String align = n.align ? *n.align : String("stretch");
        if (align == "stretch")
        {
            for (auto k : kids)
            {
                if (!(row ? k->h : k->w))
                    return Json();
            }
        }
        String j = n.justify ? *n.justify : String("start");
        String main = j == "between" ? String("space-between") : j == "around" ? String("space-around") : j == "evenly" ? String("space-evenly") : j;

        Json out = Json::Object();
        out["type"] = row ? "row" : "column";
        out["spacing"] = n.gap ? *n.gap : 0.0;
        out["padding"] = padding;
        out["align"] = main;
        out["vAlign"] = align == "stretch" ? String("start") : align;
        return out;
    }

    // Copies the members of src into dst (Object.assign / spread)
    static void Assign(Json& dst, const Json& src)
    {
        for (auto& m : src.Members())
            dst[m.first] = m.second;
    }

    namespace
    {
        // The state of one conversion: the solved boxes, the graphics and fonts used
        struct Converter
        {
            const UiDoc&                     doc;
            const UiKit*                     kit;
            UiRects                          rects;
            Vector<const UiKitElement*>      used;
            Vector<String>                   usedNames;
            Vector<String>                   fonts;
            Vector<String>                   warnings;

            // A label's text block (owner: the block it is the label of)
            Json TextOf(const UiNode& n, const UiNode* owner, const UiRect& rect, const String& name, const String& text, double padX = 0)
            {
                TextStyle style = owner ? StyleOf(n, kit, owner) : n.type == "progress" ? StyleForRole(n, kit, "number") : StyleOf(n, kit);
                UiTextFit fit = LayoutText(text, rect.w, rect.h, style, n.fontSize ? *n.fontSize : 0, padX);
                const UiFontFace& face = FaceOf(style.font, style.weight);
                if (!fonts.Contains(face.file))
                    fonts.Add(face.file);

                Json out = Json::Object();
                out["type"] = "text";
                out["name"] = name;
                out["text"] = style.uppercase ? JsToUpper(text) : text;
                out["fontFamily"] = face.file;
                out["fontSize"] = JsRound(fit.size);
                out["color"] = n.textColor ? *n.textColor : style.color;
                out["alignment"] = n.textAlign ? *n.textAlign : (owner ? String("center") : style.align);
                out["textVAlign"] = "middle";
                if (fit.lines.Count() > 1)
                    out["lineHeight"] = JsRound(fit.lineH);
                if (style.stroke)
                {
                    out["strokeColor"] = style.stroke->color;
                    out["strokeWidth"] = std::max(1.0, JsRound((style.stroke->width*fit.size)/style.size));
                }
                return out;
            }

            // The graphic a block draws: asset, nine-slice, tint
            Json Visual(const UiKitElement& el, const std::optional<String>& color)
            {
                String key = JsToLower(el.name);
                int at = usedNames.IndexOf(key);
                if (at < 0)
                {
                    usedNames.Add(key);
                    used.Add(&el);
                }
                else
                    used[at] = &el;

                Json out = Json::Object();
                out["asset"] = KitFileName(el);
                if (el.assetType == "sliced" && el.nineSlice)
                    out["nineSlice"] = NineToJson(*el.nineSlice);
                if (color && !color->IsEmpty())
                    out["color"] = *color;
                return out;
            }

            Json Convert(const UiNode& n, const UiNode& parent, const UiRect& parentRect, bool laid, bool crossH, bool crossV)
            {
                UiRect abs = rects.Get(n.id);
                UiRect rel(abs.x - parentRect.x, abs.y - parentRect.y, abs.w, abs.h);
                Json out = Json::Object();
                out["type"] = "container";
                out["name"] = n.id;
                out["position"] = XY(R2(rel.x), R2(rel.y));
                out["size"] = Size(R2(rel.w), R2(rel.h));
                out["anchor"] = AnchorMeta(n);

                if (!laid)
                {
                    if (parent.layout && !parent.layout->IsEmpty())
                    {
                        // A layout the schema cannot express: fixed boxes, stretched across where the layout stretched them
                        double maxX = 0, maxY = 0, oMaxX = R2(rel.x + rel.w), oMaxY = R2(rel.y + rel.h);
                        if (crossH)
                        {
                            maxX = 1;
                            oMaxX = R2(rel.x + rel.w - parentRect.w);
                        }
                        if (crossV)
                        {
                            maxY = 1;
                            oMaxY = R2(rel.y + rel.h - parentRect.h);
                        }
                        out["responsive"] = Responsive(0, 0, maxX, maxY, R2(rel.x), R2(rel.y), oMaxX, oMaxY);
                    }
                    else
                        out["responsive"] = ResponsiveOf(n, rel, parentRect);
                }
                if (!n.IsShown())
                    out["visible"] = false;
                if (n.opacity && *n.opacity < 1)
                    out["opacity"] = *n.opacity;

                UiSkin skin = SkinOf(n, kit);
                Json kids = Json::Array();
                UiRect box(0, 0, abs.w, abs.h);
                auto label = [&](const String& text, double padX) {
                    if (text.IsEmpty())
                        return;
                    Json lab = TextOf(n, &n, box, n.id + "_label", text, padX);
                    lab["position"] = XY(0, 0);
                    lab["size"] = Size(R2(abs.w), R2(abs.h));
                    lab["anchor"] = AnchorJson("center", "middle");
                    lab["responsive"] = Responsive(0, 0, 1, 1, 0, 0, 0, 0);
                    kids.Add(lab);
                };

                if (n.type == "group")
                {
                }
                else if (n.type == "text")
                {
                    Assign(out, TextOf(n, nullptr, box, n.id, n.text ? *n.text : String()));
                    out["name"] = n.id;
                    if (!n.text || n.text->IsEmpty())
                    {
                        out["type"] = "container";
                        out.Remove("text");
                    }
                }
                else if (n.type == "progress")
                {
                    Json progress = Json::Object();
                    progress["min"] = 0;
                    progress["max"] = 1;
                    progress["value"] = n.value ? *n.value : 0.6;
                    progress["fill"] = n.id + "_fill";
                    progress["direction"] = "left-to-right";
                    out["progress"] = progress;

                    auto full = [&](Json& o) {
                        o["position"] = XY(0, 0);
                        o["size"] = Size(R2(abs.w), R2(abs.h));
                        o["anchor"] = AnchorJson("left", "top");
                    };
                    Json track = Json::Object();
                    track["type"] = skin.el ? "image" : "rect";
                    track["name"] = n.id + "_track";
                    full(track);
                    if (skin.el)
                        Assign(track, Visual(*skin.el, n.color));
                    else
                        track["color"] = Placeholder("progress");
                    kids.Add(track);

                    Json fill = Json::Object();
                    fill["type"] = skin.fill ? "image" : "rect";
                    fill["name"] = n.id + "_fill";
                    full(fill);
                    if (skin.fill)
                        Assign(fill, Visual(*skin.fill, std::nullopt));
                    else
                        fill["color"] = "#6FBF5A";
                    kids.Add(fill);

                    if (n.text && !n.text->IsEmpty())
                    {
                        Json lab = TextOf(n, nullptr, box, n.id + "_label", *n.text);
                        full(lab);
                        kids.Add(lab);
                    }
                }
                else
                {
                    if (skin.el)
                    {
                        out["type"] = n.type == "button" ? "button" : "image";
                        Assign(out, Visual(*skin.el, n.color));
                    }
                    else
                    {
                        out["type"] = n.type == "button" ? "button" : "rect";
                        String ph = Placeholder(n.type);
                        out["color"] = n.color ? *n.color : !ph.IsEmpty() ? ph : String("#4A5060");
                    }
                    if (n.type == "button" || n.type == "tab" || n.type == "badge")
                    {
                        double k = skin.el ? KitScale(doc, *skin.el) : 1;
                        double pad = skin.el && skin.el->nineSlice
                            ? std::min({ skin.el->nineSlice->left*k, skin.el->nineSlice->right*k, abs.w/4 }) : 0;
                        label(n.text ? *n.text : String(), pad);
                    }
                    if (n.type == "input")
                        label(n.text && !n.text->IsEmpty() ? *n.text : n.placeholder ? *n.placeholder : String(), 16);
                }

                if (n.type == "text" && !n.children.empty())
                    warnings.Add(n.id + ": a text block's children are not exported");

                if (!n.children.empty() && n.type != "text")
                {
                    std::vector<const UiNode*> shown;
                    for (auto& c : n.children)
                    {
                        if (c.IsShown())
                            shown.push_back(&c);
                    }
                    Json layout = ExportableLayout(n, shown, box);
                    if (!layout.IsNull())
                        out["layout"] = layout;

                    String align = n.align ? *n.align : String("stretch");
                    String nl = n.layout ? *n.layout : String();
                    bool row = nl == "row";
                    for (auto& c : n.children)
                    {
                        bool crossH = !nl.IsEmpty() && nl != "grid" && !row && align == "stretch" && !c.w;
                        bool crossV = !nl.IsEmpty() && nl != "grid" && row && align == "stretch" && !c.h;
                        kids.Add(Convert(c, n, abs, !layout.IsNull() && c.IsShown(), crossH, crossV));
                    }
                }
                if (kids.Count() > 0)
                    out["children"] = kids;
                return out;
            }
        };
    }

    UiStructureResult ToUiStructure(const UiDoc& doc, const UiKit* kit)
    {
        Converter c{ doc, kit, SolveLayout(doc) };
        UiRect rootRect = c.rects.Get("root");

        Json root = Json::Object();
        root["type"] = "container";
        root["name"] = "root";
        root["position"] = XY(0, 0);
        root["size"] = Size(doc.width, doc.height);
        root["anchor"] = AnchorJson("left", "top");

        const UiKitElement* rootSkin = nullptr;
        if (doc.root.kit && !doc.root.kit->IsEmpty() && kit)
        {
            String want = JsToLower(*doc.root.kit);
            for (auto& e : kit->elements)
            {
                if (JsToLower(e.name) == want)
                {
                    rootSkin = &e;
                    break;
                }
            }
        }

        Json rootKids = Json::Array();
        if (rootSkin)
        {
            c.usedNames.Add(JsToLower(rootSkin->name));
            c.used.Add(rootSkin);
            Json bg = Json::Object();
            bg["type"] = "image";
            bg["name"] = "screen_background";
            bg["asset"] = KitFileName(*rootSkin);
            bg["position"] = XY(0, 0);
            bg["size"] = Size(doc.width, doc.height);
            bg["anchor"] = AnchorJson("center", "middle");
            bg["responsive"] = Responsive(0, 0, 1, 1, 0, 0, 0, 0);
            rootKids.Add(bg);
        }

        std::vector<const UiNode*> shown;
        for (auto& ch : doc.root.children)
        {
            if (ch.IsShown())
                shown.push_back(&ch);
        }
        Json rootLayout = ExportableLayout(doc.root, shown, rootRect);
        if (!rootLayout.IsNull() && !rootSkin)
            root["layout"] = rootLayout;
        bool laidOut = root.Has("layout");
        for (auto& ch : doc.root.children)
            rootKids.Add(c.Convert(ch, doc.root, rootRect, laidOut && ch.IsShown(), false, false));
        if (rootKids.Count() > 0)
            root["children"] = rootKids;

        Json structure = Json::Object();
        Json canvas = Json::Object();
        canvas["width"] = doc.width;
        canvas["height"] = doc.height;
        canvas["name"] = doc.name;
        structure["canvas"] = canvas;
        structure["root"] = root;
        Json metadata = Json::Object();
        metadata["notes"] = "AssetsLine AI UI layout";
        metadata["approximations"] = Json::Array();
        structure["metadata"] = metadata;

        UiStructureResult result;
        for (auto e : c.used)
        {
            UiStructureSprite sprite;
            sprite.file = KitFileName(*e);
            sprite.png = PipelineUtils::Base64Decode(e->png);
            sprite.width = e->width;
            sprite.height = e->height;
            double k = KitScale(doc, *e);
            if (k != 1)
            {
                // Sprite pixels per design pixel: the exporters draw its borders at the layout's density
                sprite.density = 1/k;
                sprite.dpi = GraphicDpi(e->dpi);
            }
            if (e->assetType == "sliced" && e->nineSlice)
                sprite.nineSlice = e->nineSlice;
            result.sprites.Add(sprite);
        }
        result.structure = structure;
        result.json = structure.Dump(2);
        result.fonts = c.fonts;
        result.warnings = c.warnings;
        return result;
    }
}
