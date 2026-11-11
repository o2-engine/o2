#include "o2Editor/stdafx.h"
#include "UiDocUiml.h"

#include <cmath>
#include <optional>

#include "o2Editor/Pipeline/UiDoc/UiDocUimlText.h"
#include "o2Editor/Pipeline/UiDoc/UiDocFp.h"

// The edit commands of UIML (@update, @add, @remove, @move, @replace, @screen) and their text form
namespace Editor::UiDoc
{
    using namespace UimlText;

    // --- edits ------------------------------------------------------------------------------------------

    // The key=value tokens of an @update -> fields to set (raw, in order) and keys to remove
    static void UpdateOf(const Vector<UimlToken>& toks, Json& set, Vector<String>& unset)
    {
        set = Json::Object();
        Vector<String> texts;
        for (auto& tk : toks)
        {
            if (!tk.key.IsEmpty())
            {
                String lowerValue = JsToLower(tk.value);
                if (!tk.quoted && (tk.value == "-" || tk.value.IsEmpty() || lowerValue == "none" || lowerValue == "null"))
                {
                    String k = AliasKey(tk.key);
                    // A block keeps its id, type and children: those change by @update id=/type=, @remove and @move
                    if (k == "id" || k == "type" || k == "children" || k == "rename")
                        continue;
                    if (k == "size") { unset.Add("w"); unset.Add("h"); }
                    else if (k == "pos") { unset.Add("x"); unset.Add("y"); }
                    else if (k == "cell") { unset.Add("cellW"); unset.Add("cellH"); }
                    else unset.Add(k);
                    continue;
                }

                String lowerKey = JsToLower(tk.key);
                if (lowerKey == "type")
                {
                    set["type"] = tk.value;
                    continue;
                }
                if (lowerKey == "id" || lowerKey == "rename")
                {
                    set["id"] = tk.value;
                    continue;
                }
                AssignProps(set, PropsOf(tk.key, tk.value));
            }
            else if (tk.quoted)
                texts.Add(tk.value);
            else if (JsToLower(tk.value) == "hidden")
                set["visible"] = false;
            else if (JsToLower(tk.value) == "shown")
                set["visible"] = true;
        }
        if (!texts.IsEmpty())
            set["text"] = Join(texts, " ");
    }

    // ------------------------------------------
    // Where an @add / @move puts its blocks
    // ------------------------------------------
    struct UimlPlace
    {
        std::optional<double> index;
        String                after, before, to;
        bool                  hasTo = false;
    };

    static UimlPlace PlaceOf(const Vector<UimlToken>& toks)
    {
        UimlPlace out;
        for (auto& tk : toks)
        {
            if (tk.key.IsEmpty())
                continue;
            String k = JsToLower(tk.key);
            if (k == "index" || k == "at")
            {
                double n = Json(tk.value).ToJsNumber();
                if (std::isfinite(n))
                    out.index = JsRound(n);
            }
            else if (k == "after")
                out.after = CleanId(tk.value);
            else if (k == "before")
                out.before = CleanId(tk.value);
            else if (k == "to" || k == "parent" || k == "into")
            {
                out.to = CleanId(tk.value);
                out.hasTo = true;
            }
        }
        return out;
    }

    static void AddPlace(Json& op, const UimlPlace& place)
    {
        if (place.index)
            op["index"] = *place.index;
        if (!place.after.IsEmpty())
            op["after"] = place.after;
        if (!place.before.IsEmpty())
            op["before"] = place.before;
    }

    // Collects the ids of a block and its children
    static void CollectIds(const UiNode& n, UiIdBook& book)
    {
        book.used[n.id] = true;
        for (auto& c : n.children)
            CollectIds(c, book);
    }

    UimlEditsResult ParseEdits(const String& src, const UiDoc& doc)
    {
        UimlEditsResult result;
        auto& ops = result.ops;
        auto& errors = result.errors;
        auto all = LinesOf(src);
        UiIdBook book;
        CollectIds(doc.root, book);

        int i = 0;
        while (i < all.Count())
        {
            const UimlLine& ln = all[i];
            if (!StartsWith(ln.text, "@"))
            {
                errors.Add("line " + JsNum(ln.no) + ": expected an @command, got \"" + JsSlice(ln.text, 0, 60) + "\"");
                i++;
                continue;
            }

            // The command's body: the lines up to the next command (indented or not)
            Vector<String> bodyLines;
            int j = i + 1;
            while (j < all.Count() && !StartsWith(all[j].text, "@"))
            {
                bodyLines.Add(Spaces(all[j].indent) + all[j].text);
                j++;
            }
            String bodyText = Join(bodyLines, "\n");

            auto toks = TokenizeUiml(ln.text.SubStr(1));
            String cmd = toks.IsEmpty() ? String() : JsToLower(toks[0].value);
            String arg = toks.Count() > 1 && toks[1].key.IsEmpty() && !toks[1].quoted ? CleanId(toks[1].value) : String();
            Vector<UimlToken> restToks;
            for (int t = arg.IsEmpty() ? 1 : 2; t < toks.Count(); t++)
                restToks.Add(toks[t]);
            Vector<UimlToken> afterCmd;
            for (int t = 1; t < toks.Count(); t++)
                afterCmd.Add(toks[t]);
            String lineNo = "line " + JsNum(ln.no);

            if (cmd == "update" || cmd == "set" || cmd == "edit")
            {
                if (arg.IsEmpty())
                    errors.Add(lineNo + ": @update needs a block id");
                else
                {
                    Json set;
                    Vector<String> unset;
                    UpdateOf(restToks, set, unset);
                    Json op = Json::Object();
                    op["op"] = "update";
                    op["id"] = arg;
                    if (set.Count() > 0)
                        op["set"] = set;
                    if (!unset.IsEmpty())
                        op["unset"] = JsonStrings(unset);
                    ops.Add(op);
                }
            }
            else if (cmd == "add" || cmd == "insert")
            {
                UimlPlace place = PlaceOf(restToks);
                String parent = !arg.IsEmpty() ? arg : !place.to.IsEmpty() ? place.to : String("root");
                auto nodes = ParseUimlNodes(bodyText, book, errors);
                if (nodes.empty())
                    errors.Add(lineNo + ": @add has no blocks under it");
                else
                {
                    Json op = Json::Object();
                    op["op"] = "add";
                    op["parent"] = parent;
                    Json list = Json::Array();
                    for (auto& n : nodes)
                        list.Add(NodeToJson(n));
                    op["nodes"] = list;
                    AddPlace(op, place);
                    ops.Add(op);
                }
            }
            else if (cmd == "remove" || cmd == "delete")
            {
                Vector<String> ids;
                for (auto& t : afterCmd)
                {
                    if (t.key.IsEmpty() && !t.quoted)
                    {
                        String id = CleanId(t.value);
                        if (!id.IsEmpty())
                            ids.Add(id);
                    }
                }
                if (ids.IsEmpty())
                    errors.Add(lineNo + ": @remove needs block ids");
                else
                {
                    Json op = Json::Object();
                    op["op"] = "remove";
                    op["ids"] = JsonStrings(ids);
                    ops.Add(op);
                }
            }
            else if (cmd == "move")
            {
                UimlPlace place = PlaceOf(restToks);
                if (arg.IsEmpty())
                    errors.Add(lineNo + ": @move needs a block id");
                else
                {
                    Json op = Json::Object();
                    op["op"] = "move";
                    op["id"] = arg;
                    op["parent"] = place.to;
                    AddPlace(op, place);
                    ops.Add(op);
                }
            }
            else if (cmd == "replace")
            {
                if (arg.IsEmpty())
                    errors.Add(lineNo + ": @replace needs a block id");
                else
                {
                    book.used.erase(arg);
                    auto nodes = ParseUimlNodes(bodyText, book, errors);
                    if (nodes.size() != 1)
                        errors.Add(lineNo + ": @replace needs exactly one block under it");
                    else
                    {
                        Json op = Json::Object();
                        op["op"] = "replace";
                        op["id"] = arg;
                        op["node"] = NodeToJson(nodes[0]);
                        ops.Add(op);
                    }
                }
            }
            else if (cmd == "screen" || cmd == "canvas")
            {
                Json op = Json::Object();
                op["op"] = "screen";
                Vector<UimlToken> rootToks;
                for (auto& tk : afterCmd)
                {
                    String k = JsToLower(tk.key);
                    double w, h;
                    if (!tk.key.IsEmpty() && k != "name" && k != "size" && k != "dpi")
                        rootToks.Add(tk);

                    if (k == "name")
                    {
                        String name = CleanId(tk.value);
                        if (name.IsEmpty())
                            op.Remove("name");
                        else
                            op["name"] = name;
                    }
                    else if (k == "dpi")
                    {
                        double d = Json(tk.value).ToJsNumber();
                        if (d > 0)
                            op["dpi"] = d;
                    }
                    else if (k == "size")
                    {
                        auto parts = SplitOn(tk.value, U"x\u00D7,");
                        double pw = parts.Count() > 0 ? Json(parts[0]).ToJsNumber() : NAN;
                        double ph = parts.Count() > 1 ? Json(parts[1]).ToJsNumber() : NAN;
                        if (pw > 0 && ph > 0)
                        {
                            op["width"] = pw;
                            op["height"] = ph;
                        }
                    }
                    else if (tk.key.IsEmpty() && MatchDims(tk.value, w, h))
                    {
                        op["width"] = w;
                        op["height"] = h;
                    }
                    else if (tk.key.IsEmpty() && !tk.quoted && !CleanId(tk.value).IsEmpty())
                        op["name"] = CleanId(tk.value);
                }
                ops.Add(op);

                // A @screen with props of the root (layout etc.) is an update of the root
                Json set;
                Vector<String> unset;
                UpdateOf(rootToks, set, unset);
                if (set.Count() > 0 || !unset.IsEmpty())
                {
                    Json update = Json::Object();
                    update["op"] = "update";
                    update["id"] = "root";
                    if (set.Count() > 0)
                        update["set"] = set;
                    if (!unset.IsEmpty())
                        update["unset"] = JsonStrings(unset);
                    ops.Add(update);
                }
            }
            else
                errors.Add(lineNo + ": unknown command @" + cmd);

            i = j;
        }
        return result;
    }

    UiNode CleanSet(const Json& set)
    {
        UiNode out;
        out.id = "";
        out.type = "";
        if (!set.IsObject())
            return out;

        for (auto& member : set.Members())
        {
            if (!NodeKeys().Contains(member.first))
                continue;
            Json cleaned;
            if (CleanProp(member.first, &member.second, cleaned))
                out.Set(member.first, cleaned);
        }
        return out;
    }

    // " after=x" / " before=x" / " index=n" of an @add / @move
    static String WhereOf(const UiOp& op)
    {
        if (JsTruthy(JsGet(op, "after")))
            return " after=" + JsString(JsGet(op, "after"));
        if (JsTruthy(JsGet(op, "before")))
            return " before=" + JsString(JsGet(op, "before"));
        if (op.Has("index"))
            return " index=" + JsString(JsGet(op, "index"));
        return "";
    }

    String OpsToText(const Vector<UiOp>& ops)
    {
        Vector<String> out;
        for (auto& op : ops)
        {
            String kind = JsStringOr(JsGet(op, "op"));
            if (kind == "doc")
                out.Add("@layout (whole screen rewritten)");
            else if (kind == "screen")
            {
                Vector<String> parts = { "@screen" };
                if (JsTruthy(JsGet(op, "name")))
                    parts.Add("name=" + JsString(JsGet(op, "name")));
                if (JsTruthy(JsGet(op, "width")) && JsTruthy(JsGet(op, "height")))
                    parts.Add("size=" + JsString(JsGet(op, "width")) + "x" + JsString(JsGet(op, "height")));
                if (JsTruthy(JsGet(op, "dpi")))
                    parts.Add("dpi=" + JsString(JsGet(op, "dpi")));
                out.Add(Join(parts, " "));
            }
            else if (kind == "update")
            {
                const Json* set = JsGet(op, "set");
                Vector<String> tokens = JsTruthy(set) ? PropTokens(CleanSet(*set)) : Vector<String>();
                if (JsTruthy(JsGet(set, "type")))
                    tokens.Insert("type=" + JsString(JsGet(set, "type")), 0);
                if (JsTruthy(JsGet(set, "id")))
                    tokens.Insert("id=" + JsString(JsGet(set, "id")), 0);

                // Array#join writes a missing id as ""
                Vector<String> parts = { "@update", JsStringOr(JsGet(op, "id")) };
                parts.Add(tokens);
                if (set && set->Has("text"))
                    parts.Add(Quote(set->Get("text")->ToJsString()));
                for (auto& k : JsonToStrings(JsGet(op, "unset")))
                    parts.Add(k + "=-");
                out.Add(Join(parts, " "));
            }
            else if (kind == "add")
            {
                out.Add("@add " + JsString(JsGet(op, "parent")) + WhereOf(op));
                if (const Json* nodes = JsGet(op, "nodes"); nodes && nodes->IsArray())
                {
                    for (auto& n : nodes->Items())
                        out.Add(NodeToUiml(NodeFromJson(n), 1));
                }
            }
            else if (kind == "remove")
                out.Add("@remove " + Join(JsonToStrings(JsGet(op, "ids")), " "));
            else if (kind == "move")
            {
                const Json* parent = JsGet(op, "parent");
                out.Add("@move " + JsString(JsGet(op, "id")) + " to=" + (JsTruthy(parent) ? parent->ToJsString() : String("(same)")) +
                        WhereOf(op));
            }
            else if (kind == "replace")
            {
                out.Add("@replace " + JsString(JsGet(op, "id")));
                out.Add(NodeToUiml(NodeFromJson(JsGet(op, "node") ? *JsGet(op, "node") : Json()), 1));
            }
        }
        return Join(out, "\n");
    }
}

