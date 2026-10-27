#include "o2Editor/stdafx.h"
#include <gtest/gtest.h>
#include <set>

#include "Network/NetworkTestHelpers.h"
#include "o2/Utils/FileSystem/FileSystem.h"
#include "o2Editor/Pipeline/PipelineUtils.h"
#include "o2Editor/Pipeline/UiLayout/UiLayoutMeasure.h"
#include "o2Editor/Pipeline/UiLayout/UiLayoutParse.h"
#include "o2Editor/Pipeline/UiLayout/UiLayoutPrompts.h"
#include "o2Editor/Pipeline/UiLayout/UiLayoutRunSteps.h"

using namespace o2;
using namespace Editor;
using namespace Editor::UiLayout;

namespace
{
    String FixturePath()
    {
        if (const char* env = getenv("O2_IMAGE_TO_UI_FIXTURE"))
            return UiLayout::HasSuffix(String(env), "/") ? String(env) : String(env) + "/";

        for (auto& candidate : { "../../Work/ImageToUiFixture/", "../../../../Work/ImageToUiFixture/", "../../../Work/ImageToUiFixture/" })
        {
            if (o2FileSystem.IsFileExist(String(candidate) + "design.png"))
                return candidate;
        }
        return "";
    }

    String TagFile(const String& tag)
    {
        String out;
        bool gap = false;
        for (int i = 0; i < tag.Length(); i++)
        {
            char c = tag[i];
            bool keep = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_' || c == '-';
            if (keep)
            {
                if (gap)
                    out += '_';
                out += c;
                gap = false;
            }
            else
                gap = true;
        }
        return gap ? out + "_" : out;
    }

    struct ReplayGuard
    {
        std::set<std::string> asked;
        std::set<std::string> missing;

        explicit ReplayGuard(const String& fixture)
        {
            SetAiReplay([this, fixture](const AiReplayCall& call, String& data)
            {
                String name = TagFile(call.tag) + call.ext;
                String path = fixture + "answers/" + name;
                asked.insert(name.Data());
                if (!o2FileSystem.IsFileExist(path))
                {
                    missing.insert(name.Data());
                    return false;
                }
                data = PipelineUtils::ReadFileBytes(path);
                return true;
            });
        }

        ~ReplayGuard() { SetAiReplay({}); }
    };

    // The node's own part of a run up to the cuts: texts measured by their ink, a plan per look, every look cut
    Coroutine<void> MeasureAndCutAll(Ref<RunState> st, Vector<LookPlan>* plans, bool* done)
    {
        for (auto& e : st->doc.els)
        {
            if (e->kind == "text" && !e->slot)
                MeasureText(st->design, *e);
        }
        for (auto& cell : st->doc.els)
        {
            if (!cell->cell)
                continue;
            Vector<Ref<UiEl>> texts;
            for (auto& c : st->doc.ChildrenOf(cell->id))
            {
                if (c->kind == "text")
                    texts.Add(c);
            }
            MeasureCellTexts(st->design, *cell, texts);
        }

        *plans = PlanLooks(st->doc);
        co_await CutAll(st, &st->doc, plans, &st->sprites);
        *done = true;
    }
}

// Headless: the node's part on a recorded web run's answers (an earlier web run, which also cut colour variants): every
// look the analysis names is cut from its recording, none falls back to a plain crop, and the recorded plate is clean
TEST(ImageToUiFixture, CutsTheLooksTheWebRunCut)
{
    String fixture = FixturePath();
    if (fixture.IsEmpty())
        GTEST_SKIP() << "no image-to-UI fixture: set O2_IMAGE_TO_UI_FIXTURE";

    ReplayGuard replay(fixture);
    auto st = mmake<RunState>();
    st->ai = mmake<AiCtx>();
    st->ai->exec = mmake<PipelineExecContext>();
    st->ai->seed = 12345;
    st->cfg.sprites = "auto";

    UiImage decoded;
    ASSERT_TRUE(UiImage::Decode(PipelineUtils::ReadFileBytes(fixture + "design.png"), decoded));
    st->design = decoded.Flatten(255, 255, 255);
    st->designPng = st->design.Png();
    st->trace = mmake<Trace>("", st->design.w, st->design.h);
    ASSERT_TRUE(ParseAnalysis(PipelineUtils::ReadFileBytes(fixture + "answers/analysis.txt"), st->design.w, st->design.h, st->doc));

    bool done = false;
    Vector<LookPlan> plans;
    auto coroutine = MeasureAndCutAll(st, &plans, &done);
    coroutine.Start(JobThread::Main);
    ASSERT_TRUE(NetPumpUntil([&] { return done; }, 600.0f));

    std::set<std::string> recorded;
    for (auto& file : o2FileSystem.GetFolderInfo(fixture + "answers").files)
    {
        String name = o2FileSystem.GetPathWithoutDirectories(file.path);
        if (name.StartsWith("cut_"))
            recorded.insert(name);
    }

    for (auto& m : replay.missing)
        ADD_FAILURE() << "cut without a recording: " << m;

    // What this port measured and cut, for comparing with the web run's expected/ files
    if (getenv("O2_IMAGE_TO_UI_DUMP"))
    {
        PipelineUtils::WriteFileBytes(fixture + "o2-headless/trace.json", st->trace->GetFile().Dump(1));
        for (auto& entry : st->sprites)
        {
            PipelineUtils::WriteFileBytes(fixture + "o2-headless/" + entry.first + ".png", entry.second->img.Png());
            for (int i = 0; i < entry.second->steps.Count(); i++)
                PipelineUtils::WriteFileBytes(fixture + "o2-headless/" + entry.first + "_" + (String)i + ".png", entry.second->steps[i].img.Png());
        }
    }

    std::set<std::string> cuts;
    for (auto& a : replay.asked)
    {
        if (String(a).StartsWith("cut_"))
            cuts.insert(a);
    }
    EXPECT_EQ((int)cuts.size(), plans.Count()) << "a cut per look";
    EXPECT_GE(plans.Count(), 20);
    for (auto& c : cuts)
        EXPECT_TRUE(recorded.count(c)) << "a look the web run did not cut: " << c;

    for (auto& entry : st->sprites)
    {
        EXPECT_FALSE(entry.second->note.StartsWith("plain")) << entry.first.Data() << ": " << entry.second->note.Data();
        EXPECT_GT(entry.second->img.w, 4) << entry.first.Data();
    }

    UiImage plate;
    ASSERT_TRUE(UiImage::Decode(PipelineUtils::ReadFileBytes(fixture + "answers/background.png"), plate));
    plate = plate.Resize(st->design.w, st->design.h).Flatten(255, 255, 255);
    EXPECT_EQ(PlateLeftovers(plate, st->design, st->doc, st->sprites).Count(), 0) << "the web run's plate is clean";
    EXPECT_GT(PlateLeftovers(st->design, st->design, st->doc, st->sprites).Count(), 100) << "the design itself repeats every text";
}

// A glyph inside a box goes when the box is filled from its border
TEST(ImageToUiFixture, HarmonicFillFromTheBorderRemovesAGlyph)
{
    UiImage img(20, 20);
    for (int y = 0; y < 20; y++)
    {
        for (int x = 0; x < 20; x++)
        {
            UInt8* p = img.At(x, y);
            bool glyph = x >= 8 && x <= 11 && y >= 5 && y <= 14;
            p[0] = glyph ? 20 : 200; p[1] = glyph ? 20 : 180; p[2] = glyph ? 20 : 160; p[3] = 255;
        }
    }

    HarmonicFill(img, 4, 2, 15, 17, true);
    for (int y = 0; y < 20; y++)
    {
        for (int x = 0; x < 20; x++)
            ASSERT_EQ(img.At(x, y)[0], 200) << x << "," << y;
    }
}
