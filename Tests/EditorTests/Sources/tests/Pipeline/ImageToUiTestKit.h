#pragma once

#include <cmath>
#include <regex>
#include <string>

#include "o2/Utils/Threading/Atomic.h"
#include "o2/Utils/Threading/Thread.h"
#include "o2Editor/Pipeline/PipelineUtils.h"
#include "o2Editor/Pipeline/UiLayout/UiLayoutAgent.h"
#include "o2Editor/Pipeline/UiLayout/UiLayoutAi.h"
#include "o2Editor/Pipeline/UiLayout/UiLayoutJson.h"
#include "o2Editor/Pipeline/UiLayout/UiLayoutPixels.h"

// AI image to UI with the models stubbed, AssetsLine's backend/test/imageToUi.test.ts: a tiny mockup (a blue panel holding two
// lettered tiles, a red button), the analysis from fixed JSON, every cut answered with its own crop whose border colour became
// the key colour of the prompt, and the agent's turns scripted to drive the real skill to complete: true - write a structure
// from the inventory, check, review every legend path, finalize
namespace ImageToUiTestKit
{
    using namespace o2;
    using namespace Editor;
    using namespace Editor::UiLayout;

    constexpr int W = 240, H = 320;

    // Runs a process to its end, blocking: its exit code and stdout; -1 when it cannot start or takes over a minute
    inline int RunToEnd(const String& exe, const Vector<String>& args, String& out, const String& input = "")
    {
        AgentProcess process;
        String error;
        if (!process.Start(exe, args, "", error))
            return -1;

        if (!input.IsEmpty())
            process.Write(input);
        int code = -1;
        String line;
        for (int i = 0; i < 6000 && !(process.Poll(code) && process.IsOutputClosed()); i++)
        {
            process.Pump();
            while (process.PopLine(line))
                out += line + "\n";
            Thread::SleepForMilliseconds(10);
        }
        process.Pump();
        while (process.PopLine(line))
            out += line + "\n";
        return process.Poll(code) ? code : -1;
    }

    // The first Python that imports Pillow and numpy, as the node looks for it; empty when there is none
    inline String PythonWithDependencies()
    {
        for (auto& python : PythonCandidates(""))
        {
            String out;
            if (RunToEnd(python, { "-c", "import numpy, PIL" }, out) == 0)
                return python;
        }
        return "";
    }

    inline void RoundedRect(UiImage& img, int x0, int y0, int w, int h, int r, const Rgb& fill)
    {
        // An SVG rect at (1, 1) of (w-2, h-2) with a 2 px #222 stroke: the outline spans the outer pixel ring and one more
        auto inside = [](double px, double py, double x, double y, double ww, double hh, double rr)
        {
            double cx = Math::Clamp(px, x + rr, x + ww - rr), cy = Math::Clamp(py, y + rr, y + hh - rr);
            return px >= x && py >= y && px <= x + ww && py <= y + hh && (px - cx)*(px - cx) + (py - cy)*(py - cy) <= rr*rr;
        };
        for (int y = 0; y < h; y++)
        {
            for (int x = 0; x < w; x++)
            {
                double px = x + 0.5, py = y + 0.5;
                bool outer = inside(px, py, 0, 0, w, h, r + 1);
                if (!outer)
                    continue;
                bool inner = inside(px, py, 2, 2, w - 4, h - 4, Math::Max(0, r - 1));
                UInt8* p = img.At(x0 + x, y0 + y);
                const Rgb& c = inner ? fill : Rgb(0x22, 0x22, 0x22);
                p[0] = (UInt8)c.r; p[1] = (UInt8)c.g; p[2] = (UInt8)c.b; p[3] = 255;
            }
        }
    }

    // A letter in a 5x7 pixel font, four times its size, dark brown
    inline void Letter(UiImage& img, int x0, int y0, const UInt8 (&columns)[5])
    {
        for (int c = 0; c < 5; c++)
        {
            for (int r = 0; r < 7; r++)
            {
                if (!(columns[c] & (1 << r)))
                    continue;
                for (int dy = 0; dy < 4; dy++)
                {
                    for (int dx = 0; dx < 4; dx++)
                    {
                        UInt8* p = img.At(x0 + c*4 + dx, y0 + r*4 + dy);
                        p[0] = 0x20; p[1] = 0x10; p[2] = 0x08; p[3] = 255;
                    }
                }
            }
        }
    }

    inline String MockupPng()
    {
        static const UInt8 a[5] = { 0x7e, 0x11, 0x11, 0x11, 0x7e };
        static const UInt8 b[5] = { 0x7f, 0x49, 0x49, 0x49, 0x36 };
        UiImage img = UiImage::Blank(W, H, 240, 230, 210, 255);
        RoundedRect(img, 20, 20, 200, 110, 12, Rgb(0x3A, 0x6F, 0xD8));
        RoundedRect(img, 40, 45, 60, 60, 8, Rgb(0xF2, 0xE2, 0xC4));
        RoundedRect(img, 140, 45, 60, 60, 8, Rgb(0xF2, 0xE2, 0xC4));
        Letter(img, 60, 61, a);
        Letter(img, 160, 61, b);
        RoundedRect(img, 60, 220, 120, 50, 10, Rgb(0xD8, 0x3A, 0x3A));
        return img.Png();
    }

    inline Json Box2d(double x, double y, double w, double h)
    {
        Json box = Json::Array();
        box.Add(JsRound(y/H*1000));
        box.Add(JsRound(x/W*1000));
        box.Add(JsRound((y + h)/H*1000));
        box.Add(JsRound((x + w)/W*1000));
        return box;
    }

    inline String Analysis()
    {
        Json doc = Json::Object();
        Json background = Json::Object();
        background["kind"] = "color";
        background["color"] = "#F0E6D2";
        background["describe"] = "flat beige";
        doc["background"] = background;
        Json looks = Json::Object();
        looks["panel_blue"] = "a blue rounded panel";
        looks["tile"] = "a cream square tile";
        looks["button_red"] = "a red rounded button";
        doc["looks"] = looks;

        Json els = Json::Array();
        auto el = [&](const char* id, const char* parent, const char* kind, Json box, const char* look)
        {
            Json e = Json::Object();
            e["id"] = id;
            e["parent"] = parent;
            e["kind"] = kind;
            e["box_2d"] = box;
            if (look)
                e["look"] = look;
            return e;
        };
        els.Add(el("panel", "", "panel", Box2d(20, 20, 200, 110), "panel_blue"));
        Json grid = el("tiles", "panel", "grid", Box2d(40, 45, 160, 60), nullptr);
        grid["rows"] = 1;
        grid["cols"] = 2;
        Json cells = Json::Array();
        for (auto letter : { "A", "B" })
        {
            Json cell = Json::Object();
            cell["look"] = "tile";
            Json texts = Json::Array();
            texts.Add(letter);
            cell["texts"] = texts;
            cells.Add(cell);
        }
        grid["cells"] = cells;
        els.Add(grid);
        els.Add(el("play", "", "button", Box2d(60, 220, 120, 50), "button_red"));
        doc["elements"] = els;
        return doc.Dump();
    }

    // The image model: the picture it was sent, its border colour turned into the key colour the prompt names
    inline bool CutAnswer(const AiReplayCall& call, String& data)
    {
        if (call.images.IsEmpty())
            return false;

        UiImage img;
        if (!UiImage::Decode(call.images[0], img))
            return false;

        std::smatch m;
        std::string prompt = call.prompt.Data();
        Rgb key(0, 255, 0);
        if (std::regex_search(prompt, m, std::regex("\\((#[0-9A-F]{6})\\) background")))
            ParseHex(String(m[1].str().c_str()), key);

        const UInt8* bg = img.At(0, 0);
        int br = bg[0], bgG = bg[1], bb = bg[2];
        for (int y = 0; y < img.h; y++)
        {
            for (int x = 0; x < img.w; x++)
            {
                UInt8* p = img.At(x, y);
                if (std::abs(p[0] - br) + std::abs(p[1] - bgG) + std::abs(p[2] - bb) < 40)
                {
                    p[0] = (UInt8)key.r; p[1] = (UInt8)key.g; p[2] = (UInt8)key.b;
                }
            }
        }
        data = img.Png();
        return true;
    }

    inline String Act(const Json& action, const String* content = nullptr)
    {
        String text = "Next.\n```action\n" + action.Dump() + "\n```";
        if (content)
            text += "\n```content\n" + *content + "\n```";
        return text;
    }

    inline Json ActionOf(const char* name)
    {
        Json action = Json::Object();
        action["action"] = name;
        return action;
    }

    // The agent, scripted: turn 1 writes a structure from the real inventory, 2 checks, 3 reviews every legend path (citing
    // risk_review.png for the high-risk ones) under the review binding the check printed, 4 finalizes
    inline String ScriptedTurn(const String& prompt, const String& taskDir)
    {
        std::smatch m;
        std::string text = prompt.Data();
        int turn = std::regex_search(text, m, std::regex("# Your turn (\\d+) of")) ? std::stoi(m[1].str()) : 0;
        auto read = [&](const String& file)
        {
            Json json;
            Json::Parse(PipelineUtils::ReadFileBytes(taskDir + file), json);
            return json;
        };

        if (turn == 1)
        {
            Json inventory = read("assets/assets_inventory.json");
            auto sizeOf = [&](const String& name)
            {
                Json size = Json::Object();
                if (const Json* assets = inventory.Get("assets"))
                {
                    for (auto& a : assets->Items())
                    {
                        if (a.Get("path") && a.Get("path")->Str() == name)
                        {
                            size["width"] = a.Get("width")->Num();
                            size["height"] = a.Get("height")->Num();
                        }
                    }
                }
                return size;
            };
            auto anchor = []()
            {
                Json a = Json::Object();
                a["horizontal"] = "center";
                a["vertical"] = "middle";
                return a;
            };
            auto position = [](double x, double y)
            {
                Json p = Json::Object();
                p["x"] = x;
                p["y"] = y;
                return p;
            };
            auto image = [&](const char* name, const char* asset, double x, double y)
            {
                Json e = Json::Object();
                e["type"] = "image";
                e["name"] = name;
                e["asset"] = asset;
                e["position"] = position(x, y);
                e["size"] = sizeOf(asset);
                e["anchor"] = anchor();
                return e;
            };
            auto label = [&](const char* name, const char* value, double x, double y)
            {
                Json e = Json::Object();
                e["type"] = "text";
                e["name"] = name;
                e["text"] = value;
                e["position"] = position(x, y);
                Json size = Json::Object();
                size["width"] = 60;
                size["height"] = 60;
                e["size"] = size;
                e["anchor"] = anchor();
                e["fontFamily"] = "fonts/Roboto-Bold.ttf";
                e["fontSize"] = 28;
                e["color"] = "#201008";
                e["alignment"] = "center";
                e["textVAlign"] = "middle";
                return e;
            };

            Json structure = Json::Object();
            Json canvas = Json::Object();
            canvas["width"] = W;
            canvas["height"] = H;
            canvas["name"] = "Test";
            structure["canvas"] = canvas;
            Json root = Json::Object();
            root["type"] = "container";
            root["name"] = "root";
            root["position"] = position(0, 0);
            Json rootSize = Json::Object();
            rootSize["width"] = W;
            rootSize["height"] = H;
            root["size"] = rootSize;
            Json topLeft = Json::Object();
            topLeft["horizontal"] = "left";
            topLeft["vertical"] = "top";
            root["anchor"] = topLeft;
            Json children = Json::Array();
            Json backdrop = Json::Object();
            backdrop["type"] = "rect";
            backdrop["name"] = "backdrop";
            backdrop["color"] = "#F0E6D2";
            backdrop["position"] = position(0, 0);
            backdrop["size"] = rootSize;
            backdrop["anchor"] = anchor();
            children.Add(backdrop);
            children.Add(image("panel", "panel_blue.png", 20, 20));
            children.Add(image("tile_a", "icon_tile.png", 40, 45));
            children.Add(image("tile_b", "icon_tile.png", 140, 45));
            children.Add(label("a", "A", 40, 45));
            children.Add(label("b", "B", 140, 45));
            children.Add(image("play", "button_red.png", 60, 220));
            root["children"] = children;
            structure["root"] = root;
            structure["metadata"] = Json::Object();

            Json action = ActionOf("write");
            action["path"] = "ui_structure.json";
            String content = structure.Dump(2);
            return Act(action, &content);
        }

        if (turn == 2)
            return Act(ActionOf("check"));

        if (turn == 3)
        {
            if (!std::regex_search(text, m, std::regex("<!-- image-to-ui-review-binding:[^>]*-->")))
            {
                Json stop = ActionOf("stop");
                size_t at = text.find("# Result of your last action");
                stop["reason"] = String(("check did not pass: " + text.substr(at == std::string::npos ? 0 : at, 3000)).c_str());
                return Act(stop);
            }

            String binding = m[0].str().c_str();
            Json legend = read("all_elements_legend.json");
            Json risk = read("review_risk.json");
            Vector<String> high;
            if (const Json* items = risk.Get("items"))
            {
                for (auto& item : items->Items())
                {
                    if (item.Get("risk_level") && item.Get("risk_level")->Str() == "high")
                        high.Add(item.Get("path")->Str());
                }
            }

            String review = binding + "\n\n| Path | Status | Issue | Recheck |\n| --- | --- | --- | --- |\n";
            if (const Json* targets = legend.Get("targets"))
            {
                for (auto& target : targets->Items())
                {
                    String path = target.Get("path")->Str();
                    review += "| `" + path + "` | aligned | matches | " +
                        (high.Contains(path) ? String("`risk_review.png`") : String("`all_elements.png`, `comparison.png`")) + " |\n";
                }
            }

            Json action = ActionOf("write");
            action["path"] = "alignment_review.md";
            return Act(action, &review);
        }

        if (turn == 4)
            return Act(ActionOf("finalize"));

        Json stop = ActionOf("stop");
        stop["reason"] = "script exhausted";
        return Act(stop);
    }

    // The agent in a round of edits, scripted (AssetsLine's scriptedEditor): rename the button the note points at, check, carry
    // the rename and the new binding into the review with two exact edits, finalize
    inline String ScriptedEditTurn(const String& prompt, const String& taskDir)
    {
        std::smatch m;
        std::string text = prompt.Data();
        int turn = std::regex_search(text, m, std::regex("# Your turn (\\d+) of")) ? std::stoi(m[1].str()) : 0;
        if (turn == 1)
        {
            Json structure;
            Json::Parse(PipelineUtils::ReadFileBytes(taskDir + "ui_structure.json"), structure);
            int index = -1;
            if (const Json* children = structure.Get("root") ? structure.Get("root")->Get("children") : nullptr)
            {
                for (int i = 0; i < children->Count() && index < 0; i++)
                {
                    if (children->At(i)->Get("name") && children->At(i)->Get("name")->Str() == "play")
                        index = i;
                }
            }
            Json op = Json::Object();
            op["op"] = "replace";
            op["path"] = "/root/children/" + (String)index + "/name";
            op["value"] = "start";
            Json ops = Json::Array();
            ops.Add(op);
            Json action = ActionOf("patch");
            action["ops"] = ops;
            return Act(action);
        }

        if (turn == 2)
            return Act(ActionOf("check"));

        if (turn == 3)
        {
            Json action = ActionOf("edit");
            action["path"] = "alignment_review.md";
            action["old"] = "`root/play`";
            action["new"] = "`root/start`";
            return Act(action);
        }

        if (turn == 4)
        {
            std::string review = PipelineUtils::ReadFileBytes(taskDir + "alignment_review.md").Data();
            std::smatch old, latest;
            std::regex_search(review, old, std::regex("<!-- image-to-ui-review-binding:[^>]*-->"));
            std::regex_search(text, latest, std::regex("# Latest review binding[^\\n]*\\n\\n(<!--[^>]*-->)"));
            Json action = ActionOf("edit");
            action["path"] = "alignment_review.md";
            action["old"] = String(old.empty() ? "" : old[0].str().c_str());
            action["new"] = String(latest.size() < 2 ? "" : latest[1].str().c_str());
            return Act(action);
        }

        if (turn == 5)
            return Act(ActionOf("finalize"));

        Json stop = ActionOf("stop");
        stop["reason"] = "script exhausted";
        return Act(stop);
    }

    // Answers every model call of a run while it lives; counts them. The turns of a round of edits (their task holds the
    // user's request) go to the scripted editor; failSecondRound makes every turn after the first of round 2 fail, as a
    // provider out of credits does
    struct StubModels
    {
        String      taskDir; // <work>/task/ of the node, where the scripted agent reads the skill's files
        Atomic<int> analysis{ 0 };
        Atomic<int> agent{ 0 };
        Atomic<int> cuts{ 0 };
        Atomic<int> other{ 0 };
        Atomic<int> failSecondRound{ 0 };

        explicit StubModels(const String& task): taskDir(task)
        {
            SetAiReplay([this](const AiReplayCall& call, String& data)
            {
                if (call.tag == "analysis" && call.ext == ".txt")
                {
                    analysis.FetchAdd(1);
                    data = Analysis();
                    return true;
                }
                if (call.tag.StartsWith("agent ") && call.ext == ".txt")
                {
                    agent.FetchAdd(1);
                    if (!call.prompt.Contains("# Changes the user asked for"))
                    {
                        data = ScriptedTurn(call.prompt, taskDir);
                        return true;
                    }

                    std::smatch m;
                    std::string prompt = call.prompt.Data();
                    if (failSecondRound.Load() && call.prompt.Contains("round 2 of edits") &&
                        std::regex_search(prompt, m, std::regex("# Your turn ([2-9]|\\d\\d) of")))
                    {
                        return false;
                    }
                    data = ScriptedEditTurn(call.prompt, taskDir);
                    return true;
                }
                if (call.ext == ".png" && call.tag.StartsWith("cut "))
                {
                    cuts.FetchAdd(1);
                    return CutAnswer(call, data);
                }
                other.FetchAdd(1);
                return false;
            });
        }

        ~StubModels() { SetAiReplay({}); }
    };
}
