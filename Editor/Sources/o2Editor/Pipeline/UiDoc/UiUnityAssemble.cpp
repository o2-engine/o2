#include "o2Editor/stdafx.h"
#include "UiUnityInternal.h"

#include <cmath>
#include <functional>

#include "o2Editor/Pipeline/UiDoc/UiDocFp.h"

namespace Editor::UiDoc::Unity
{
    // --- GameObjects ------------------------------------------------------------------------------------

    // RectTransform's native setters return early when the new value is approximately the stored one (squared distance
    // within 1e-5^2), keeping the old value: a near-equal or a -0 never replaces it
    static V2 Keep(const V2& old, const V2& v)
    {
        V2 n = MakeV2(v.x, v.y);
        double dx = F(n.x - old.x), dy = F(n.y - old.y);
        return F(F(dx * dx) + F(dy * dy)) <= F(1e-5 * 1e-5) ? old : n;
    }

    static const V2 kZero = { 0, 0 };
    static const V2 kOne = { 1, 1 };

    void GO::SetAm(const V2& v) { mAm = Keep(mAm, v); }
    void GO::SetAx(const V2& v) { mAx = Keep(mAx, v); }
    void GO::SetAp(const V2& v) { mAp = Keep(mAp, v); }
    void GO::SetSd(const V2& v) { mSd = Keep(mSd, v); }
    void GO::SetPv(const V2& v) { mPv = Keep(mPv, v); }

    V2 GO::OffsetMin() const
    {
        return Sub(mAp, Scale(mSd, mPv));
    }

    void GO::SetOffsetMin(const V2& value)
    {
        V2 offset = Sub(value, Sub(mAp, Scale(mSd, mPv)));
        SetSd(Sub(mSd, offset));
        SetAp(Add(mAp, Scale(offset, Sub(kOne, mPv))));
    }

    V2 GO::OffsetMax() const
    {
        return Add(mAp, Scale(mSd, Sub(kOne, mPv)));
    }

    void GO::SetOffsetMax(const V2& value)
    {
        V2 offset = Sub(value, Add(mAp, Scale(mSd, Sub(kOne, mPv))));
        SetSd(Add(mSd, offset));
        SetAp(Add(mAp, Scale(offset, mPv)));
    }

    void GO::SetParent(GO* p, int index /*= -1*/)
    {
        if (parent)
        {
            auto& list = parent->children;
            list.erase(std::find(list.begin(), list.end(), this));
        }
        parent = p;
        if (index < 0 || index >= (int)p->children.size())
            p->children.push_back(this);
        else
            p->children.insert(p->children.begin() + index, this);
    }

    int GO::SiblingIndex() const
    {
        if (!parent)
            return 0;
        return (int)(std::find(parent->children.begin(), parent->children.end(), this) - parent->children.begin());
    }

    void GO::SetSiblingIndex(int i)
    {
        auto& list = parent->children;
        list.erase(std::find(list.begin(), list.end(), this));
        list.insert(list.begin() + std::min(i, (int)list.size()), this);
    }

    bool GO::ActiveInHierarchy() const
    {
        for (const GO* g = this; g; g = g->parent)
        {
            if (!g->active)
                return false;
        }
        return true;
    }

    Comp* GO::Get(const String& kind) const
    {
        for (auto& c : comps)
        {
            if (c->kind == kind)
                return c.get();
        }
        return nullptr;
    }

    Comp* GO::Graphic() const
    {
        for (auto& c : comps)
        {
            if (c->kind == "Image" || c->kind == "Text")
                return c.get();
        }
        return nullptr;
    }

    static String UniqueName(GO* parent, const String& name)
    {
        auto taken = [&](const String& n)
        {
            for (auto c : parent->children)
            {
                if (c->name == n)
                    return true;
            }
            return false;
        };
        if (!taken(name))
            return name;
        for (int i = 1; ; i++)
        {
            String candidate = name + " (" + String(i) + ")";
            if (!taken(candidate))
                return candidate;
        }
    }

    static void Stretch(GO* g)
    {
        g->SetAm(MakeV2(0, 0));
        g->SetAx(MakeV2(1, 1));
        g->SetOffsetMin(kZero);
        g->SetOffsetMax(kZero);
    }

    // --- the assembler -----------------------------------------------------------------------------------

    static Col White() { return Col{ 1, 1, 1, 1 }; }
    static Col Clear() { return Col{ 0, 0, 0, 0 }; }

    static String StateName(const String& v)
    {
        if (v == "." || v == "..")
            return "State_" + v;
        String out;
        for (int i = 0; i < v.Length(); i++)
        {
            char c = v[i];
            if (c == '%')
                out += "%25";
            else if (c == '/')
                out += "%2F";
            else if (c == '\\')
                out += "%5C";
            else
                out += c;
        }
        return out;
    }

    static String HelperName(const Json& node, const String& prefix)
    {
        std::set<std::string> names;
        for (auto c : ChildrenOf(node))
            names.insert(TextOf(c, "name").Data());
        String r = prefix;
        for (int i = 1; names.count(r.Data()); i++)
            r = prefix + "_" + String(i);
        return r;
    }

    static void GraphicsInChildren(GO* g, std::vector<Comp*>& out)
    {
        for (auto& c : g->comps)
        {
            if (c->kind == "Image" || c->kind == "Text")
                out.push_back(c.get());
        }
        for (auto child : g->children)
            GraphicsInChildren(child, out);
    }

    static bool IsObjJ(const Json* v)
    {
        return v && v->IsObject();
    }

    static bool IsDistributed(const String& mode)
    {
        return mode == "space-between" || mode == "space-around" || mode == "space-evenly";
    }

    // RectTransform.rect.size, the root canvas taken at the reference size
    static V2 RectSize(GO* g)
    {
        if (!g->parent)
            return g->Sd();
        V2 p = RectSize(g->parent);
        return MakeV2(F(F(p.x * F(g->Ax().x - g->Am().x)) + g->Sd().x), F(F(p.y * F(g->Ax().y - g->Am().y)) + g->Sd().y));
    }

    Assembler::Assembler(Doc& doc, UnityAssets& assets): doc(doc), assets(assets)
    {
        boxes = doc.Resolve();
    }

    GO* Assembler::NewGO(const String& name)
    {
        pool.push_back(std::make_unique<GO>(name));
        return pool.back().get();
    }

    GO* Assembler::NewRect(const String& name, GO* parent)
    {
        GO* g = NewGO(name);
        g->SetParent(parent);
        g->SetPv(MakeV2(0, 1));
        return g;
    }

    Comp* Assembler::AddComp(GO* go, const String& kind)
    {
        if ((kind == "Image" || kind == "Text") && !go->Get("CanvasRenderer"))
        {
            auto renderer = std::make_unique<Comp>();
            renderer->kind = "CanvasRenderer";
            renderer->go = go;
            go->comps.push_back(std::move(renderer));
        }
        auto c = std::make_unique<Comp>();
        c->kind = kind;
        c->go = go;
        go->comps.push_back(std::move(c));
        return go->comps.back().get();
    }

    const UBox& Assembler::BoxOf(const String& path)
    {
        auto it = boxes.find(path.Data());
        if (it == boxes.end())
            throw ExportError(("no box for " + path).Data());
        return it->second;
    }

    GO* Assembler::Build(const String& name)
    {
        root = NewGO(name);
        AddComp(root, "Canvas");
        AddComp(root, "CanvasScaler")->reference = doc.ref;
        AddComp(root, "GraphicRaycaster");
        root->SetSd(doc.ref);
        Doc snapshot = doc.Snapshot();
        BuildNode(doc, snapshot, "root", root);
        return root;
    }

    GO* Assembler::BuildNode(Doc& source, Doc& snapshot, const String& path, GO* parent)
    {
        Json node = snapshot.Node(path);
        GO* rect = NewRect(TextOf(&node, "name"), parent);
        bool isRoot = path == "root";
        String pp = isRoot ? String() : ParentPath(path);
        UBox box = BoxOf(path);
        V2 parentSize = isRoot ? doc.ref : MakeV2(BoxOf(pp).w, BoxOf(pp).h);
        Json parentNode;
        if (!isRoot)
            parentNode = snapshot.Node(pp);
        SetRectangle(rect, node, box, parentSize, isRoot ? nullptr : &parentNode, isRoot && box.w == doc.ref.x && box.h == doc.ref.y);

        Json state;
        if (const Json* found = source.Node(path).Get("state"))
            state = *found;
        if (!state.IsObject())
        {
            BuildBody(source, snapshot, path, rect);
            return rect;
        }

        GO* group = NewRect("States", rect);
        Stretch(group);
        String current = TextOf(&state, "current");
        const Json* variants = state.Get("variants");
        if (variants && variants->IsObject())
        {
            for (auto& variant : variants->Members())
            {
                Doc variantSource(source.json);
                variantSource.Node(path)["state"]["current"] = variant.first;
                Doc variantSnapshot = variantSource.Snapshot();
                GO* selector = NewRect(UniqueName(group, StateName(variant.first)), group);
                Stretch(selector);
                selector->active = variant.first == current;
                GO* content = NewRect("Content", selector);
                Stretch(content);
                BuildBody(variantSource, variantSnapshot, path, content);
            }
        }
        return rect;
    }

    void Assembler::BuildBody(Doc& source, Doc& snapshot, const String& path, GO* rect)
    {
        Json node = snapshot.Node(path);
        String type = TextOf(&node, "type");
        const Json* colorValue = node.Get("color");
        bool hasColor = colorValue && !colorValue->IsNull();
        Col color = hasColor ? ParseColor(TextOf(&node, "color")) : White();
        color.a = F(color.a * F(NumOf(&node, "opacity", 1)));
        String asset = TextOf(&node, "asset");

        Json parentNode;
        if (path != "root")
            parentNode = snapshot.Node(ParentPath(path));
        const Json* ownerProgress = IsObjJ(parentNode.Get("progress")) ? parentNode.Get("progress") : nullptr;
        bool separateVisual = ownerProgress && TextOf(ownerProgress, "fill") == TextOf(&node, "name");

        if (!asset.IsEmpty() || (hasColor && type != "text"))
        {
            GO* visual = rect;
            if (separateVisual || type == "text")
            {
                visual = NewRect(HelperName(node, "Image"), rect);
                Stretch(visual);
            }
            Comp* image = AddComp(visual, "Image");
            image->color = color;
            image->raycast = type == "overlay";
            if (!asset.IsEmpty())
            {
                SpriteInfo sprite = assets.SpriteFor(asset, node.Get("nineSlice"), F(NumOf(&node, "hueShift")));
                image->sprite = sprite;
                image->imageType = HasSlice(node.Get("nineSlice")) ? 1 : 0;
                // Sprite pixels per layout pixel: a denser sprite's borders shrink to the layout's scale
                image->ppuMul = F((100 / sprite.ppu) * (sprite.density ? *sprite.density : 1));
            }
            images++;
        }
        if (type == "text")
            AddText(rect, node, color, separateVisual);

        std::vector<std::pair<String, GO*>> kids;
        auto kidOf = [&](const String& name) -> GO*
        {
            for (auto& k : kids)
            {
                if (k.first == name)
                    return k.second;
            }
            throw ExportError((path + ": no child " + name).Data());
        };
        for (auto child : ChildrenOf(node))
        {
            String name = TextOf(child, "name");
            kids.push_back({ name, BuildNode(source, snapshot, path + "/" + name, rect) });
        }

        if (const Json* progress = IsObjJ(node.Get("progress")) ? node.Get("progress") : nullptr)
        {
            GO* fill = kidOf(TextOf(progress, "fill"));
            double ratio = F((NumOf(progress, "value") - NumOf(progress, "min")) / (NumOf(progress, "max", 1) - NumOf(progress, "min")));
            UBox fb = BoxOf(path + "/" + TextOf(progress, "fill"));
            AddProgressClip(fill, ratio, TextOf(progress, "direction"), MakeV2(fb.w, fb.h));
        }
        if (const Json* scroll = IsObjJ(node.Get("scroll")) ? node.Get("scroll") : nullptr)
            AddScroll(rect, kidOf(TextOf(scroll, "content")), *scroll, path);

        if (type == "button")
        {
            Comp* button = AddComp(rect, "Button");
            Comp* graphic = rect->Graphic();
            if (!graphic && separateVisual)
            {
                std::vector<Comp*> found;
                GraphicsInChildren(rect, found);
                graphic = found.empty() ? nullptr : found[0];
            }
            if (!graphic)
            {
                GO* hit = rect;
                if (separateVisual)
                {
                    hit = NewRect(HelperName(node, "HitArea"), rect);
                    Stretch(hit);
                    hit->SetSiblingIndex(0);
                }
                graphic = AddComp(hit, "Image");
                graphic->color = Clear();
                graphic->raycast = false;
            }
            graphic->raycast = true;
            button->target = graphic;
        }
        ApplyLayout(rect, node, kids);
        rect->active = BoolOf(&node, "visible", true);
    }

    void Assembler::AddText(GO* parent, const Json& node, const Col& color, bool separateVisual)
    {
        double sx = F(NumOf(&node, "textScaleX", 1));
        GO* visual = separateVisual || !TextOf(&node, "asset").IsEmpty() || !Approximately(sx, 1) ?
            NewRect(HelperName(node, "Text"), parent) : parent;
        String family = TextOf(&node, "fontFamily");
        const FontInfo& font = assets.Font(family);
        int fontSize = (int)std::max(1.0, RoundEven(NumOf(&node, "fontSize", 24)));
        String horizontal = TextOf(&node, "alignment", "left");
        String vertical = TextOf(&node, "textVAlign", "top");
        int x = horizontal == "right" ? 2 : horizontal == "center" ? 1 : 0;
        int y = vertical == "bottom" || vertical == "end" ? 2 : vertical == "middle" || vertical == "center" ? 1 : 0;

        double lineSpacing = 1;
        double lineHeight = 0, metricSize = 0;
        bool metrics = !font.file.IsEmpty() && FontMetrics(font.file, lineHeight, metricSize);
        const Json* lh = node.Get("lineHeight");
        if (lh && !lh->IsNull())
        {
            if (metrics && lineHeight > 0 && metricSize > 0)
                lineSpacing = F(F(NumOf(&node, "lineHeight")) / ((lineHeight * fontSize) / metricSize));
            else if (!metrics)
                assets.warnings.insert((TextOf(&node, "name") + ": lineHeight kept at 1 (no metrics for the font)").Data());
        }

        Comp* t = AddComp(visual, "Text");
        t->color = color;
        t->raycast = false;
        t->font = font;
        t->fontSize = fontSize;
        t->alignment = y * 3 + x;
        t->lineSpacing = lineSpacing;
        t->text = TextOf(&node, "text");

        if (visual != parent)
        {
            double pivot = F(x * 0.5);
            visual->SetPv(MakeV2(pivot, 1 - y * 0.5));
            visual->SetAm(MakeV2(pivot * (1 - 1 / sx), 0));
            visual->SetAx(MakeV2(pivot + (1 - pivot) / sx, 1));
            visual->SetOffsetMin(kZero);
            visual->SetOffsetMax(kZero);
            visual->sc[0] = sx;
            visual->sc[1] = 1;
            visual->sc[2] = 1;
        }

        double stroke = F(NumOf(&node, "strokeWidth"));
        if (stroke > 0)
        {
            Col sc = ParseColor(TextOf(&node, "strokeColor", "#000000"));
            sc.a = F(sc.a * F(NumOf(&node, "opacity", 1)));
            Comp* outline = AddComp(visual, "Outline");
            outline->color = sc;
            outline->distance = MakeV2(stroke, -stroke);
        }
        texts++;
    }

    void Assembler::AddScroll(GO* owner, GO* content, const Json& spec, const String& path)
    {
        int index = content->SiblingIndex();
        GO* viewport = NewRect(UniqueName(owner, "Viewport"), owner);
        Stretch(viewport);
        viewport->SetSiblingIndex(index);
        AddComp(viewport, "RectMask2D");
        Comp* image = AddComp(viewport, "Image");
        image->color = Clear();
        image->raycast = true;
        content->SetParent(viewport);

        UBox cb = BoxOf(path + "/" + TextOf(&spec, "content"));
        V2 size = MakeV2(cb.w, cb.h);
        UBox ob = BoxOf(path);
        V2 extent = MakeV2(ob.w, ob.h);
        V2 span = Sub(content->Ax(), content->Am());
        content->SetAm(MakeV2(0, F(1 - span.y)));
        content->SetAx(MakeV2(span.x, 1));
        content->SetSd(Sub(size, Scale(extent, span)));
        V2 offset = PairOf(spec.Get("offset"));
        V2 lim = MakeV2(std::max(0.0, F(size.x - extent.x)), std::max(0.0, F(size.y - extent.y)));
        offset = MakeV2(std::min(offset.x, lim.x), std::min(offset.y, lim.y));
        content->SetAp(MakeV2(-offset.x, offset.y));

        Comp* sr = AddComp(owner, "ScrollRect");
        sr->viewport = viewport;
        sr->content = content;
        sr->horizontal = TextOf(&spec, "direction") != "vertical";
        sr->vertical = TextOf(&spec, "direction") != "horizontal";
    }

    void Assembler::SetRectangle(GO* rect, const Json& node, const UBox& box, const V2& parent, const Json* parentNode, bool stretchRoot)
    {
        const Json* a = node.Get("anchor");
        String h = TextOf(a, "horizontal"), v = TextOf(a, "vertical");
        V2 low = MakeV2(h == "right" ? 1 : h == "center" ? 0.5 : 0, v == "bottom" ? 0 : v == "middle" ? 0.5 : 1);
        V2 high = low;
        if (stretchRoot)
        {
            low = MakeV2(0, 0);
            high = MakeV2(1, 1);
        }
        else if (IsObjJ(node.Get("responsive")))
        {
            const Json* responsive = node.Get("responsive");
            V2 first = PairOf(responsive->Get("min")), last = PairOf(responsive->Get("max"));
            low = MakeV2(first.x, F(1 - last.y));
            high = MakeV2(last.x, F(1 - first.y));
        }
        else if (parentNode && IsObjJ(parentNode->Get("layout")))
        {
            const Json* layout = parentNode->Get("layout");
            auto kids = ChildrenOf(*parentNode);
            int n = kids.Count();
            int index = -1;
            for (int i = 0; i < n && index < 0; i++)
            {
                if (TextOf(kids[i], "name") == TextOf(&node, "name"))
                    index = i;
            }
            bool row = TextOf(layout, "type") == "row";
            String mode = TextOf(layout, "align", "start");
            if (!IsDistributed(mode) && TextOf(layout, "spacing") == "even")
                mode = "space-evenly";
            double main = mode == "space-evenly" ? F((index + 1) / (double)(n + 1)) :
                mode == "space-around" ? F((index + 0.5) / n) :
                mode == "space-between" ? (n == 1 ? 0 : F(index / (double)(n - 1))) : FactorF(mode);
            double cross = FactorF(TextOf(&node, row ? "vAlign" : "align", TextOf(layout, "vAlign", "start")));
            low = row ? MakeV2(main, F(1 - cross)) : MakeV2(cross, F(1 - main));
            high = low;
        }
        else
        {
            if (node.Has("align"))
                low.x = high.x = FactorF(TextOf(&node, "align"));
            if (node.Has("vAlign"))
                low.y = high.y = F(1 - FactorF(TextOf(&node, "vAlign")));
        }

        rect->SetPv(MakeV2(0, 1));
        rect->SetAm(low);
        rect->SetAx(high);
        rect->SetOffsetMin(MakeV2(box.x - low.x * parent.x, parent.y - box.y - box.h - low.y * parent.y));
        rect->SetOffsetMax(MakeV2(box.x + box.w - high.x * parent.x, parent.y - box.y - high.y * parent.y));
    }

    void Assembler::AddProgressClip(GO* fill, double ratio, const String& direction, const V2& ref)
    {
        std::vector<GO*> original = fill->children;
        GO* clip = NewRect(UniqueName(fill, "ProgressClip"), fill);
        Stretch(clip);
        auto mask = std::make_unique<Comp>();
        mask->kind = "RectMask2D";
        mask->go = clip;
        clip->comps.push_back(std::move(mask));

        V2 low = MakeV2(0, 0), high = MakeV2(1, 1);
        if (direction == "left-to-right")
            high = MakeV2(ratio, high.y);
        else if (direction == "right-to-left")
            low = MakeV2(F(1 - ratio), low.y);
        else if (direction == "top-to-bottom")
            low = MakeV2(low.x, F(1 - ratio));
        else if (direction == "bottom-to-top")
            high = MakeV2(high.x, ratio);
        clip->SetAm(low);
        clip->SetAx(high);

        GO* content = NewRect("Content", clip);
        Stretch(content);
        if (ratio > 0)
        {
            V2 span = Sub(high, low);
            content->SetAm(MakeV2(-low.x / span.x, -low.y / span.y));
            content->SetAx(MakeV2((1 - low.x) / span.x, (1 - low.y) / span.y));
        }
        else
        {
            clip->active = false;
            content->SetAm(MakeV2(0, 1));
            content->SetAx(MakeV2(0, 1));
            content->SetSd(ref);
        }
        for (auto c : original)
            c->SetParent(content);
    }

    // UiPrefabLayout.Apply: the node's layout as a UGUI layout group
    void Assembler::ApplyLayout(GO* ownerIn, const Json& node, const std::vector<std::pair<String, GO*>>& kids)
    {
        const Json* spec = IsObjJ(node.Get("layout")) ? node.Get("layout") : nullptr;
        if (!spec)
            return;

        GO* owner = ownerIn;
        String kind = TextOf(spec, "type");
        bool grid = kind == "grid", row = kind == "row";
        V2 pad = IsObjJ(spec->Get("padding")) ? PairOf(spec->Get("padding")) : MakeV2(NumOf(spec, "padding"), NumOf(spec, "padding"));
        String mode = TextOf(spec, "align", "start");
        if (!grid && !IsDistributed(mode) && TextOf(spec, "spacing") == "even")
            mode = "space-evenly";
        bool distributed = !grid && IsDistributed(mode);
        auto authored = ChildrenOf(node);

        auto kidOf = [&](const String& name) -> GO*
        {
            for (auto& k : kids)
            {
                if (k.first == name)
                    return k.second;
            }
            throw ExportError(("layout: no child " + name).Data());
        };
        bool foreign = false;
        for (auto c : owner->children)
        {
            bool known = false;
            for (auto& k : kids)
                known = known || k.second == c;
            foreign = foreign || !known;
        }
        if (foreign)
        {
            GO* content = NewRect(UniqueName(owner, "LayoutContent"), owner);
            content->SetAm(MakeV2(0, 0));
            content->SetAx(MakeV2(1, 1));
            content->SetOffsetMin(kZero);
            content->SetOffsetMax(kZero);
            for (auto& k : kids)
                k.second->SetParent(content);
            owner = content;
        }

        double mainPadding = row ? pad.x : pad.y;
        double spacing = grid || distributed ? 0 : F(NumOf(spec, "spacing"));
        int n = authored.Count();
        if (distributed && n > 0)
        {
            double total = 0;
            for (auto c : authored)
            {
                V2 s = SizeOf(c->Get("size"), "layout child");
                total = F(total + (row ? s.x : s.y));
            }
            V2 ownerSize = RectSize(owner);
            double available = std::max(0.0, F((row ? ownerSize.x : ownerSize.y) - 2 * mainPadding - total));
            spacing = mode == "space-between" ? (n > 1 ? F(available / (n - 1)) : 0) :
                mode == "space-around" ? F(available / n) : F(available / (n + 1));
            if (mode != "space-between")
                mainPadding = F(mainPadding + spacing * (mode == "space-around" ? 0.5 : 1));
        }

        auto group = std::make_unique<Comp>();
        group->go = owner;
        if (grid)
        {
            group->kind = "Grid";
            group->cell = SizeOf(spec->Get("cellSize"), "grid cell");
            group->gridSpacing = PairOf(spec->Get("spacing"));
            group->columns = (int)std::trunc(NumOf(spec, "columns"));
        }
        else
        {
            group->kind = row ? "HLG" : "VLG";
            group->spacing = spacing;
        }
        double hp = row && distributed ? mainPadding : pad.x;
        double vp = !row && distributed ? mainPadding : pad.y;
        group->padding[0] = group->padding[1] = (int)RoundEven(hp);
        group->padding[2] = group->padding[3] = (int)RoundEven(vp);
        int mainFactor = distributed ? (mode == "space-between" ? 0 : 1) : FactorI(mode);
        int cross = FactorI(TextOf(spec, "vAlign", "start"));
        group->alignment = grid || row ? cross * 3 + mainFactor : mainFactor * 3 + cross;
        owner->comps.push_back(std::move(group));

        std::vector<GO*> order;
        for (auto child : authored)
        {
            GO* rect = kidOf(TextOf(child, "name"));
            V2 size = SizeOf(child->Get("size"), "layout child");
            V2 offset = PairOf(child->Get("offset"));
            int childCross = grid ? 0 : FactorI(TextOf(child, row ? "vAlign" : "align", TextOf(spec, "vAlign", "start")));
            bool needsSlot = !Eq2(offset, kZero) || !rect->ActiveSelf() || (!grid && childCross != cross);
            GO* slot = rect;
            if (needsSlot)
            {
                slot = NewRect(UniqueName(owner, rect->name + "_LayoutSlot"), owner);
                V2 os = RectSize(owner);
                slot->SetSd(!grid && childCross != cross ?
                    (row ? MakeV2(size.x, std::max(size.y, F(os.y - 2 * pad.y))) : MakeV2(std::max(size.x, F(os.x - 2 * pad.x)), size.y)) :
                    size);
                rect->SetParent(slot);
                rect->SetPv(MakeV2(0, 1));
                double fac = F(childCross * 0.5);
                V2 anchor = grid ? MakeV2(0, 1) : row ? MakeV2(0, F(1 - fac)) : MakeV2(fac, 1);
                rect->SetAm(anchor);
                rect->SetAx(anchor);
                rect->SetSd(size);
                rect->SetAp(grid ? MakeV2(offset.x, -offset.y) : row ? MakeV2(offset.x, fac * size.y - offset.y) :
                            MakeV2(offset.x - fac * size.x, -offset.y));
            }
            order.push_back(slot);
        }
        for (int i = 0; i < (int)order.size(); i++)
            order[i]->SetSiblingIndex(i);
    }

    void MarkDriven(GO* root)
    {
        root->drive.all = true;
        std::function<void(GO*)> walk = [&](GO* g)
        {
            Comp* group = nullptr;
            for (auto& c : g->comps)
            {
                if (!group && (c->kind == "Grid" || c->kind == "HLG" || c->kind == "VLG"))
                    group = c.get();
            }
            if (group && g->ActiveInHierarchy())
            {
                for (auto c : g->children)
                {
                    if (!c->ActiveInHierarchy())
                        continue;
                    c->drive.anchors = c->drive.x = c->drive.y = true;
                    if (group->kind == "Grid")
                        c->drive.sx = c->drive.sy = true;
                }
            }
            for (auto c : g->children)
                walk(c);
        };
        walk(root);
    }

    std::vector<GO*> Collect(GO* root)
    {
        std::vector<GO*> out;
        std::function<void(GO*)> walk = [&](GO* g)
        {
            out.push_back(g);
            for (auto c : g->children)
                walk(c);
        };
        walk(root);
        return out;
    }
}
