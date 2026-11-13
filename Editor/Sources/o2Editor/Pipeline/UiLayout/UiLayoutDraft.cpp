#include "o2Editor/stdafx.h"
#include "UiLayoutDraft.h"

#include <cmath>
#include <cstdlib>
#include <map>
#include <set>

#include "o2Editor/Pipeline/UiDoc/UiDocEdit.h"
#include "o2Editor/Pipeline/UiDoc/UiDocFromStructure.h"
#include "o2Editor/Pipeline/UiDoc/UiDocRotate.h"
#include "o2Editor/Pipeline/UiDoc/UiDocFp.h"

namespace Editor::UiLayout
{
    namespace
    {
        namespace LayoutDoc = ::Editor::UiDoc;

        // A block that draws no kit graphic on purpose (render.ts KIT_NONE)
        const char* const KIT_NONE_NAME = "none";

        bool Near(double a, double b, double tol)
        {
            return std::abs(a - b) <= tol;
        }

        // cleanColor(v), "" for none
        String CleanColorText(const String& text)
        {
            Json v(text);
            String out;
            return LayoutDoc::CleanColor(&v, out) ? out : String();
        }

        bool RgbOf(const String& hex, int out[3])
        {
            String c = CleanColorText(hex);
            if (c.IsEmpty())
                return false;

            for (int i = 0; i < 3; i++)
                out[i] = (int)std::strtol(((std::string)c.SubStr(1 + i*2, 3 + i*2)).c_str(), nullptr, 16);
            return true;
        }

        // How far two colours are; 255 when either is missing
        double ColorDist(const String& a, const String& b)
        {
            int x[3], y[3];
            if (!RgbOf(a, x) || !RgbOf(b, y))
                return 255.0;

            double d0 = x[0] - y[0], d1 = x[1] - y[1], d2 = x[2] - y[2];
            return std::sqrt(d0*d0 + d1*d1 + d2*d2);
        }

        // /^[\d\s.,:+\-×x%/]+$/
        bool IsNumberText(const String& text)
        {
            std::u32string cps = LayoutDoc::Utf8ToCodePoints(text);
            if (cps.empty())
                return false;

            for (char32_t c : cps)
            {
                bool ok = (c >= U'0' && c <= U'9') || LayoutDoc::IsJsSpace(c) || c == U'.' || c == U',' || c == U':' || c == U'+' ||
                          c == U'-' || c == 0xD7 || c == U'x' || c == U'%' || c == U'/';
                if (!ok)
                    return false;
            }
            return true;
        }

        // Members of `from` written over `to`, as Object.assign does
        void Assign(Json& to, const Json& from)
        {
            for (auto& m : from.Members())
                to[m.first] = m.second;
        }

        // The props that put the node at r inside p with the anchor, sizes written (placeAt(...).set assigned)
        void Place(Json& node, const LayoutDoc::UiRect& p, const LayoutDoc::UiRect& r, const LayoutDoc::Anchor& a)
        {
            Json own = node;
            own.Remove("children");
            Assign(node, LayoutDoc::PlaceAt(LayoutDoc::NodeFromJson(own), p, r, &a, true).set);
        }

        // ----------------------------------------------------------------------------------
        // The draft's state: the elements by parent, ids taken, styles, boxes and the count
        // ----------------------------------------------------------------------------------
        class Drafter
        {
        public:
            Drafter(const UiDoc& analysis, const DraftOptions& options);

            // The analysis -> the result
            DraftResult Run();

        private:
            const UiDoc&                                    mAn;
            const DraftOptions&                             mO;
            double                                          mW = 0, mH = 0;
            std::map<std::string, Vector<const UiEl*>>      mKids;
            std::set<std::string>                           mUsed = { "root" };
            Vector<LayoutDoc::TextStyle>                    mStyles;
            double                                          mMaxFont = 0;
            DraftResult                                     mResult;

        private:
            const Vector<const UiEl*>& KidsOf(const String& id) const;
            String IdFor(const String& want);
            String RoleOf(const UiEl& e, bool inButton) const;
            Json TextProps(const UiEl& e, bool inButton);
            Json Build(const UiEl& e, bool inButton);
        };

        Drafter::Drafter(const UiDoc& analysis, const DraftOptions& options): mAn(analysis), mO(options)
        {
            mW = mAn.w;
            mH = mAn.h;
            for (auto& e : mAn.els)
                mKids[(std::string)e->parent].Add(e.Get());
            mStyles = mO.styles;

            bool nan = false;
            for (auto& e : mAn.els)
            {
                if (e->kind != "text")
                    continue;
                double f = e->fontSize.value_or(0);
                if (std::isnan(f))
                    nan = true;
                else
                    mMaxFont = std::max(mMaxFont, f);
            }
            if (nan)
                mMaxFont = NAN;
        }

        DraftResult Drafter::Run()
        {
            LayoutDoc::UiRect rootRect(0, 0, mW, mH);
            Json root = Json::Object();
            root["id"] = "root";
            root["type"] = "screen";
            Json top = Json::Array();
            if (!mO.backgroundKit.IsEmpty())
            {
                mUsed.insert("backdrop");
                Json backdrop = Json::Object();
                backdrop["id"] = "backdrop";
                backdrop["type"] = "background";
                backdrop["anchor"] = "stretch";
                backdrop["kit"] = mO.backgroundKit;
                top.Add(backdrop);
                mResult.blocks++;
            }
            else if (String color = CleanColorText(mO.backgroundColor); !color.IsEmpty())
                root["color"] = color;

            for (auto e : KidsOf(""))
            {
                Json n = Build(*e, false);
                LayoutDoc::UiRect r = mResult.boxes[n["id"].Str()];
                Place(n, rootRect, r, AnchorFor(r, rootRect));
                top.Add(n);
            }
            if (top.Count())
                root["children"] = top;

            Json doc = Json::Object();
            String name = LayoutDoc::CleanId(mO.name);
            doc["name"] = name.IsEmpty() ? String("Screen") : name;
            doc["width"] = mW;
            doc["height"] = mH;
            if (auto dpi = mO.dpi ? LayoutDoc::CleanDpi(*mO.dpi) : std::nullopt)
                doc["dpi"] = *dpi;
            doc["root"] = root;
            mResult.doc = LayoutDoc::NormalizeDoc(doc);
            return mResult;
        }

        const Vector<const UiEl*>& Drafter::KidsOf(const String& id) const
        {
            static const Vector<const UiEl*> none;
            auto it = mKids.find((std::string)id);
            return it != mKids.end() ? it->second : none;
        }

        String Drafter::IdFor(const String& want)
        {
            String base = LayoutDoc::CleanId(want);
            if (base.IsEmpty())
                base = "block";
            for (int i = 1; ; i++)
            {
                String t = i == 1 ? base : base + "_" + LayoutDoc::JsNum(i);
                std::string key = LayoutDoc::JsToLower(t);
                if (!mUsed.count(key))
                {
                    mUsed.insert(key);
                    return t;
                }
            }
        }

        String Drafter::RoleOf(const UiEl& e, bool inButton) const
        {
            double size = e.fontSize.value_or(32);
            bool maxFont = mMaxFont != 0 && !std::isnan(mMaxFont);
            if (inButton)
                return "button";
            if (IsNumberText(e.text))
                return "number";
            if (maxFont && size >= mMaxFont*0.92)
                return "title";
            if (maxFont && size >= mMaxFont*0.6)
                return "heading";
            if (maxFont && size <= mMaxFont*0.35)
                return "caption";
            return "body";
        }

        // A label's style and what the block overrides of it
        Json Drafter::TextProps(const UiEl& e, bool inButton)
        {
            double size = std::max(6.0, LayoutDoc::JsRound(e.fontSize ? *e.fontSize : std::max(10.0, e.box.h*0.8)));
            String color = CleanColorText(e.color);
            if (color.IsEmpty())
                color = "#FFFFFF";

            String told;
            if (auto it = mO.styleOf.find(e.id); it != mO.styleOf.end())
                told = LayoutDoc::JsToLower(it->second);
            int st = -1;
            for (int i = 0; i < mStyles.Count() && st < 0; i++)
            {
                if (LayoutDoc::JsToLower(mStyles[i].name) == told)
                    st = i;
            }

            if (st < 0)
            {
                // Told no style: the nearest by colour, else one of its own, named by its role
                String role = RoleOf(e, inButton);
                int close = -1;
                double closeD = 0;
                for (int i = 0; i < mStyles.Count(); i++)
                {
                    auto& s = mStyles[i];
                    double d = ColorDist(s.color, color)/120 + (s.stroke.has_value() != !e.stroke.IsEmpty() ? 0.4 : 0) + (s.role == role ? -0.1 : 0);
                    if (close < 0 || d < closeD)
                    {
                        close = i;
                        closeD = d;
                    }
                }
                if (close >= 0 && closeD < 0.45)
                    st = close;
                else
                {
                    std::set<std::string> taken;
                    for (auto& x : mStyles)
                        taken.insert(LayoutDoc::JsToLower(x.name));
                    String name = role;
                    for (int i = 2; taken.count(LayoutDoc::JsToLower(name)); i++)
                        name = role + "_" + LayoutDoc::JsNum(i);

                    Json raw = Json::Object();
                    raw["name"] = name;
                    raw["role"] = role;
                    raw["font"] = "Roboto";
                    raw["weight"] = e.weight == "bold" ? 700 : e.weight == "medium" ? 500 : 400;
                    raw["size"] = size;
                    raw["color"] = color;
                    if (!e.stroke.IsEmpty())
                    {
                        Json stroke = Json::Object();
                        stroke["color"] = e.stroke;
                        stroke["width"] = std::max(1.0, LayoutDoc::JsRound(size/16));
                        raw["stroke"] = stroke;
                    }
                    else
                        raw["stroke"] = Json(nullptr);
                    raw["align"] = e.align.IsEmpty() ? String("center") : e.align;
                    raw["description"] = "read off the reference picture";

                    LayoutDoc::TextStyle made = LayoutDoc::NormalizeTextStyle(raw, name);
                    mStyles.Add(made);
                    mResult.styles.Add(made);
                    st = mStyles.Count() - 1;
                }
            }

            const LayoutDoc::TextStyle& style = mStyles[st];
            Json out = Json::Object();
            out["style"] = style.name;
            if (std::abs(style.size - size)/size > 0.06)
                out["fontSize"] = size;
            // A style the model chose for the label carries its colour; only a clearly different fill is kept on the block
            bool gradient = style.gradient && !style.gradient->IsEmpty();
            if (ColorDist(style.color, color) > 90 && !gradient)
                out["textColor"] = color;
            if (!e.align.IsEmpty() && e.align != style.align)
                out["textAlign"] = e.align;
            return out;
        }

        Json Drafter::Build(const UiEl& e, bool inButton)
        {
            LayoutDoc::UiRect rect(LayoutDoc::JsRound(e.box.x), LayoutDoc::JsRound(e.box.y), std::max(1.0, LayoutDoc::JsRound(e.box.w)),
                                   std::max(1.0, LayoutDoc::JsRound(e.box.h)));
            String id = IdFor(e.id);
            Json node = Json::Object();
            node["id"] = id;
            node["type"] = "group";
            mResult.blocks++;
            mResult.boxes[id] = rect;

            const Vector<const UiEl*>& mine = KidsOf(e.id);
            std::set<const UiEl*> absorbed;
            String kitName = e.kit;
            if (kitName.IsEmpty() && !e.look.IsEmpty())
            {
                if (auto it = mO.assetOf.find(e.look); it != mO.assetOf.end())
                    kitName = it->second;
            }
            const LayoutDoc::UiKitElement* el = LayoutDoc::KitElement(mO.kit, kitName);
            String mapped = el ? LayoutDoc::ElementNodeType(el->type) : String();

            String type = "group";
            if (e.kind == "text")
            {
                type = "text";
                node["text"] = e.text;
                Assign(node, TextProps(e, inButton));
            }
            else if (e.kind == "group" || e.kind == "grid")
                type = "group";
            else if (e.kind == "progress")
            {
                type = "progress";
                node["value"] = e.value.value_or(0.5);
                // The fill is the kit's (or none); a caption over the bar is the bar's own label
                const UiEl* label = nullptr;
                for (auto c : mine)
                {
                    if (c->kind == "image")
                        absorbed.insert(c);
                    if (!label && c->kind == "text")
                        label = c;
                }
                if (label)
                {
                    node["text"] = label->text;
                    Assign(node, TextProps(*label, false));
                    absorbed.insert(label);
                }
            }
            else if (e.kind == "button")
            {
                type = mapped == "tab" || mapped == "badge" ? mapped : String("button");
                // A lone label on a button is the button's text
                if (mine.Count() == 1 && mine[0]->kind == "text")
                {
                    node["text"] = mine[0]->text;
                    Assign(node, TextProps(*mine[0], true));
                    absorbed.insert(mine[0]);
                }
            }
            else if (e.kind == "input")
                type = "input";
            else if (e.kind == "panel")
            {
                bool own = !mapped.IsEmpty() && mapped != "progress" && mapped != "background" && mapped != "icon" && mapped != "image";
                type = own ? mapped : String("panel");
            }
            else
            {
                // A fixed picture: what its graphic is, else an icon when small
                if (mapped == "progress" || mapped == "background")
                    type = "image";
                else if (!mapped.IsEmpty())
                    type = mapped;
                else
                    type = rect.w*rect.h < mW*mH*0.012 && std::max(rect.w, rect.h) < std::min(mW, mH)*0.2 ? "icon" : "image";
            }

            if (!kitName.IsEmpty() && type != "text" && type != "group")
                node["kit"] = el ? el->name : kitName;
            else if (kitName.IsEmpty() && type != "text" && type != "group" && !CleanColorText(e.color).IsEmpty())
            {
                // A flat shape: its colour, no graphic
                type = "panel";
                node["kit"] = KIT_NONE_NAME;
                node["color"] = CleanColorText(e.color);
            }
            node["type"] = type;
            if (type != "text" && e.opacity && *e.opacity < 1)
                node["opacity"] = *e.opacity;

            // A tilted element: turned about its centre, its box the upright one inside what the picture shows (a kit
            // graphic's own proportions make that exact)
            auto turn = e.rotation ? LayoutDoc::CleanTurn(*e.rotation) : std::nullopt;
            if (turn)
            {
                node["rotation"] = *turn;
                std::optional<double> aspect;
                if (el && el->assetType == "simple")
                    aspect = el->width/el->height;
                LayoutDoc::UiSize own = LayoutDoc::UnturnedSize(rect.w, rect.h, *turn, aspect);
                double cx = rect.x + rect.w/2, cy = rect.y + rect.h/2;
                rect = LayoutDoc::UiRect(LayoutDoc::JsRound(cx - own.w/2), LayoutDoc::JsRound(cy - own.h/2), std::max(1.0, LayoutDoc::JsRound(own.w)),
                                         std::max(1.0, LayoutDoc::JsRound(own.h)));
                mResult.boxes[id] = rect;
            }

            bool inBtn = inButton || type == "button" || type == "tab";
            Json children = Json::Array();
            for (auto c : mine)
            {
                if (absorbed.count(c))
                    continue;

                Json child = Build(*c, inBtn);
                LayoutDoc::UiRect r = mResult.boxes[child["id"].Str()];
                Place(child, rect, r, AnchorFor(r, rect));
                children.Add(child);
            }
            if (children.Count())
                node["children"] = children;
            return node;
        }
    }

    ::Editor::UiDoc::Anchor AnchorFor(const ::Editor::UiDoc::UiRect& r, const ::Editor::UiDoc::UiRect& p)
    {
        double tolX = std::max(2.0, p.w*0.02), tolY = std::max(2.0, p.h*0.02);
        double l = r.x - p.x, rt = p.x + p.w - r.x - r.w, t = r.y - p.y, b = p.y + p.h - r.y - r.h;
        ::Editor::UiDoc::Anchor out;
        if (r.w >= p.w*0.9 && Near(l, rt, std::max(tolX, p.w*0.04)))
            out.h = "stretch";
        else if (Near(l, rt, std::max(tolX, r.w*0.12)))
            out.h = "center";
        else
            out.h = l <= rt ? "left" : "right";

        if (r.h >= p.h*0.9 && Near(t, b, std::max(tolY, p.h*0.04)))
            out.v = "stretch";
        else if (Near(t, b, std::max(tolY, r.h*0.12)))
            out.v = "middle";
        else
            out.v = t <= b ? "top" : "bottom";
        return out;
    }

    DraftResult DraftFromAnalysis(const UiDoc& analysis, const DraftOptions& o)
    {
        Drafter drafter(analysis, o);
        return drafter.Run();
    }
}
