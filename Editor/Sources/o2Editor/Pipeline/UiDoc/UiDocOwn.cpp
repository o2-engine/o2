#include "o2Editor/stdafx.h"
#include "UiDocOwn.h"

#include <cmath>

#include "o2Editor/Pipeline/UiDoc/UiDocFp.h"

namespace Editor::UiDoc
{
    std::optional<UiOwnAsset> CleanOwnAsset(const Json& raw)
    {
        if (!raw.IsObject())
            return std::nullopt;

        String uploadId = JsTrim(JsStringOr(raw.Get("uploadId")));
        if (uploadId.IsEmpty() || uploadId.Contains("/") || uploadId.Contains("\\"))
            return std::nullopt;

        String name = CleanKitName(raw.Get("name"), "");
        if (name.IsEmpty())
            return std::nullopt;

        UiOwnAsset a;
        a.name = name;
        String type = JsString(raw.Get("type"));
        a.type = UiElementTypes().Contains(type) ? type : GuessElementType(name);
        String assetType = JsString(raw.Get("assetType"));
        a.assetType = UiAssetTypes().Contains(assetType) ? assetType : DefaultAssetType(a.type);
        a.uploadId = uploadId;
        auto size = [&](const char* key) {
            double v = JsNumber(raw.Get(key));
            return std::max(1.0, JsRound(std::isnan(v) || v == 0 ? 1.0 : v));
        };
        a.width = size("width");
        a.height = size("height");
        if (JsTruthy(raw.Get("variant")))
            a.variant = JsSlice(JsString(raw.Get("variant")), 0, 40);
        if (JsTruthy(raw.Get("description")))
            a.description = JsSlice(JsString(raw.Get("description")), 0, 300);
        if (JsTruthy(raw.Get("purpose")))
            a.purpose = JsSlice(JsString(raw.Get("purpose")), 0, 300);
        auto nine = NormalizeNine(raw.Get("nineSlice"));
        if (nine && a.assetType != "simple")
            a.nineSlice = nine;
        a.dpi = CleanDpi(raw.Get("dpi"));
        String from = JsStringOr(raw.Get("from"));
        if (from == "drawn" || from == "cut")
            a.from = from;
        return a;
    }

    Json OwnAssetToJson(const UiOwnAsset& a)
    {
        Json out = Json::Object();
        out["name"] = a.name;
        out["type"] = a.type;
        out["assetType"] = a.assetType;
        out["uploadId"] = a.uploadId;
        out["width"] = a.width;
        out["height"] = a.height;
        if (a.variant)
            out["variant"] = *a.variant;
        if (a.description)
            out["description"] = *a.description;
        if (a.purpose)
            out["purpose"] = *a.purpose;
        if (a.nineSlice)
            out["nineSlice"] = NineToJson(*a.nineSlice);
        if (a.dpi)
            out["dpi"] = *a.dpi;
        if (a.from)
            out["from"] = *a.from;
        return out;
    }

    // A step that brought the layout to other units: the scale and how the density of what was made before changes
    static bool RescaleOf(const UiStep& step, double& scale, double& dpiK)
    {
        for (auto& op : step.Ops())
        {
            if (OpKind(op) != "screen")
                continue;
            const Json* s = op.Get("scale");
            double k = JsNumber(s);
            if (!JsTruthy(s) || !(k > 0) || std::fabs(k - 1) < 1e-6)
                continue;
            auto to = CleanDpi(op.Get("dpi")), from = CleanDpi(op.Get("fromDpi"));
            scale = k;
            dpiK = to && from ? *to / *from / k : 1/k;
            return true;
        }
        return false;
    }

    // An ordered map by lower-cased name where a later entry goes to the end
    template<typename T>
    static void PutLast(Vector<Pair<String, T>>& list, const String& name, const T& value)
    {
        String key = JsToLower(name);
        int at = list.IndexOf([&](const Pair<String, T>& p) { return p.first == key; });
        if (at >= 0)
            list.RemoveAt(at);
        list.Add(Pair<String, T>(key, value));
    }

    static void RescaleAssets(Vector<Pair<String, UiOwnAsset>>& assets, double dpiK)
    {
        for (auto& kv : assets)
            kv.second.dpi = CleanDpi((kv.second.dpi ? *kv.second.dpi : UI_BASE_DPI)*dpiK);
    }

    static void AddAssets(Vector<Pair<String, UiOwnAsset>>& assets, const UiStep& step)
    {
        const Json* list = step.data.Get("assets");
        if (!list || !list->IsArray())
            return;
        for (auto& raw : list->Items())
        {
            if (auto a = CleanOwnAsset(raw))
                PutLast(assets, a->name, *a);
        }
    }

    UiOwnKit OwnKitOf(const Vector<UiStep>& steps)
    {
        Vector<Pair<String, UiOwnAsset>> assets;
        Vector<Pair<String, TextStyle>> styles;
        for (auto& s : steps)
        {
            double scale = 1, dpiK = 1;
            if (RescaleOf(s, scale, dpiK))
            {
                RescaleAssets(assets, dpiK);
                for (auto& kv : styles)
                    kv.second = ScaleTextStyle(kv.second, scale);
            }
            AddAssets(assets, s);
            const Json* list = s.data.Get("styles");
            if (list && list->IsArray())
            {
                for (auto& raw : list->Items())
                {
                    TextStyle st = NormalizeTextStyle(raw, "style_" + JsNum(styles.Count() + 1));
                    PutLast(styles, st.name, st);
                }
            }
        }

        UiOwnKit out;
        for (auto& kv : assets)
            out.assets.Add(kv.second);
        for (auto& kv : styles)
            out.styles.Add(kv.second);
        return out;
    }

    Vector<UiOwnAsset> OwnAssetsOf(const Vector<UiStep>& steps)
    {
        Vector<Pair<String, UiOwnAsset>> assets;
        for (auto& s : steps)
        {
            double scale = 1, dpiK = 1;
            if (RescaleOf(s, scale, dpiK))
                RescaleAssets(assets, dpiK);
            AddAssets(assets, s);
        }

        Vector<UiOwnAsset> out;
        for (auto& kv : assets)
            out.Add(kv.second);
        return out;
    }

    Vector<String> OwnUploadIds(const Vector<UiStep>& steps)
    {
        Vector<String> out;
        for (auto& s : steps)
        {
            const Json* list = s.data.Get("assets");
            if (!list || !list->IsArray())
                continue;
            for (auto& raw : list->Items())
            {
                auto a = CleanOwnAsset(raw);
                if (a && !out.Contains(a->uploadId))
                    out.Add(a->uploadId);
            }
        }
        return out;
    }

    UiKitElement OwnElement(const UiOwnAsset& a, const String& png /*= ""*/)
    {
        UiKitElement e;
        e.name = a.name;
        e.type = a.type;
        e.assetType = a.assetType;
        e.width = a.width;
        e.height = a.height;
        e.png = png;
        e.own = true;
        e.uploadId = a.uploadId;
        e.variant = a.variant;
        e.description = a.description;
        e.purpose = a.purpose;
        e.nineSlice = a.nineSlice;
        if (a.dpi)
        {
            e.dpi = a.dpi;
            e.dpiFrom = String("item");
        }
        return e;
    }

    // Unique case-insensitively among taken: name, name_2, name_3...
    static String UniqueName(const String& name, Vector<String>& taken)
    {
        String n = name;
        for (int i = 2; taken.Contains(JsToLower(n)); i++)
            n = name + "_" + JsNum(i);
        taken.Add(JsToLower(n));
        return n;
    }

    std::optional<UiKit> MergeKits(const Vector<const UiKit*>& kits)
    {
        Vector<const UiKit*> real = kits.FindAll([](const UiKit* k) { return k != nullptr; });
        if (real.IsEmpty())
            return std::nullopt;
        if (real.Count() == 1)
            return *real[0];

        UiKit out;
        Vector<String> names, styleNames;
        for (auto k : real)
        {
            for (auto e : k->elements)
            {
                e.name = UniqueName(e.name, names);
                out.elements.Add(e);
            }
            for (auto s : k->textStyles)
            {
                s.name = UniqueName(s.name, styleNames);
                out.textStyles.Add(s);
            }
        }

        for (auto k : real)
        {
            if (k->device)
            {
                out.device = k->device;
                break;
            }
        }

        out.name = real[0]->name;
        for (auto k : real)
        {
            if (!k->name.IsEmpty() && k->name != "kit")
            {
                out.name = k->name;
                break;
            }
        }
        return out;
    }

    String FreeKitName(const UiKit* kit, const String& want, const Vector<String>& alsoTaken /*= {}*/)
    {
        Vector<String> taken;
        if (kit)
        {
            for (auto& e : kit->elements)
                taken.Add(JsToLower(e.name));
        }
        for (auto& n : alsoTaken)
            taken.Add(JsToLower(n));

        Json wanted(want);
        String base = CleanKitName(&wanted, "graphic");
        String n = base;
        for (int i = 2; taken.Contains(JsToLower(n)); i++)
            n = base + "_" + JsNum(i);
        return n;
    }

    Vector<String> ConfigUploadIds(const Json& config)
    {
        Vector<String> out;
        auto add = [&](const Json* v) {
            if (!v || !v->IsString())
                return;
            String id = JsTrim(v->Str());
            if (!id.IsEmpty() && !id.Contains("/") && !id.Contains("\\") && !out.Contains(id))
                out.Add(id);
        };
        if (!config.IsObject())
            return out;

        add(config.Get("uploadId"));
        const Json* items = config.Get("items");
        if (items && items->IsObject())
        {
            for (auto& m : items->Members())
                add(JsGet(JsGet(m.second, "prepared"), "uploadId"));
        }
        for (const char* key : { "steps", "uiRedo" })
        {
            const Json* steps = config.Get(key);
            if (!steps || !steps->IsArray())
                continue;
            for (auto& s : steps->Items())
            {
                const Json* assets = JsGet(s, "assets");
                if (!assets || !assets->IsArray())
                    continue;
                for (auto& a : assets->Items())
                    add(JsGet(a, "uploadId"));
            }
        }
        return out;
    }

    bool IsLayoutFixedOutput(const String& name)
    {
        return name == "image" || name == "layout";
    }

    Vector<LayoutOwnPort> LayoutOwnPorts(const Json& config)
    {
        auto assets = OwnAssetsOf(StepsOf(config));
        Vector<String> taken = { "image", "layout" };
        for (auto& a : assets)
            taken.Add(JsToLower(a.name));

        Vector<LayoutOwnPort> out;
        for (auto& a : assets)
        {
            String name = a.name;
            for (int i = 2; IsLayoutFixedOutput(JsToLower(name)); i++)
            {
                String next = a.name + "_" + JsNum(i);
                if (!taken.Contains(JsToLower(next)))
                {
                    name = next;
                    taken.Add(JsToLower(next));
                }
            }
            out.Add(LayoutOwnPort{ name, a });
        }
        return out;
    }

    String LayoutOwnPortId(const String& name)
    {
        std::string out = "own-";
        for (char32_t c : Utf8ToCodePoints(JsToLower(name)))
        {
            bool keep = (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_' || c == '-';
            out += keep ? (char)c : '_';
            if (c > 0xFFFF)
                out += '_';
        }
        return String(out);
    }

    Vector<LayoutOutputPort> LayoutOwnOutputs(const Json& config, const Vector<Pair<String, String>>& had)
    {
        Vector<LayoutOutputPort> out;
        for (auto& p : LayoutOwnPorts(config))
        {
            String id;
            for (auto& h : had)
            {
                if (JsToLower(h.first) == JsToLower(p.name))
                {
                    id = h.second;
                    break;
                }
            }
            out.Add(LayoutOutputPort{ id.IsEmpty() ? LayoutOwnPortId(p.name) : id, p.name });
        }
        return out;
    }

    std::optional<LayoutOwnPort> LayoutOwnPortByName(const Json& config, const String& name)
    {
        if (IsLayoutFixedOutput(name))
            return std::nullopt;
        for (auto& p : LayoutOwnPorts(config))
        {
            if (JsToLower(p.name) == JsToLower(name))
                return p;
        }
        return std::nullopt;
    }

    String OwnGraphicSig(const LayoutOwnPort& own)
    {
        return "uiLayout-own:" + own.asset.uploadId + ":" + (own.asset.dpi ? JsNum(*own.asset.dpi) : String());
    }
}
