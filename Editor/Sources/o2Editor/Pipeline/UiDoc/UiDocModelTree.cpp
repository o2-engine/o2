#include "o2Editor/stdafx.h"
#include "UiDocModel.h"

#include "o2Editor/Pipeline/UiDoc/UiDocFp.h"

// A layout document as JSON (in the web's key order) and the tree helpers
namespace Editor::UiDoc
{
    // --- JSON ----------------------------------------------------------------------------------------

    Json NodeToJson(const UiNode& node, bool withChildren /*= true*/)
    {
        Json out = Json::Object();
        out["id"] = node.id;
        out["type"] = node.type;
        for (auto& key : node.keys)
        {
            if (key == "children")
            {
                if (!withChildren)
                    continue;
                Json kids = Json::Array();
                for (auto& c : node.children)
                    kids.Add(NodeToJson(c));
                out["children"] = kids;
            }
            else
                out[key] = node.Get(key);
        }
        return out;
    }

    Json DocToJson(const UiDoc& doc)
    {
        Json out = Json::Object();
        out["name"] = doc.name;
        out["width"] = doc.width;
        out["height"] = doc.height;
        if (doc.dpi && !doc.dpiAfterRoot)
            out["dpi"] = *doc.dpi;
        out["root"] = NodeToJson(doc.root);
        if (doc.dpi && doc.dpiAfterRoot)
            out["dpi"] = *doc.dpi;
        return out;
    }

    UiNode NodeFromJson(const Json& raw)
    {
        UiNode out;
        out.id = JsStringOr(JsGet(raw, "id"));
        out.type = JsStringOr(JsGet(raw, "type"));
        if (!raw.IsObject())
            return out;

        for (auto& member : raw.Members())
        {
            const String& key = member.first;
            const Json& v = member.second;
            if (key == "children")
            {
                if (v.IsArray())
                {
                    std::vector<UiNode> kids;
                    for (auto& c : v.Items())
                        kids.push_back(NodeFromJson(c));
                    out.SetChildren(kids);
                }
                continue;
            }
            if (!NodeKeys().Contains(key) || v.IsNull())
                continue;

            if (key == "margin" || key == "pad")
            {
                if (auto b = ParseBox4(&v))
                {
                    Json box = Json::Array();
                    for (double d : *b)
                        box.Add(Json(d));
                    out.Set(key, box);
                }
                continue;
            }
            out.Set(key, v);
        }
        return out;
    }

    // --- tree helpers --------------------------------------------------------------------------------

    void Walk(const UiNode& node, const std::function<void(const UiNode&, const UiNode*, int)>& fn, const UiNode* parent /*= nullptr*/,
              int depth /*= 0*/)
    {
        fn(node, parent, depth);
        for (auto& c : node.children)
            Walk(c, fn, &node, depth + 1);
    }

    // The first block with the id under node (node included), with its parent
    static UiNode* FindIn(UiNode& node, UiNode* parent, const String& id, UiNode** parentOut)
    {
        if (node.id == id)
        {
            if (parentOut)
                *parentOut = parent;
            return &node;
        }
        for (auto& c : node.children)
        {
            if (auto hit = FindIn(c, &node, id, parentOut))
                return hit;
        }
        return nullptr;
    }

    UiNode* FindNode(UiDoc& doc, const String& id)
    {
        return FindIn(doc.root, nullptr, id, nullptr);
    }

    const UiNode* FindNode(const UiDoc& doc, const String& id)
    {
        return FindNode(const_cast<UiDoc&>(doc), id);
    }

    UiNode* FindParent(UiDoc& doc, const String& id)
    {
        UiNode* parent = nullptr;
        return FindIn(doc.root, nullptr, id, &parent) ? parent : nullptr;
    }

    const UiNode* FindParent(const UiDoc& doc, const String& id)
    {
        return FindParent(const_cast<UiDoc&>(doc), id);
    }

    int CountNodes(const UiDoc& doc)
    {
        int n = 0;
        Walk(doc.root, [&](const UiNode&, const UiNode*, int) { n++; });
        return n - 1;
    }

    Vector<String> AllIds(const UiDoc& doc)
    {
        Vector<String> out;
        Walk(doc.root, [&](const UiNode& n, const UiNode*, int) { if (!out.Contains(n.id)) out.Add(n.id); });
        return out;
    }
}

