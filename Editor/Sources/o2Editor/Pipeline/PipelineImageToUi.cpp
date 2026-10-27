#include "o2Editor/stdafx.h"
#include "PipelineImageToUi.h"

#include <cmath>

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
