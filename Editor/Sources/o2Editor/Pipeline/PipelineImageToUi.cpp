#include "o2Editor/stdafx.h"
#include "PipelineImageToUi.h"

#include <cmath>

#include "o2/Utils/FileSystem/FileSystem.h"
#include "o2/Utils/Serialization/DataValue.h"
#include "o2Editor/Pipeline/PipelineUtils.h"
#include "o2Editor/Pipeline/UiLayout/UiLayoutJson.h"
#include "o2Editor/Pipeline/UiLayout/UiLayoutPixels.h"

namespace Editor::PipelineImageToUi
{
    using UiLayout::Json;

    const String type = "imageToUi";
    const String defaultModel = "gemini-3.1-pro-preview";
    const String defaultImageModel = "gemini-3.1-flash-image";
    const String defaultScreenName = "Screen";
    const String defaultAssetPath = "Generated/UI";

    const Vector<int>& TurnOptions()
    {
        static const Vector<int> options = { 20, 40, 60, 90, 120 };
        return options;
    }

    String SpriteName(const String& key)
    {
        return key == "preview" || key == "layout" ? "sprite_" + key : key;
    }

    Vector<PipelinePort> SpritePorts(const PipelineNode& node)
    {
        Vector<PipelinePort> ports;
        for (auto& port : node.outputs)
        {
            if (port.portType == PipelinePortType::Image && port.name != "preview")
                ports.Add(port);
        }
        return ports;
    }

    static double JsNumberOf(const DataValue* value, double def)
    {
        if (!value)
            return def;

        if (value->IsNumber())
            return (double)*value;

        if (value->IsString())
        {
            double n = Json(value->GetString()).ToJsNumber();
            return std::isfinite(n) ? n : def;
        }

        if (value->IsBoolean())
            return (bool)*value ? 1.0 : 0.0;

        return value->IsNull() ? 0.0 : def;
    }

    PipelineImageToUiConfig ReadConfig(const PipelineNode& node)
    {
        PipelineImageToUiConfig cfg;
        cfg.model = node.GetConfigString("model", "").Trimed(" \t\n\r");
        if (cfg.model.IsEmpty())
            cfg.model = defaultModel;

        cfg.imageModel = node.GetConfigString("imageModel", "").Trimed(" \t\n\r");
        if (cfg.imageModel.IsEmpty())
            cfg.imageModel = defaultImageModel;

        double turns = JsNumberOf(node.GetConfigValue("maxTurns"), defaultMaxTurns);
        cfg.maxTurns = (int)Math::Max(4.0, Math::Min(120.0, UiLayout::JsRound(turns)));
        cfg.sprites = node.GetConfigString("sprites", "auto") == "ai" ? "ai" : "auto";

        String background = node.GetConfigString("background", "auto");
        cfg.background = background == "image" || background == "color" || background == "none" ? background : String("auto");

        String name = node.GetConfigString("screenName", "").Trimed(" \t\n\r"), clean;
        for (int i = 0; i < name.Length(); i++)
        {
            char c = name[i];
            if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_' || c == '-')
                clean += c;
        }
        cfg.screenName = clean.IsEmpty() ? defaultScreenName : clean;
        cfg.hint = node.GetConfigString("hint", "");

        String assetPath = node.GetConfigString("assetPath", "").Trimed(" \t\n\r");
        assetPath.ReplaceAll("\\", "/");
        while (assetPath.StartsWith("/"))
            assetPath = assetPath.SubStr(1);
        while (UiLayout::HasSuffix(assetPath, "/"))
            assetPath = assetPath.SubStr(0, assetPath.Length() - 1);
        cfg.assetPath = assetPath.IsEmpty() ? defaultAssetPath : assetPath;
        return cfg;
    }

    std::optional<PipelineImageToUiSummary> SummarizeLayout(const String& json)
    {
        if (json.IsEmpty())
            return std::nullopt;

        Json s;
        if (!Json::Parse(json, s) || !s.Get("root"))
            return std::nullopt;

        PipelineImageToUiSummary summary;
        Vector<String> assets;
        Function<void(const Json&)> walk;
        walk = [&](const Json& n)
        {
            summary.elements++;
            if (n.Get("type") && n.Get("type")->Str() == "text")
                summary.texts++;
            if (n.Get("asset") && n.Get("asset")->IsString() && !assets.Contains(n.Get("asset")->Str()))
                assets.Add(n.Get("asset")->Str());
            if (auto children = n.Get("children"))
            {
                for (auto& c : children->Items())
                    walk(c);
            }
        };
        walk(*s.Get("root"));
        summary.elements--;
        summary.sprites = assets.Count();

        if (const Json* canvas = s.Get("canvas"))
        {
            summary.name = canvas->Get("name") ? canvas->Get("name")->ToJsString() : String();
            summary.width = canvas->Get("width") ? (int)canvas->Get("width")->Num() : 0;
            summary.height = canvas->Get("height") ? (int)canvas->Get("height")->Num() : 0;
        }
        return summary;
    }

    std::optional<PipelineImageToUiRun> ReadRunSummary(const String& traceJson)
    {
        Json trace;
        if (traceJson.IsEmpty() || !Json::Parse(traceJson, trace))
            return std::nullopt;

        const Json* summary = trace.Get("summary");
        if (!summary || !summary->Get("complete") || !summary->Get("complete")->IsBool())
            return std::nullopt;

        PipelineImageToUiRun run;
        run.complete = summary->Get("complete")->Bool();
        run.checks = summary->Get("check_count") ? (int)summary->Get("check_count")->Num() : 0;
        run.turns = summary->Get("turns") ? (int)summary->Get("turns")->Num() : 0;
        run.blockers = summary->Get("blockers") ? summary->Get("blockers")->Count() : 0;
        return run;
    }

    String RunLine(const PipelineImageToUiRun& run)
    {
        return run.complete ? "completed \xC2\xB7 " + (String)run.checks + " checks" : "not completed \xC2\xB7 " + (String)run.blockers + " blockers";
    }

    String SummaryLine(const PipelineImageToUiSummary& summary, const PipelineImageToUiRun* run, int sprites /*= -1*/)
    {
        String line = (String)summary.elements + " elements \xC2\xB7 " + (String)(sprites >= 0 ? sprites : summary.sprites) + " sprites";
        return run ? RunLine(*run) + " \xC2\xB7 " + line : line;
    }
}

namespace Editor::PipelineImageToUi
{
    // JavaScript's String#slice(0, units) over UTF-8: a character outside the BMP counts two units
    static String CutUtf16(const String& text, int units)
    {
        int count = 0, i = 0, n = text.Length();
        while (i < n)
        {
            UInt8 c = (UInt8)text[i];
            int len = c < 0x80 ? 1 : (c >> 5) == 0x6 ? 2 : (c >> 4) == 0xE ? 3 : (c >> 3) == 0x1E ? 4 : 1;
            int w = len == 4 ? 2 : 1;
            if (count + w > units)
                break;
            count += w;
            i = Math::Min(n, i + len);
        }
        return i >= n ? text : text.SubStr(0, i);
    }

    // String(value ?? "")
    static String JsText(const Json* value)
    {
        return !value || value->IsNull() ? String() : value->ToJsString();
    }

    static double JsNumberOrNaN(const Json* value)
    {
        return value ? value->ToJsNumber() : NAN;
    }

    static std::optional<PipelineImageToUiEditItem> EditItem(const Json& raw)
    {
        if (!raw.IsObject())
            return std::nullopt;

        PipelineImageToUiEditItem item;
        item.text = CutUtf16(JsText(raw.Get("text")).Trimed(" \t\n\r\f\v"), editTextMax);
        if (item.text.IsEmpty())
            return std::nullopt;

        const Json* a = raw.Get("at");
        if (a && a->IsObject())
        {
            double x = JsNumberOrNaN(a->Get("x")), y = JsNumberOrNaN(a->Get("y"));
            if (std::isfinite(x) && std::isfinite(y))
            {
                PipelineImageToUiEditPoint at;
                const Json* on = a->Get("on");
                at.on = on && on->IsString() && on->Str() == "design" ? "design" : "result";
                at.x = (int)Math::Max(0.0, UiLayout::JsRound(x));
                at.y = (int)Math::Max(0.0, UiLayout::JsRound(y));
                item.at = at;
            }
        }
        return item;
    }

    Vector<PipelineImageToUiEditItem> EditItems(const Json& raw)
    {
        Vector<PipelineImageToUiEditItem> items;
        if (!raw.IsArray())
            return items;

        for (auto& r : raw.Items())
        {
            if (items.Count() >= editItemsMax)
                break;
            if (auto item = EditItem(r))
                items.Add(*item);
        }
        return items;
    }

    Vector<PipelineImageToUiEditRound> Edits(const Json& raw)
    {
        Vector<PipelineImageToUiEditRound> rounds;
        if (!raw.IsArray())
            return rounds;

        for (auto& r : raw.Items())
        {
            if (!r.IsObject() && !r.IsArray())
                continue;

            auto items = EditItems(r.Get("items") ? *r.Get("items") : Json());
            if (items.IsEmpty())
                continue;

            PipelineImageToUiEditRound round;
            const Json* id = r.Get("id");
            round.id = CutUtf16(id && !id->IsNull() ? id->ToJsString() : "e" + (String)(rounds.Count() + 1), 64);
            round.items = items;
            rounds.Add(round);
        }
        return rounds;
    }

    Vector<PipelineImageToUiEditRound> Edits(const PipelineNode& node)
    {
        return Edits(ConfigJson(node, "edits"));
    }

    Vector<PipelineImageToUiEditItem> ReadDraft(const PipelineNode& node)
    {
        Vector<PipelineImageToUiEditItem> draft;
        Json raw = ConfigJson(node, "editsDraft");
        if (!raw.IsArray())
            return draft;

        for (auto& r : raw.Items())
        {
            if (!r.IsObject())
                continue;

            PipelineImageToUiEditItem item;
            item.text = JsText(r.Get("text"));
            const Json* a = r.Get("at");
            if (a && a->IsObject())
            {
                double x = JsNumberOrNaN(a->Get("x")), y = JsNumberOrNaN(a->Get("y"));
                if (std::isfinite(x) && std::isfinite(y))
                {
                    PipelineImageToUiEditPoint at;
                    at.on = a->Get("on") && a->Get("on")->Str() == "design" ? "design" : "result";
                    at.x = (int)x;
                    at.y = (int)y;
                    item.at = at;
                }
            }
            draft.Add(item);
        }
        return draft;
    }

    int EditTurns(int maxTurns)
    {
        return Math::Max(12, (int)UiLayout::JsRound(maxTurns/2.0));
    }

    Json EditItemsJson(const Vector<PipelineImageToUiEditItem>& items)
    {
        Json list = Json::Array();
        for (auto& item : items)
        {
            Json json = Json::Object();
            json["text"] = item.text;
            if (item.at)
            {
                Json at = Json::Object();
                at["on"] = item.at->on;
                at["x"] = item.at->x;
                at["y"] = item.at->y;
                json["at"] = at;
            }
            list.Add(json);
        }
        return list;
    }

    Json EditsJson(const Vector<PipelineImageToUiEditRound>& rounds)
    {
        Json list = Json::Array();
        for (auto& round : rounds)
        {
            Json json = Json::Object();
            json["id"] = round.id;
            json["items"] = EditItemsJson(round.items);
            list.Add(json);
        }
        return list;
    }

    void WriteEdits(PipelineNode& node, const Vector<PipelineImageToUiEditRound>& rounds)
    {
        SetConfigJson(node, "edits", EditsJson(rounds));
    }

    void WriteDraft(PipelineNode& node, const Vector<PipelineImageToUiEditItem>& items)
    {
        SetConfigJson(node, "editsDraft", EditItemsJson(items));
    }

    String NewEditId(Int64 epochMs)
    {
        static const char* digits = "0123456789abcdefghijklmnopqrstuvwxyz";
        UInt64 v = epochMs > 0 ? (UInt64)epochMs : 0;
        String out;
        do
        {
            out = String(digits[v % 36]) + out;
            v /= 36;
        }
        while (v > 0);
        return "e" + out;
    }

    bool ApplyEditRound(PipelineNode& node, const Vector<PipelineImageToUiEditItem>& items, const String& id)
    {
        auto clean = EditItems(EditItemsJson(items));
        if (clean.IsEmpty())
            return false;

        auto rounds = Edits(node);
        rounds.Add({ id, clean });
        WriteEdits(node, rounds);
        WriteDraft(node, {});
        node.SetConfigBool("editsView", false);
        return true;
    }

    bool UndoEditRound(PipelineNode& node)
    {
        auto rounds = Edits(node);
        if (rounds.IsEmpty())
            return false;

        rounds.RemoveAt(rounds.Count() - 1);
        WriteEdits(node, rounds);
        return true;
    }

    Vector<PipelineImageToUiPin> EditPins(const Vector<PipelineImageToUiEditItem>& draft, const Vector<PipelineImageToUiEditRound>& rounds,
                                          int viewRound)
    {
        Vector<PipelineImageToUiPin> pins;
        for (int i = 0; i < draft.Count(); i++)
        {
            if (draft[i].at)
                pins.Add({ i + 1, draft[i].at->on, draft[i].at->x, draft[i].at->y, false });
        }
        if (viewRound >= 0 && viewRound < rounds.Count())
        {
            auto& items = rounds[viewRound].items;
            for (int i = 0; i < items.Count(); i++)
            {
                if (items[i].at)
                    pins.Add({ i + 1, items[i].at->on, items[i].at->x, items[i].at->y, true });
            }
        }
        return pins;
    }

    String EditPlace(const PipelineImageToUiEditItem& item)
    {
        if (!item.at)
            return "whole screen";
        return (item.at->on == "design" ? String("Source") : String("Result")) + " \xC2\xB7 " + (String)item.at->x + ", " + (String)item.at->y;
    }

    bool PictureClick(const RectF& drawn, const Vec2I& size, const Vec2F& point, int& x, int& y)
    {
        if (drawn.Width() <= 0.0f || drawn.Height() <= 0.0f || size.x <= 0 || size.y <= 0)
            return false;

        x = (int)UiLayout::JsRound((point.x - drawn.left)/drawn.Width()*size.x);
        y = (int)UiLayout::JsRound((drawn.top - point.y)/drawn.Height()*size.y);
        return x >= 0 && y >= 0 && x < size.x && y < size.y;
    }

    bool CanApplyEdits(bool running, bool hasResult, const Vector<PipelineImageToUiEditItem>& draft)
    {
        return !running && hasResult && !EditItems(EditItemsJson(draft)).IsEmpty();
    }

    static bool SafeRoundId(const String& id)
    {
        if (id.IsEmpty() || id.Length() > 64)
            return false;
        for (int i = 0; i < id.Length(); i++)
        {
            char c = id[i];
            if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_' || c == '-'))
                return false;
        }
        return true;
    }

    String StateTracePath(const String& workDir, const Vector<PipelineImageToUiEditRound>& edits)
    {
        String work = workDir;
        while (UiLayout::HasSuffix(work, "/"))
            work = work.SubStr(0, work.Length() - 1);
        String live = work + "/trace/trace.json";
        if (edits.Count() > 200 || edits.Any([](const PipelineImageToUiEditRound& r) { return !SafeRoundId(r.id); }))
            return live;

        Json state;
        if (!Json::Parse(PipelineUtils::ReadFileBytes(work + "/rounds.json"), state) || !state.IsObject())
            return live;

        const Json* rounds = state.Get("rounds");
        int kept = rounds && rounds->IsArray() ? rounds->Count() : 0;
        if (kept < edits.Count())
            return live;
        for (int i = 0; i < edits.Count(); i++)
        {
            const Json* id = rounds->At(i)->Get("id");
            if (!id || !id->IsString() || id->Str() != edits[i].id)
                return live;
        }

        String path = work + "/rounds/" + (String)edits.Count() + "/trace.json";
        return o2FileSystem.IsFileExist(path) ? path : live;
    }

    Json ConfigJson(const PipelineNode& node, const String& key)
    {
        Json json;
        if (auto value = node.GetConfigValue(key))
            Json::Parse(PipelineUtils::CanonicalJson(*value), json);
        return json;
    }

    void SetConfigJson(PipelineNode& node, const String& key, const Json& value)
    {
        if (!node.config.IsObject())
            node.config.SetObject();

        DataDocument doc;
        if (!doc.LoadFromData(value.Dump()))
            return;

        node.RemoveConfig(key);
        node.config[key.Data()] = static_cast<const DataValue&>(doc);
    }
}
