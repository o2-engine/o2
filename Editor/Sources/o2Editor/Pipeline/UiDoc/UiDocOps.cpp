#include "o2Editor/stdafx.h"
#include "UiDocOps.h"

#include <cmath>

#include "o2Editor/Pipeline/UiDoc/UiDocFp.h"

namespace Editor::UiDoc
{
    // The string value of a member, "" when it is no string
    static String StrOf(const Json& obj, const char* key)
    {
        const Json* v = JsGet(obj, key);
        return v && v->IsString() ? v->Str() : String();
    }

    // --- ops --------------------------------------------------------------------------------------------

    String OpKind(const UiOp& op)
    {
        return StrOf(op, "op");
    }

    UiOp MakeDocOp(const UiDoc& doc)
    {
        Json op = Json::Object();
        op["op"] = "doc";
        op["doc"] = DocToJson(doc);
        return op;
    }

    UiOp MakeScreenOp(const String& name, std::optional<double> width, std::optional<double> height, std::optional<double> dpi)
    {
        Json op = Json::Object();
        op["op"] = "screen";
        if (!name.IsEmpty())
            op["name"] = name;
        if (width)
            op["width"] = *width;
        if (height)
            op["height"] = *height;
        if (dpi)
            op["dpi"] = *dpi;
        return op;
    }

    UiOp MakeUpdateOp(const String& id, const Json& set, const Vector<String>& unset /*= {}*/)
    {
        Json op = Json::Object();
        op["op"] = "update";
        op["id"] = id;
        if (set.IsObject() && set.Count() > 0)
            op["set"] = set;
        if (!unset.IsEmpty())
            op["unset"] = JsonStrings(unset);
        return op;
    }

    UiOp MakeAddOp(const String& parent, const Vector<Json>& nodes, std::optional<double> index /*= std::nullopt*/,
                   const String& after /*= ""*/, const String& before /*= ""*/)
    {
        Json op = Json::Object();
        op["op"] = "add";
        op["parent"] = parent;
        Json list = Json::Array();
        for (auto& n : nodes)
            list.Add(n);
        op["nodes"] = list;
        if (index)
            op["index"] = *index;
        if (!after.IsEmpty())
            op["after"] = after;
        if (!before.IsEmpty())
            op["before"] = before;
        return op;
    }

    UiOp MakeRemoveOp(const Vector<String>& ids)
    {
        Json op = Json::Object();
        op["op"] = "remove";
        op["ids"] = JsonStrings(ids);
        return op;
    }

    UiOp MakeMoveOp(const String& id, const String& parent, std::optional<double> index /*= std::nullopt*/,
                    const String& after /*= ""*/, const String& before /*= ""*/)
    {
        Json op = Json::Object();
        op["op"] = "move";
        op["id"] = id;
        op["parent"] = parent;
        if (index)
            op["index"] = *index;
        if (!after.IsEmpty())
            op["after"] = after;
        if (!before.IsEmpty())
            op["before"] = before;
        return op;
    }

    UiOp MakeReplaceOp(const String& id, const Json& node)
    {
        Json op = Json::Object();
        op["op"] = "replace";
        op["id"] = id;
        op["node"] = node;
        return op;
    }

    // --- applying ------------------------------------------------------------------------------------------

    // The block with the id an op names (only a text id names one)
    static UiNode* FindById(UiDoc& doc, const Json* id)
    {
        return id && id->IsString() ? FindNode(doc, id->Str()) : nullptr;
    }

    // A block by id. The screen itself is root; the agent also names it by the screen's name or as "screen". A missing or
    // non-text id fails as the web's id.toLowerCase() does
    static UiNode* BlockOf(UiDoc& doc, const Json* id, String& error)
    {
        if (UiNode* hit = FindById(doc, id))
            return hit;
        if (!id || !id->IsString())
        {
            error = "id.toLowerCase is not a function";
            return nullptr;
        }
        String k = JsToLower(id->Str());
        return k == "screen" || k == JsToLower(doc.name) ? &doc.root : nullptr;
    }

    // Every pixel value of a block and of the blocks in it, times k
    static void ScaleNode(UiNode& n, double k)
    {
        auto px = [&](double v) { return JsRound(v*k*10)/10; };
        for (auto* v : { &n.x, &n.y, &n.w, &n.h, &n.gap, &n.gapY, &n.cellW, &n.cellH, &n.fontSize })
        {
            if (*v)
                **v = px(**v);
        }
        for (auto* box : { &n.margin, &n.pad })
        {
            if (*box)
            {
                for (auto& v : **box)
                    v = px(v);
            }
        }
        for (auto& c : n.children)
            ScaleNode(c, k);
    }

    static UiNode* FindParentById(UiDoc& doc, const Json* id)
    {
        return id && id->IsString() ? FindParent(doc, id->Str()) : nullptr;
    }

    // Every id of the document but those inside except
    static UiIdBook BookOf(const UiDoc& doc, const UiNode* except)
    {
        UiIdBook book;
        Walk(doc.root, [&](const UiNode& n, const UiNode*, int) { book.used[n.id] = true; });
        if (except)
            Walk(*except, [&](const UiNode& n, const UiNode*, int) { book.used.erase(n.id); });
        return book;
    }

    // Where in the parent's children an add / move puts its block
    static int PlaceIndex(const std::vector<UiNode>& kids, const UiOp& place)
    {
        const Json* after = JsGet(place, "after");
        const Json* before = JsGet(place, "before");
        const Json* index = JsGet(place, "index");
        auto find = [&](const Json* id) {
            for (int i = 0; i < (int)kids.size(); i++)
            {
                if (id->IsString() && kids[i].id == id->Str())
                    return i;
            }
            return -1;
        };

        if (JsTruthy(after))
        {
            int i = find(after);
            if (i >= 0)
                return i + 1;
        }
        if (JsTruthy(before))
        {
            int i = find(before);
            if (i >= 0)
                return i;
        }
        if (index && index->IsNumber() && std::isfinite(index->Num()))
        {
            double i = JsRound(index->Num());
            int len = (int)kids.size();
            return i < 0 ? (int)std::max(0.0, len + 1 + i) : (int)std::min((double)len, i);
        }
        return (int)kids.size();
    }

    static bool IsInside(const UiNode& node, const String& id)
    {
        bool hit = false;
        Walk(node, [&](const UiNode& n, const UiNode*, int) { if (n.id == id) hit = true; });
        return hit;
    }

    // The keys an update may set on the root
    static bool IsRootKey(const String& key)
    {
        static const Vector<String> keys = { "layout", "gap", "gapY", "pad", "align", "justify", "cols", "cellW", "cellH",
                                             "kit", "variant", "color", "opacity", "note" };
        return keys.Contains(key);
    }

    // Sets the fields of an update on a node, in order; false with the message at the first that cannot be set
    static bool SetProps(UiDoc& doc, UiNode& node, const Json& set, bool isRoot, String& error)
    {
        for (auto& member : set.Members())
        {
            const String& k = member.first;
            const Json& v = member.second;
            if (k == "type")
            {
                if (isRoot)
                    continue;
                String type, layout;
                if (!ResolveType(JsStringOr(&v), type, layout) || type == "screen")
                {
                    error = node.id + ": unknown type \"" + v.ToJsString() + "\"";
                    return false;
                }
                node.type = type;
                if (!layout.IsEmpty())
                    node.Set("layout", Json(layout));
                continue;
            }
            if (k == "id")
            {
                if (isRoot)
                    continue;
                String id = CleanId(&v);
                if (id.IsEmpty() || id == node.id)
                    continue;
                if (FindNode(doc, id))
                {
                    error = node.id + ": cannot rename to \"" + id + "\" \xE2\x80\x94 taken";
                    return false;
                }
                node.id = id;
                continue;
            }
            if (!NodeKeys().Contains(k))
                continue;
            if (isRoot && !IsRootKey(k))
                continue;

            Json cleaned;
            if (CleanProp(k, &v, cleaned))
                node.Set(k, cleaned);
            else
                node.Unset(k);
        }
        return true;
    }

    bool ApplyOp(UiDoc& doc, const UiOp& op, String& error)
    {
        String kind = OpKind(op);
        if (kind == "doc")
        {
            const Json* d = JsGet(op, "doc");
            doc = NormalizeDoc(d ? *d : Json());
            return true;
        }

        if (kind == "screen")
        {
            const Json* name = JsGet(op, "name");
            if (JsTruthy(name))
            {
                String n = CleanId(name);
                if (!n.IsEmpty())
                    doc.name = n;
            }
            const Json* width = JsGet(op, "width");
            const Json* height = JsGet(op, "height");
            if (JsTruthy(width) && JsTruthy(height) && JsNumber(width) >= 64 && JsNumber(height) >= 64)
            {
                doc.width = std::min(8192.0, JsRound(JsNumber(width)));
                doc.height = std::min(8192.0, JsRound(JsNumber(height)));
            }
            if (const Json* dpi = JsGet(op, "dpi"))
            {
                if (auto d = CleanDpi(dpi))
                {
                    if (!doc.dpi)
                        doc.dpiAfterRoot = true;
                    doc.dpi = d;
                }
            }
            const Json* scale = JsGet(op, "scale");
            double k = JsNumber(scale);
            if (JsTruthy(scale) && k > 0 && std::fabs(k - 1) > 1e-6)
                ScaleNode(doc.root, k);
            return true;
        }

        if (kind == "update")
        {
            String thrown;
            UiNode* node = BlockOf(doc, JsGet(op, "id"), thrown);
            if (!node)
            {
                error = !thrown.IsEmpty() ? thrown : "@update: no block \"" + JsString(JsGet(op, "id")) + "\"";
                return false;
            }

            bool isRoot = node == &doc.root;
            const Json* set = JsGet(op, "set");
            if (JsTruthy(set) && set->IsObject() && !SetProps(doc, *node, *set, isRoot, error))
                return false;

            // A block keeps its id, type and children (an unset of them stored by an older editor too)
            for (auto& k : JsonToStrings(JsGet(op, "unset")))
            {
                if (k != "id" && k != "type" && k != "children")
                    node->Unset(k);
            }

            // The text form writes the anchor canonically; keep stored ones canonical too
            if (node->anchor && !node->anchor->IsEmpty())
                node->anchor = AnchorName(ParseAnchor(*node->anchor));
            if (!node->layout || node->layout->IsEmpty())
            {
                for (const char* k : { "gap", "gapY", "pad", "align", "justify", "cols", "cellW", "cellH" })
                    node->Unset(k);
            }
            return true;
        }

        if (kind == "add")
        {
            const Json* parentId = JsGet(op, "parent");
            String thrown;
            Json rootId("root");
            UiNode* parent = BlockOf(doc, JsTruthy(parentId) ? parentId : &rootId, thrown);
            if (!parent)
            {
                error = !thrown.IsEmpty() ? thrown : "@add: no block \"" + JsString(parentId) + "\" to add into";
                return false;
            }

            const Json* nodes = JsGet(op, "nodes");
            if (!nodes || !nodes->IsArray())
            {
                error = nodes ? "op.nodes.map is not a function" : "Cannot read properties of undefined (reading 'map')";
                return false;
            }

            UiIdBook book = BookOf(doc, nullptr);
            std::vector<UiNode> fresh;
            for (auto& n : nodes->Items())
                fresh.push_back(NormalizeNode(n, book));

            std::vector<UiNode> kids = parent->children;
            int at = PlaceIndex(kids, op);
            kids.insert(kids.begin() + at, fresh.begin(), fresh.end());
            parent->SetChildren(kids);
            return true;
        }

        if (kind == "remove")
        {
            const Json* ids = JsGet(op, "ids");
            if (!ids || !ids->IsArray())
            {
                error = "op.ids is not iterable";
                return false;
            }

            for (auto& idValue : ids->Items())
            {
                if (idValue.IsString() && idValue.Str() == "root")
                    continue;
                UiNode* parent = FindParentById(doc, &idValue);
                if (!parent || !parent->hasChildren)
                {
                    error = "@remove: no block \"" + idValue.ToJsString() + "\"";
                    return false;
                }
                const String id = idValue.Str();
                std::vector<UiNode> kept;
                for (auto& c : parent->children)
                {
                    if (c.id != id)
                        kept.push_back(c);
                }
                if (kept.empty())
                    parent->ClearChildren();
                else
                    parent->SetChildren(kept);
            }
            return true;
        }

        if (kind == "move")
        {
            const Json* idValue = JsGet(op, "id");
            UiNode* node = FindById(doc, idValue);
            UiNode* from = FindParentById(doc, idValue);
            if (!node || !from)
            {
                error = "@move: no block \"" + JsString(idValue) + "\"";
                return false;
            }

            const Json* parentId = JsGet(op, "parent");
            String thrown;
            UiNode* to = JsTruthy(parentId) ? BlockOf(doc, parentId, thrown) : from;
            if (!to)
            {
                error = !thrown.IsEmpty() ? thrown : "@move: no block \"" + JsString(parentId) + "\" to move into";
                return false;
            }
            if (IsInside(*node, to->id))
            {
                error = "@move: \"" + JsString(idValue) + "\" cannot go inside itself";
                return false;
            }

            // Copies first: the vectors below move nodes around
            UiNode moved = *node;
            const String id = idValue->Str(), toId = to->id;
            bool same = from == to;

            std::vector<UiNode> rest;
            for (auto& c : from->children)
            {
                if (c.id != id)
                    rest.push_back(c);
            }
            from->children = rest;
            // The web drops an emptied source's children after the insert; nothing of the target depends on it
            if (rest.empty() && !same)
                from->ClearChildren();

            UiNode* target = same ? from : FindNode(doc, toId);
            std::vector<UiNode> kids = target->children;
            int at = PlaceIndex(kids, op);
            kids.insert(kids.begin() + at, moved);
            target->SetChildren(kids);
            return true;
        }

        if (kind == "replace")
        {
            const Json* idValue = JsGet(op, "id");
            UiNode* old = FindById(doc, idValue);
            if (!old)
            {
                error = "@replace: no block \"" + JsString(idValue) + "\"";
                return false;
            }

            const Json* nodeValue = JsGet(op, "node");
            if (old == &doc.root)
            {
                Json raw = Json::Object();
                if (nodeValue && nodeValue->IsObject())
                    raw = *nodeValue;
                raw["id"] = "root";
                UiIdBook book;
                doc.root = NormalizeNode(raw, book, true);
                doc = NormalizeDoc(DocToJson(doc));
                return true;
            }

            UiIdBook book = BookOf(doc, old);
            UiNode fresh = NormalizeNode(nodeValue ? *nodeValue : Json(), book);
            UiNode* parent = FindParentById(doc, idValue);
            for (auto& c : parent->children)
            {
                if (c.id == idValue->Str())
                    c = fresh;
            }
            return true;
        }

        return true;
    }

    UiApplyResult ApplyOps(const UiDoc& doc, const Vector<UiOp>& ops)
    {
        UiApplyResult result;
        result.doc = doc;
        for (auto& op : ops)
        {
            String error;
            if (!ApplyOp(result.doc, op, error))
                result.errors.Add(error);
        }
        return result;
    }

    UiOp MakeRescaleOp(double width, double height, double dpi, double scale, double fromDpi)
    {
        Json op = Json::Object();
        op["op"] = "screen";
        op["width"] = width;
        op["height"] = height;
        op["dpi"] = dpi;
        op["scale"] = scale;
        op["fromDpi"] = fromDpi;
        return op;
    }

    Vector<UiStepPoint> CleanPoints(const Json* raw, double w /*= INFINITY*/, double h /*= INFINITY*/)
    {
        Vector<UiStepPoint> out;
        if (!raw || !raw->IsArray())
            return out;

        for (auto& p : raw->Items())
        {
            if (!p.IsObject() && !p.IsArray())
                continue;
            double x = JsNumber(JsGet(p, "x")), y = JsNumber(JsGet(p, "y"));
            String text = JsSlice(JsTrim(JsStringOr(JsGet(p, "text"))), 0, 2000);
            if (!std::isfinite(x) || !std::isfinite(y))
                continue;
            out.Add(UiStepPoint{ std::max(0.0, std::min(w, JsRound(x))), std::max(0.0, std::min(h, JsRound(y))), text });
            if (out.Count() >= 20)
                break;
        }
        return out;
    }

    Json PointsToJson(const Vector<UiStepPoint>& points)
    {
        Json out = Json::Array();
        for (auto& p : points)
        {
            Json j = Json::Object();
            j["x"] = p.x;
            j["y"] = p.y;
            j["text"] = p.text;
            out.Add(j);
        }
        return out;
    }

    // Every [n] token (one or two digits) of a text: where it starts, how long it is, its number
    struct PinToken
    {
        size_t at = 0, length = 0;
        int    n = 0;
    };
    static Vector<PinToken> PinTokens(const String& text)
    {
        Vector<PinToken> out;
        const std::string& s = text;
        size_t i = 0;
        while (i < s.size())
        {
            if (s[i] == '[')
            {
                size_t j = i + 1;
                while (j < s.size() && j - i - 1 < 2 && s[j] >= '0' && s[j] <= '9')
                    j++;
                size_t digits = j - i - 1;
                if (digits >= 1 && j < s.size() && s[j] == ']')
                {
                    out.Add(PinToken{ i, j - i + 1, std::stoi(s.substr(i + 1, digits)) });
                    i = j + 1;
                    continue;
                }
            }
            i++;
        }
        return out;
    }

    Vector<int> PinsNamed(const String& text, int count)
    {
        Vector<int> order;
        for (auto& t : PinTokens(text))
        {
            if (t.n >= 1 && t.n <= count && !order.Contains(t.n))
                order.Add(t.n);
        }
        return order;
    }

    String DraftWithNotes(const String& draft, const Vector<UiStepPoint>& points)
    {
        Vector<int> named = PinsNamed(draft, points.Count());
        String text = draft;
        for (int i = 0; i < points.Count(); i++)
        {
            String note = JsTrim(points[i].text);
            if (named.Contains(i + 1) || note.IsEmpty())
                continue;
            text = JsTrimEnd(text) + (JsTrim(text).IsEmpty() ? "" : "\n") + "[" + JsNum(i + 1) + "] " + note;
        }
        return text;
    }

    UiPinnedRequest RequestWithPins(const String& draft, const Vector<UiStepPoint>& points)
    {
        String full = DraftWithNotes(draft, points);
        Vector<int> order = PinsNamed(full, points.Count());

        const std::string& s = full;
        std::string text;
        size_t from = 0;
        for (auto& t : PinTokens(full))
        {
            text += s.substr(from, t.at - from);
            int i = order.IndexOf(t.n);
            if (i >= 0 && t.n <= points.Count())
                text += "[" + std::to_string(i + 1) + "]";
            else
                text += s.substr(t.at, t.length);
            from = t.at + t.length;
        }
        text += s.substr(from);

        UiPinnedRequest out;
        out.text = String(text);
        for (int n : order)
            out.points.Add(UiStepPoint{ points[n - 1].x, points[n - 1].y, "" });
        return out;
    }
}
