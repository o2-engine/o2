#pragma once

#include <gtest/gtest.h>

#include "o2Editor/Pipeline/UiDoc/UiDocAgent.h"
#include "o2Editor/Pipeline/UiDoc/UiDocEdit.h"
#include "o2Editor/Pipeline/UiDoc/UiDocStructure.h"
#include "o2Editor/Pipeline/UiDoc/UiDocUiml.h"
#include "o2Editor/Pipeline/UiDoc/UiKitPrep.h"

// The UI layout port's tests: the web's own tests (backend/test/uiLayout.test.ts, uiEdit.test.ts, the pure parts of
// dpi.test.ts) and golden checks against the web's outputs for the same inputs (UiDocGolden.inc, made by a script that
// runs the web's TypeScript)
namespace UiDocTest
{
    using namespace o2;
    using namespace Editor::UiDoc;

    // gtest prints o2 strings as character lists: compare them as std::string
#define EXPECT_STR_EQ(a, b) EXPECT_EQ(std::string(a), std::string(b))

    // The golden document: {uiml: [...], edits: [...], solve: [...], ...}
    const Json& Golden();

    // A golden section
    inline const Json& G(const char* key)
    {
        static const Json none;
        const Json* v = Golden().Get(key);
        return v ? *v : none;
    }

    // A member of a golden case (null when missing)
    inline const Json& M(const Json& obj, const char* key)
    {
        static const Json none;
        const Json* v = obj.Get(key);
        return v ? *v : none;
    }

    // A string member of a golden case
    inline String S(const Json& obj, const char* key)
    {
        return M(obj, key).Str();
    }

    // A golden array of strings
    inline Vector<String> Strings(const Json& arr)
    {
        Vector<String> out;
        for (auto& item : arr.Items())
            out.Add(item.Str());
        return out;
    }

    // The value as the golden script writes undefined
    inline bool IsUndef(const Json& v)
    {
        return v.IsString() && v.Str() == "__undef__";
    }

    // The first SCREEN of the web's uiLayout.test.ts
    inline String Screen()
    {
        return S(*G("uiml").At(0), "src");
    }

    // The SCREEN of the web's uiEdit.test.ts
    inline String Screen2()
    {
        return S(*G("uiml").At(1), "src");
    }

    // A box as the golden lists write it: [id, x, y, w, h]
    inline Json RectEntry(const String& id, const UiRect& r)
    {
        Json out = Json::Array();
        out.Add(Json(id));
        out.Add(Json(r.x));
        out.Add(Json(r.y));
        out.Add(Json(r.w));
        out.Add(Json(r.h));
        return out;
    }

    // A box as JSON {x, y, w, h}
    inline Json RectJson(const UiRect& r)
    {
        Json out = Json::Object();
        out["x"] = r.x;
        out["y"] = r.y;
        out["w"] = r.w;
        out["h"] = r.h;
        return out;
    }

    // A box from JSON {x, y, w, h}
    inline UiRect RectOf(const Json& j)
    {
        return UiRect(M(j, "x").Num(), M(j, "y").Num(), M(j, "w").Num(), M(j, "h").Num());
    }

    // An anchor as JSON {h, v}
    inline Json AnchorJson(const Anchor& a)
    {
        Json out = Json::Object();
        out["h"] = a.h;
        out["v"] = a.v;
        return out;
    }

    // A patch as JSON {set, unset}
    inline Json PatchJson(const UiPatch& p)
    {
        Json out = Json::Object();
        out["set"] = p.set;
        out["unset"] = JsonStrings(p.unset);
        return out;
    }

    // The web's kit for the golden cases (shared/uiKit parseKit of the golden kit JSON)
    inline UiKit GoldenKit()
    {
        return ParseKit(S(G("kit"), "json"));
    }

    // A document from UIML text
    inline UiDoc Parse(const String& src)
    {
        return ParseUiml(src).doc;
    }

    // The steps of a JSON array
    inline Vector<UiStep> StepsFrom(const Json& arr)
    {
        Vector<UiStep> out;
        for (auto& s : arr.Items())
            out.Add(UiStep(s));
        return out;
    }
}
