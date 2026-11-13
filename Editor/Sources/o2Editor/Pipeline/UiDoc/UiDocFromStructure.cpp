#include "o2Editor/stdafx.h"
#include "UiDocFromStructure.h"

#include <cmath>
#include <cstdlib>
#include <functional>
#include <map>
#include <set>

#include "o2Editor/Pipeline/UiDoc/UiDocEdit.h"
#include "o2Editor/Pipeline/UiDoc/UiDocRotate.h"
#include "o2Editor/Pipeline/UiDoc/UiDocFp.h"

namespace Editor::UiDoc
{
    namespace
    {
        // A block that draws no kit graphic on purpose (render.ts KIT_NONE)
        const char* const KIT_NONE_NAME = "none";

        // num(v, d): Number(v) when it is finite, else d; null and '' count as missing
        double Num(const Json* v, double d = 0.0)
        {
            if (v && (v->IsNull() || (v->IsString() && v->Str().IsEmpty())))
                return d;
            double n = JsNumber(v);
            return std::isfinite(n) ? n : d;
        }

        // The children of a structure element that are elements (objects); anything else gives none
        Vector<const Json*> KidsOf(const Json& s)
        {
            Vector<const Json*> out;
            if (const Json* list = JsGet(s, "children"); list && list->IsArray())
            {
                for (auto& k : list->Items())
                {
                    if (k.IsObject())
                        out.Add(&k);
                }
            }
            return out;
        }

        bool Near(double a, double b, double tol = 2.0)
        {
            return std::abs(a - b) <= tol;
        }

        // v === text
        bool StrIs(const Json* v, const char* text)
        {
            return v && v->IsString() && v->Str() == text;
        }

        // cleanColor(v), "" for none
        String CleanColorOf(const Json* v)
        {
            String out;
            return CleanColor(v, out) ? out : String();
        }

        bool RgbOf(const String& hex, int out[3])
        {
            Json v(hex);
            String c = CleanColorOf(&v);
            if (c.IsEmpty())
                return false;

            for (int i = 0; i < 3; i++)
                out[i] = (int)std::strtol(((std::string)c.SubStr(1 + i*2, 3 + i*2)).c_str(), nullptr, 16);
            return true;
        }

        // How far two colours are; both missing = 0, one missing = 255
        double ColorDist(const String& a, const String& b)
        {
            int x[3], y[3];
            bool hx = RgbOf(a, x), hy = RgbOf(b, y);
            if (!hx || !hy)
                return hx == hy ? 0.0 : 255.0;

            double d0 = x[0] - y[0], d1 = x[1] - y[1], d2 = x[2] - y[2];
            return std::sqrt(d0*d0 + d1*d1 + d2*d2);
        }

        // /^[\d\s.,:+\-×x%/]+$/
        bool IsNumberText(const String& text)
        {
            std::u32string cps = Utf8ToCodePoints(text);
            if (cps.empty())
                return false;

            for (char32_t c : cps)
            {
                bool ok = (c >= U'0' && c <= U'9') || IsJsSpace(c) || c == U'.' || c == U',' || c == U':' || c == U'+' || c == U'-' ||
                          c == 0xD7 || c == U'x' || c == U'%' || c == U'/';
                if (!ok)
                    return false;
            }
            return true;
        }

        // /(^|_)(btn|button|tab)(_|$)/i
        bool IsButtonName(const String& name)
        {
            return JsSplit(name, "_").Any([](const String& part) {
                String p = JsToLower(part);
                return p == "btn" || p == "button" || p == "tab";
            });
        }

        // `children?.length` is truthy
        bool HasLength(const Json* v)
        {
            return v && (v->IsArray() ? v->Count() > 0 : v->IsString() ? !v->Str().IsEmpty() : v->IsObject() && JsTruthy(v->Get("length")));
        }

        // The catalogue font and weight a `fontFamily` file names (Roboto when it is none of ours)
        void FontOfFile(const Json* file, String& font, double& weight)
        {
            String f = JsToLower(JsSplit(JsStringOr(file).ReplacedAll("\\", "/"), "/").Last());
            for (auto& entry : UiFonts())
            {
                for (auto& face : entry.faces)
                {
                    if (JsToLower(face.file) == f)
                    {
                        font = entry.family;
                        weight = face.weight;
                        return;
                    }
                }
            }

            font = "Roboto";
            weight = f.Contains("bold") || f.Contains("black") || f.Contains("heavy") ? 700 : f.Contains("medium") ? 500 : 400;
        }

        // A label's look as the structure gives it
        struct TextLook
        {
            String font, color, strokeColor, role;
            double weight = 400, size = 32, strokeWidth = 0;
            bool   hasStroke = false;
        };

        // How far a style is from a label's look: 0 = the same; over 1 = not this style
        double StyleDistance(const TextStyle& s, const TextLook& t)
        {
            double d = 0;
            if (s.font != t.font)
                d += 0.6;
            else if (s.weight != t.weight)
                d += 0.15;
            d += std::min(1.0, ColorDist(s.color, t.color)/120)*0.6;
            if (s.stroke.has_value() != t.hasStroke)
                d += 0.35;
            else if (s.stroke && t.hasStroke)
                d += std::min(1.0, ColorDist(s.stroke->color, t.strokeColor)/160)*0.25;
            double ratio = std::max(s.size, t.size)/std::max(1.0, std::min(s.size, t.size));
            d += std::min(0.5, (ratio - 1)*0.25);
            if (s.role == t.role)
                d -= 0.1;
            return d;
        }

        // Members of `from` written over `to`, as Object.assign does
        void Assign(Json& to, const Json& from)
        {
            for (auto& m : from.Members())
                to[m.first] = m.second;
        }

        bool IsHidden(const Json* node)
        {
            const Json* v = JsGet(node, "visible");
            return v && v->IsBool() && !v->Bool();
        }

        // The props that put the node at r inside p with the anchor, sizes written (placeAt(...).set assigned)
        void Place(Json& node, const UiRect& p, const UiRect& r, const Anchor& a)
        {
            Json own = node;
            own.Remove("children");
            Assign(node, PlaceAt(NodeFromJson(own), p, r, &a, true).set);
        }

        // pad(t, r, b, l): rounded, not negative; none when all are 0
        void SetPad(Json& node, double t, double r, double b, double l)
        {
            Json pad = Json::Array();
            for (double v : { t, r, b, l })
                pad.Add(Json(std::max(0.0, JsRound(v))));
            if (pad.Items().Any([](const Json& x) { return x.Num() > 0; }))
                node["pad"] = pad;
            else
                node.Remove("pad");
        }

        // ----------------------------------------------------------
        // One block built from the structure: its node, box and source
        // ----------------------------------------------------------
        struct Built
        {
            Json        node;
            UiRect      rect;
            const Json* s = nullptr;
        };

        // ---------------------------------------------------------------------------------
        // The conversion's state: kit lookups, ids taken, styles, counts and the skipped keys
        // ---------------------------------------------------------------------------------
        class StructureReader
        {
        public:
            StructureReader(const FromStructureInput& input): mInput(input)
            {
                if (!mInput.kit)
                    return;

                for (auto& e : mInput.kit->elements)
                    mByFile[(std::string)JsToLower(e.name + ".png")] = &e;
                for (auto& entry : mInput.assetNames)
                {
                    if (auto el = KitElement(mInput.kit, entry.second))
                        mByFile[(std::string)JsToLower(entry.first)] = el;
                }
                mStyles = mInput.kit->textStyles;
            }

            FromStructureResult Run()
            {
                const Json empty = Json::Object();
                const Json& st = JsIsObjectLike(&mInput.structure) ? mInput.structure : empty;
                const Json* canvasValue = JsGet(st, "canvas");
                const Json& canvas = JsNullish(canvasValue) ? empty : *canvasValue;
                const Json* rootValue = JsGet(st, "root");
                const Json& rootS = JsNullish(rootValue) ? empty : *rootValue;
                double W = JsRound(Num(JsGet(canvas, "width"), Num(JsGet(JsGet(rootS, "size"), "width"), 1080)));
                double H = JsRound(Num(JsGet(canvas, "height"), Num(JsGet(JsGet(rootS, "size"), "height"), 1920)));
                Scan(rootS);

                UiRect rootRect(0, 0, W, H);
                Json root = Json::Object();
                root["id"] = "root";
                root["type"] = "screen";
                Vector<Built> top;
                for (auto k : KidsOf(rootS))
                    top.Add(Build(*k, "root/" + JsStringOr(JsGet(k, "name")), "root", rootRect, false));
                if (!top.IsEmpty())
                {
                    const Json* lay = JsGet(rootS, "layout");
                    String kind = JsIsObjectLike(lay) ? JsString(JsGet(lay, "type")) : String();
                    bool laid = false;
                    if (kind == "row" || kind == "column" || kind == "grid")
                    {
                        mResult.stats.layouts++;
                        laid = TryLayout(root, rootRect, top, kind, *lay);
                        if (!laid)
                            mResult.stats.layoutsFlattened++;
                    }
                    if (!laid)
                    {
                        for (auto& b : top)
                            Place(b.node, rootRect, b.rect, AnchorOf(*b.s, b.rect, rootRect, true));
                    }
                    Json children = Json::Array();
                    for (auto& b : top)
                        children.Add(b.node);
                    root["children"] = children;
                }

                auto& stats = mResult.stats;
                for (auto& s : mSkipped)
                    mResult.warnings.Add(s.first + ": not carried into the layout (" + JsNum(s.second) + ")");
                if (!stats.assetsMissing.IsEmpty())
                {
                    Vector<String> first;
                    for (int i = 0; i < stats.assetsMissing.Count() && i < 12; i++)
                        first.Add(stats.assetsMissing[i]);
                    mResult.warnings.Add("graphics the kit has no picture for: " + Join(first, ", "));
                }
                if (stats.layoutsFlattened)
                {
                    mResult.warnings.Add(JsNum(stats.layoutsFlattened) + " of " + JsNum(stats.layouts) +
                                         " row / column / grid group(s) kept as placed blocks (their spacing is not regular)");
                }

                Json doc = Json::Object();
                String name = CleanId(JsGet(canvas, "name"));
                doc["name"] = name.IsEmpty() ? String("Screen") : name;
                doc["width"] = W;
                doc["height"] = H;
                if (auto dpi = mInput.dpi ? CleanDpi(*mInput.dpi) : std::nullopt)
                    doc["dpi"] = *dpi;
                doc["root"] = root;
                mResult.doc = NormalizeDoc(doc);
                return mResult;
            }

        private:
            const FromStructureInput&                  mInput;
            std::map<std::string, const UiKitElement*> mByFile;
            std::set<std::string>                      mUsed = { "root" };
            Vector<TextStyle>                          mStyles;  // The kit's, then the ones made here
            double                                     mMaxFont = 0;
            Vector<std::pair<String, int>>             mSkipped; // Keys not carried, in first-seen order
            FromStructureResult                        mResult;

            void Skip(const String& what)
            {
                for (auto& s : mSkipped)
                {
                    if (s.first == what)
                    {
                        s.second++;
                        return;
                    }
                }
                mSkipped.Add({ what, 1 });
            }

            const UiKitElement* ElementOf(const Json* asset)
            {
                if (!asset || !asset->IsString() || asset->Str().IsEmpty())
                    return nullptr;

                const String& path = asset->Str();
                String file = StartsWith(path, "./") ? path.SubStr(2) : StartsWith(path, "/") ? path.SubStr(1) : path;
                file = JsToLower(JsSplit(file, "/").Last());

                auto it = mByFile.find((std::string)file);
                if (it == mByFile.end())
                    it = mByFile.find((std::string)(file + ".png"));
                const UiKitElement* el = it != mByFile.end() ? it->second : nullptr;
                if (!el && !mResult.stats.assetsMissing.Contains(path))
                    mResult.stats.assetsMissing.Add(path);
                return el;
            }

            bool Take(const String& id)
            {
                std::string key = JsToLower(id);
                if (id.IsEmpty() || mUsed.count(key))
                    return false;

                mUsed.insert(key);
                return true;
            }

            String IdFor(const Json* name, const String& parentId)
            {
                String base = CleanId(name);
                if (base.IsEmpty())
                    base = "block";
                if (Take(base))
                    return base;
                if (parentId != "root")
                {
                    String joined = JsSlice(parentId + "_" + base, 0, 64);
                    if (Take(joined))
                        return joined;
                }
                for (int i = 2; ; i++)
                {
                    String t = JsSlice(base, 0, 60) + "_" + JsNum(i);
                    if (Take(t))
                        return t;
                }
            }

            void Scan(const Json& s)
            {
                if (StrIs(JsGet(s, "type"), "text"))
                    mMaxFont = std::max(mMaxFont, Num(JsGet(s, "fontSize")));
                for (auto c : KidsOf(s))
                    Scan(*c);
            }

            String RoleOfText(const Json& s, bool inButton) const
            {
                double size = Num(JsGet(s, "fontSize"), 32);
                if (inButton)
                    return "button";
                if (IsNumberText(JsStringOr(JsGet(s, "text"))))
                    return "number";
                if (mMaxFont && size >= mMaxFont*0.92)
                    return "title";
                if (mMaxFont && size >= mMaxFont*0.6)
                    return "heading";
                if (mMaxFont && size <= mMaxFont*0.35)
                    return "caption";
                return "body";
            }

            Json TextProps(const Json& s, bool inButton)
            {
                TextLook look;
                FontOfFile(JsGet(s, "fontFamily"), look.font, look.weight);
                double strokeW = Num(JsGet(s, "strokeWidth"));
                look.size = std::max(6.0, JsRound(Num(JsGet(s, "fontSize"), 32)));
                look.color = CleanColorOf(JsGet(s, "color"));
                if (look.color.IsEmpty())
                    look.color = "#FFFFFF";
                if (strokeW > 0)
                {
                    look.hasStroke = true;
                    look.strokeColor = CleanColorOf(JsGet(s, "strokeColor"));
                    if (look.strokeColor.IsEmpty())
                        look.strokeColor = "#000000";
                    look.strokeWidth = strokeW;
                }
                look.role = RoleOfText(s, inButton);

                int best = -1;
                double bestD = INFINITY;
                for (int i = 0; i < mStyles.Count(); i++)
                {
                    double d = StyleDistance(mStyles[i], look);
                    if (d < bestD)
                    {
                        best = i;
                        bestD = d;
                    }
                }

                const Json* alignment = JsGet(s, "alignment");
                if (best < 0 || bestD > 0.55)
                {
                    std::set<std::string> taken;
                    for (auto& x : mStyles)
                        taken.insert(JsToLower(x.name));
                    String name = look.role;
                    for (int i = 2; taken.count(JsToLower(name)); i++)
                        name = look.role + "_" + JsNum(i);

                    Json raw = Json::Object();
                    raw["name"] = name;
                    raw["role"] = look.role;
                    raw["font"] = look.font;
                    raw["weight"] = look.weight;
                    raw["size"] = look.size;
                    raw["color"] = look.color;
                    if (look.hasStroke)
                    {
                        Json stroke = Json::Object();
                        stroke["color"] = look.strokeColor;
                        stroke["width"] = look.strokeWidth;
                        raw["stroke"] = stroke;
                    }
                    else
                        raw["stroke"] = Json(nullptr);
                    raw["align"] = StrIs(alignment, "left") || StrIs(alignment, "right") ? alignment->Str() : String("center");
                    raw["description"] = "read off the reference picture";

                    TextStyle made = NormalizeTextStyle(raw, name);
                    mStyles.Add(made);
                    mResult.styles.Add(made);
                    best = mStyles.Count() - 1;
                }

                const TextStyle& style = mStyles[best];
                Json out = Json::Object();
                out["style"] = style.name;
                if (std::abs(style.size - look.size)/look.size > 0.06)
                    out["fontSize"] = look.size;
                if (ColorDist(style.color, look.color) > 24)
                    out["textColor"] = look.color;
                bool aligned = StrIs(alignment, "left") || StrIs(alignment, "right") || StrIs(alignment, "center");
                if (aligned && alignment->Str() != style.align)
                    out["textAlign"] = alignment->Str();
                return out;
            }

            UiRect BoxOf(const Json& s, const String& path, const UiRect& parent) const
            {
                auto hit = mInput.boxes.find(path);
                if (hit != mInput.boxes.end())
                    return hit->second;

                const Json* pos = JsGet(s, "position");
                const Json* size = JsGet(s, "size");
                return UiRect(parent.x + Num(JsGet(pos, "x")), parent.y + Num(JsGet(pos, "y")), Num(JsGet(size, "width"), parent.w),
                              Num(JsGet(size, "height"), parent.h));
            }

            Anchor AnchorOf(const Json& s, const UiRect& r, const UiRect& p, bool fills) const
            {
                const Json* a = JsGet(s, "anchor");
                const Json* ah = JsGet(a, "horizontal");
                const Json* av = JsGet(a, "vertical");
                Anchor out;
                out.h = StrIs(ah, "center") || StrIs(ah, "right") ? ah->Str() : String("left");
                out.v = StrIs(av, "middle") || StrIs(av, "bottom") ? av->Str() : String("top");

                const Json* resp = JsGet(s, "responsive");
                const Json* rMin = JsGet(resp, "min");
                const Json* rMax = JsGet(resp, "max");
                if (JsTruthy(rMin) && JsTruthy(rMax))
                {
                    if (Num(JsGet(rMin, "x")) != Num(JsGet(rMax, "x")))
                        out.h = "stretch";
                    if (Num(JsGet(rMin, "y")) != Num(JsGet(rMax, "y")))
                        out.v = "stretch";
                }

                // What spans its parent keeps spanning it on another screen: a backdrop, a bar across the top
                if (fills)
                {
                    if (Near(r.x, p.x) && Near(r.w, p.w))
                        out.h = "stretch";
                    if (Near(r.y, p.y) && Near(r.h, p.h))
                        out.v = "stretch";
                }
                return out;
            }

            Built Build(const Json& s, const String& path, const String& parentId, const UiRect& parentRect, bool inButton)
            {
                UiRect rect = BoxOf(s, path, parentRect);
                Vector<const Json*> kids = KidsOf(s);
                for (const char* k : { "state", "scroll", "hueShift", "textScaleX", "role" })
                {
                    if (JsGet(s, k))
                        Skip(k);
                }

                Json node = Json::Object();
                node["id"] = IdFor(JsGet(s, "name"), parentId);
                node["type"] = "group";
                mResult.stats.blocks++;
                if (IsHidden(&s))
                    node["visible"] = false;
                double op = Num(JsGet(s, "opacity"), 1);
                std::set<int> absorbed;

                if (JsStringOr(JsGet(s, "type"), "container") == "text")
                {
                    node["type"] = "text";
                    node["text"] = JsStringOr(JsGet(s, "text"));
                    Assign(node, TextProps(s, inButton));
                    mResult.stats.texts++;
                }
                else
                    BuildLook(s, path, rect, kids, node, absorbed);

                if (op < 1)
                    node["opacity"] = std::max(0.0, op);
                if (auto turn = CleanTurn(JsGet(s, "rotation")))
                    node["rotation"] = *turn;

                // Children, in order
                String nodeType = JsStringOr(JsGet(node, "type"));
                bool inBtn = inButton || nodeType == "button" || nodeType == "tab";
                String id = JsStringOr(JsGet(node, "id"));
                Vector<Built> built;
                for (int i = 0; i < kids.Count(); i++)
                {
                    if (!absorbed.count(i))
                        built.Add(Build(*kids[i], path + "/" + JsStringOr(JsGet(*kids[i], "name")), id, rect, inBtn));
                }
                if (!built.IsEmpty())
                {
                    const Json* lay = JsGet(s, "layout");
                    String kind = JsIsObjectLike(lay) ? JsString(JsGet(lay, "type")) : String();
                    bool shown = true;
                    for (auto& b : built)
                        shown = shown && !IsHidden(&b.node);

                    bool laid = false;
                    if (absorbed.empty() && (kind == "row" || kind == "column" || kind == "grid") && shown)
                    {
                        mResult.stats.layouts++;
                        laid = TryLayout(node, rect, built, kind, *lay);
                        if (!laid)
                            mResult.stats.layoutsFlattened++;
                    }
                    if (!laid)
                    {
                        for (auto& b : built)
                        {
                            bool fills = JsStringOr(JsGet(b.node, "type")) != "text" || Near(b.rect.w, rect.w);
                            Place(b.node, rect, b.rect, AnchorOf(*b.s, b.rect, rect, fills));
                        }
                    }
                    Json children = Json::Array();
                    for (auto& b : built)
                        children.Add(b.node);
                    node["children"] = children;
                }

                Built out;
                out.node = node;
                out.rect = rect;
                out.s = &s;
                return out;
            }

            void BuildLook(const Json& s, const String& path, const UiRect& rect, const Vector<const Json*>& kids, Json& node,
                           std::set<int>& absorbed)
            {
                String type = JsStringOr(JsGet(s, "type"), "container");
                const UiKitElement* el = ElementOf(JsGet(s, "asset"));
                const Json* prog = JsGet(s, "progress");

                // A picture that fills the block under its other children is the block's own look: one panel (button, slot...)
                // instead of a group with a background child. With a single label over it, a button drawn as one block
                Vector<int> texts;
                for (int i = 0; i < kids.Count(); i++)
                {
                    if (StrIs(JsGet(kids[i], "type"), "text"))
                        texts.Add(i);
                }
                const Json* first = kids.IsEmpty() ? nullptr : kids[0];
                const Json* base = nullptr;
                if (JsTruthy(first) && StrIs(JsGet(first, "type"), "image") && !JsTruthy(JsGet(s, "asset")) &&
                    CleanColorOf(JsGet(s, "color")).IsEmpty() && !HasLength(JsGet(first, "children")))
                {
                    UiRect fr = BoxOf(*first, path + "/" + JsStringOr(JsGet(first, "name")), rect);
                    if (Near(fr.x, rect.x) && Near(fr.y, rect.y) && Near(fr.w, rect.w) && Near(fr.h, rect.h))
                        base = first;
                }
                const UiKitElement* baseEl = base ? ElementOf(JsGet(base, "asset")) : nullptr;

                if (JsIsObjectLike(prog))
                {
                    node["type"] = "progress";
                    double v = (Num(JsGet(prog, "value"), 0.6) - Num(JsGet(prog, "min"), 0))/
                               std::max(1e-6, Num(JsGet(prog, "max"), 1) - Num(JsGet(prog, "min"), 0));
                    node["value"] = std::max(0.0, std::min(1.0, v));
                    String fillName = JsStringOr(JsGet(prog, "fill"));
                    const Json* track = nullptr;
                    for (auto k : kids)
                    {
                        if (StrIs(JsGet(k, "type"), "image") && JsString(JsGet(k, "name")) != fillName)
                        {
                            track = k;
                            break;
                        }
                    }
                    const UiKitElement* trackEl = el ? el : track ? ElementOf(JsGet(track, "asset")) : nullptr;
                    if (trackEl)
                        node["kit"] = trackEl->name;
                    if (!texts.IsEmpty())
                    {
                        const Json& label = *kids[texts[0]];
                        node["text"] = JsStringOr(JsGet(label, "text"));
                        Assign(node, TextProps(label, false));
                    }
                    for (int i = 0; i < kids.Count(); i++)
                        absorbed.insert(i);
                }
                else if (base)
                {
                    const Json* baseName = JsGet(base, "name");
                    if (JsNullish(baseName))
                        baseName = JsGet(s, "name");
                    String mapped = baseEl ? ElementNodeType(baseEl->type) : ElementNodeType(GuessElementType(JsStringOr(baseName)));
                    String nodeType = type == "button" ? String("button") : mapped == "progress" || mapped == "background" ? String("panel") : mapped;
                    node["type"] = nodeType;
                    if (baseEl)
                        node["kit"] = baseEl->name;
                    String tint = CleanColorOf(JsGet(base, "color"));
                    if (!tint.IsEmpty())
                        node["color"] = tint;
                    absorbed.insert(0);

                    bool labelled = nodeType == "button" || nodeType == "tab" || nodeType == "badge" || IsButtonName(JsStringOr(JsGet(s, "name")));
                    if (labelled && kids.Count() == 2 && texts.Count() == 1)
                    {
                        if (nodeType != "tab" && nodeType != "badge")
                            nodeType = "button";
                        node["type"] = nodeType;
                        node["text"] = JsStringOr(JsGet(kids[texts[0]], "text"));
                        Assign(node, TextProps(*kids[texts[0]], true));
                        absorbed.insert(texts[0]);
                    }
                    if (nodeType == "button")
                        mResult.stats.buttons++;
                }
                else if (type == "image" || el)
                {
                    String nodeType = el ? ElementNodeType(el->type) : ElementNodeType(GuessElementType(JsStringOr(JsGet(s, "name"))));
                    // A progress track met outside a progress block is just its picture
                    if (nodeType == "progress")
                        nodeType = "image";
                    if (type == "button")
                        nodeType = "button";
                    node["type"] = nodeType;
                    if (el)
                        node["kit"] = el->name;
                    String tint = CleanColorOf(JsGet(s, "color"));
                    if (!tint.IsEmpty())
                        node["color"] = tint;
                    if (type == "button")
                        mResult.stats.buttons++;
                }
                else if (type == "rect" || type == "overlay" || !CleanColorOf(JsGet(s, "color")).IsEmpty())
                {
                    String nodeType = type == "rect" && (rect.h <= 6 || rect.w <= 6) ? "divider" : type == "button" ? "button" : "panel";
                    node["type"] = nodeType;
                    String c = CleanColorOf(JsGet(s, "color"));
                    if (!c.IsEmpty())
                        node["color"] = c;
                    // A flat shape the skill drew with a colour (a dimming layer, a strip) is a plain block: it must not pick up
                    // a kit graphic of its type
                    if (!c.IsEmpty() && nodeType == "panel")
                        node["kit"] = KIT_NONE_NAME;
                }
                else
                    node["type"] = type == "button" ? "button" : "group";
            }

            bool TryLayout(Json& node, const UiRect& rect, Vector<Built>& built, const String& kind, const Json& lay)
            {
                Vector<UiRect> rel;
                for (auto& b : built)
                    rel.Add(UiRect(b.rect.x - rect.x, b.rect.y - rect.y, b.rect.w, b.rect.h));

                // The children as the solver lays them (the probe is not normalized, as the web's is not)
                auto fits = [&]() -> bool {
                    UiDoc probe;
                    probe.name = "p";
                    probe.width = rect.w;
                    probe.height = rect.h;
                    Json root = node;
                    root["id"] = "root";
                    root["type"] = "screen";
                    Json children = Json::Array();
                    for (auto& b : built)
                        children.Add(b.node);
                    root["children"] = children;
                    probe.root = NodeFromJson(root);

                    UiRects got = SolveLayout(probe, rect.w, rect.h);
                    for (int i = 0; i < built.Count(); i++)
                    {
                        const UiRect* g = got.Find(JsStringOr(JsGet(built[i].node, "id")));
                        if (!g || !Near(g->x, rel[i].x) || !Near(g->y, rel[i].y) || !Near(g->w, rel[i].w) || !Near(g->h, rel[i].h))
                            return false;
                    }
                    return true;
                };

                bool ok = kind == "grid" ? TryGrid(node, built, rel, lay, fits) : TryLine(node, rect, built, rel, kind == "row", lay, fits);
                if (!ok)
                {
                    for (const char* k : { "layout", "gap", "gapY", "pad", "align", "justify", "cols", "cellW", "cellH" })
                        node.Remove(k);
                    for (auto& b : built)
                    {
                        for (const char* k : { "w", "h", "x", "y" })
                            b.node.Remove(k);
                    }
                }
                return ok;
            }

            bool TryGrid(Json& node, Vector<Built>& built, const Vector<UiRect>& rel, const Json& lay,
                         const std::function<bool()>& fits)
            {
                const Json* cell = JsGet(lay, "cellSize");
                const Json* sp = JsGet(lay, "spacing");
                double cols = std::max(1.0, JsRound(Num(JsGet(lay, "columns"), 1)));
                node["layout"] = "grid";
                node["cols"] = cols;
                double cellW = JsRound(Num(JsGet(cell, "width"), rel[0].w));
                double cellH = JsRound(Num(JsGet(cell, "height"), rel[0].h));
                node["cellW"] = cellW;
                node["cellH"] = cellH;
                bool spacingNumber = sp && sp->IsNumber();
                double gx = spacingNumber ? Num(sp) : Num(JsGet(sp, "x"));
                double gy = spacingNumber ? Num(sp) : Num(JsGet(sp, "y"));
                if (gx)
                    node["gap"] = gx;
                if (gy != gx)
                    node["gapY"] = gy;

                // The first cell's corner is the padding; a centred grid has none
                for (bool centred : { true, false })
                {
                    node.Remove("align");
                    node.Remove("justify");
                    if (centred)
                    {
                        node["align"] = "center";
                        node["justify"] = "center";
                        node.Remove("pad");
                    }
                    else
                        SetPad(node, rel[0].y - (cellH - rel[0].h)/2, 0, 0, rel[0].x - (cellW - rel[0].w)/2);

                    for (int i = 0; i < built.Count(); i++)
                    {
                        Json& kid = built[i].node;
                        kid.Remove("w");
                        kid.Remove("h");
                        if (!Near(rel[i].w, cellW, 1))
                            kid["w"] = JsRound(rel[i].w);
                        if (!Near(rel[i].h, cellH, 1))
                            kid["h"] = JsRound(rel[i].h);
                    }
                    if (fits())
                        return true;
                }
                return false;
            }

            bool TryLine(Json& node, const UiRect& rect, Vector<Built>& built, const Vector<UiRect>& rel, bool row,
                         const Json& lay, const std::function<bool()>& fits)
            {
                auto main = [&](const UiRect& r) { return row ? r.x : r.y; };
                auto len = [&](const UiRect& r) { return row ? r.w : r.h; };
                auto cross = [&](const UiRect& r) { return row ? r.y : r.x; };
                auto clen = [&](const UiRect& r) { return row ? r.h : r.w; };
                double mainLen = row ? rect.w : rect.h, crossLen = row ? rect.h : rect.w;

                Vector<double> gaps;
                for (int i = 1; i < rel.Count(); i++)
                    gaps.Add(main(rel[i]) - (main(rel[i - 1]) + len(rel[i - 1])));
                double gap = 0;
                if (!gaps.IsEmpty())
                {
                    double sum = 0;
                    for (double g : gaps)
                        sum += g;
                    gap = sum/gaps.Count();
                }
                for (double g : gaps)
                {
                    if (!Near(g, gap, 1.5))
                        return false;
                }
                if (gap < -0.5)
                    return false;

                node["layout"] = row ? "row" : "column";
                double lead = main(rel[0]), trail = mainLen - (main(rel.Last()) + len(rel.Last()));
                Vector<double> c0, c1;
                for (auto& r : rel)
                {
                    c0.Add(cross(r));
                    c1.Add(crossLen - cross(r) - clen(r));
                }
                auto same = [](const Vector<double>& a) { return !a.Any([&](double v) { return !Near(v, a[0], 1.5); }); };
                auto least = [](const Vector<double>& a) { return *std::min_element(a.begin(), a.end()); };
                bool centred = true, spans = true;
                for (int i = 0; i < rel.Count(); i++)
                {
                    centred = centred && Near(c0[i], c1[i], 2);
                    spans = spans && Near(clen(rel[i]), crossLen);
                }
                String want = JsStringOr(JsGet(lay, "align"), "start");

                auto crossSetup = [&](double& ca, double& cb) {
                    if (centred && !spans)
                    {
                        node["align"] = "center";
                        ca = cb = 0;
                        return;
                    }
                    if (same(c0))
                    {
                        node["align"] = "start";
                        ca = c0[0];
                        cb = least(c1);
                        return;
                    }
                    if (same(c1))
                    {
                        node["align"] = "end";
                        ca = least(c0);
                        cb = c1[0];
                        return;
                    }
                    node["align"] = "start";
                    double base = least(c0);
                    for (int i = 0; i < built.Count(); i++)
                    {
                        double d = JsRound(c0[i] - base);
                        if (d)
                            built[i].node[row ? "y" : "x"] = d;
                    }
                    ca = base;
                    cb = 0;
                };
                auto setup = [&](const String& justify, double padLead, double padTrail) {
                    for (auto& b : built)
                    {
                        b.node.Remove("x");
                        b.node.Remove("y");
                    }
                    for (int i = 0; i < built.Count(); i++)
                    {
                        built[i].node["w"] = JsRound(rel[i].w);
                        built[i].node["h"] = JsRound(rel[i].h);
                    }
                    if (gap > 0.5 && (justify == "start" || justify == "center" || justify == "end"))
                        node["gap"] = JsRound(gap*100)/100;
                    else
                        node.Remove("gap");
                    if (justify != "start")
                        node["justify"] = justify;
                    else
                        node.Remove("justify");
                    double ca = 0, cb = 0;
                    crossSetup(ca, cb);
                    if (row)
                        SetPad(node, ca, padTrail, cb, padLead);
                    else
                        SetPad(node, padLead, cb, padTrail, ca);
                };

                // Candidates: how the skill described the row, then plainly from the start with the measured padding
                Vector<std::function<void()>> tries;
                String spread = want == "space-between" ? "between" : want == "space-around" ? "around" : want == "space-evenly" ? "evenly" : "";
                if (!spread.IsEmpty())
                    tries.Add([&]() { setup(spread, 0, 0); });
                if ((want == "center" || Near(lead, trail, 2)) && lead > 1)
                    tries.Add([&]() { setup("center", 0, 0); });
                tries.Add([&]() { setup("start", lead, 0); });
                for (auto& t : tries)
                {
                    t();
                    if (fits())
                        return true;
                }
                return false;
            }
        };

    }

    FromStructureResult FromUiStructure(const FromStructureInput& input)
    {
        StructureReader reader(input);
        return reader.Run();
    }

    Map<String, UiRect> LegendBoxes(const Json& legend)
    {
        Map<String, UiRect> out;
        const Json* targets = JsGet(legend, "targets");
        if (!targets || !targets->IsArray())
            return out;

        for (auto& t : targets->Items())
        {
            const Json* b = JsGet(t, "abs_bbox");
            const Json* p = JsGet(t, "path");
            if (!p || !p->IsString() || !b || !b->IsArray() || b->Count() != 4)
                continue;

            double v[4];
            bool finite = true;
            for (int i = 0; i < 4; i++)
            {
                v[i] = b->At(i)->ToJsNumber();
                finite = finite && std::isfinite(v[i]);
            }
            if (finite)
                out[p->Str()] = UiRect(v[0], v[1], v[2], v[3]);
        }
        return out;
    }

    String ElementNodeType(const String& elementType)
    {
        static const std::pair<const char*, const char*> types[] = {
            { "background", "background" }, { "panel", "panel" }, { "frame", "panel" }, { "button", "button" }, { "icon", "icon" },
            { "image", "image" }, { "progressBar", "progress" }, { "progressFill", "image" }, { "toggle", "toggle" }, { "slider", "slider" },
            { "sliderHandle", "image" }, { "input", "input" }, { "tab", "tab" }, { "badge", "badge" }, { "divider", "divider" },
            { "header", "panel" }, { "slot", "slot" }, { "other", "image" },
        };
        for (auto& t : types)
        {
            if (elementType == t.first)
                return t.second;
        }
        return "";
    }
}
