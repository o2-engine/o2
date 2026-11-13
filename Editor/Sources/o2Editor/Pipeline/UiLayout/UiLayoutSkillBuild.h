#pragma once

#include "o2Editor/Pipeline/UiDoc/UiDocFromStructure.h"
#include "o2Editor/Pipeline/UiLayout/UiLayoutAgent.h"
#include "o2Editor/Pipeline/UiLayout/UiLayoutRunLog.h"
#include "o2Editor/Pipeline/UiLayout/UiLayoutRunSteps.h"

// ------------------------------------------------------------------------------------------------
// The layout of a screen rebuilt from its reference is made by XuToWei's Image-To-UI skill, run unchanged
// with the vision model as its agent (o2/Tools/ImageToUi/agent/agent.py, RunAgent) - the AI image to UI
// node's algorithm: it measures regions of the design on a grid, writes ui_structure.json, renders it and
// compares the reconstruction with the design pixel by pixel, fixes, crops every risky element against the
// design, reviews and finalizes only when its own gates pass (port of AssetsLine
// backend/src/uiLayout/skillBuild.ts).
//
// What the layout node adds is the UI kit: the skill's sprite folder IS the node's kit - every kit graphic at
// the design's density with its nine-slice borders, plus the few graphics cut out of the reference for what
// the kit lacks. And it does not start from nothing: the markup of the reference in terms of the kit is
// written as the first ui_structure.json (agent.py --seed)
// ------------------------------------------------------------------------------------------------
namespace Editor::UiLayout
{
    // The skill's inputs for a build
    struct SkillInputs
    {
        WorkDirs            dirs;
        Map<String, String> assetNames; // Sprite file (lower case) -> the kit graphic's name
        Vector<String>      fonts;      // Font files in fonts/
        String              seed;       // The first ui_structure.json, in the skill's sprite paths
        Vector<String>      kitFiles;
        Vector<String>      ownFiles;
    };

    // Writes a kit graphic into the sprite folder at the design's density, its borders as Unity's sprite border (.png.meta);
    // returns its file name, empty when it has no picture
    String WriteKitSprite(const String& dir, const ::Editor::UiDoc::UiKitElement& el, double designDpi);

    // The skill's folders for a build: the design, the sprite folder (the whole kit, then the node's own graphics), the fonts,
    // a fresh task folder - and the draft as the first ui_structure.json
    SkillInputs LayoutSkillInputs(const String& workDir, const String& designFlatPng, double designDpi, const ::Editor::UiDoc::UiKit* kit,
                                  const Vector<::Editor::UiDoc::TextStyle>& styles, const ::Editor::UiDoc::UiDoc& draft);

    // The draft as the skill's first ui_structure.json: its own export, with the sprite borders at the design's density and
    // fonts by the paths the skill is given
    String SeedStructure(const ::Editor::UiDoc::UiDoc& draft, const ::Editor::UiDoc::UiKit* kit, double designDpi);

    // What the agent is told about the sprite folder and the draft
    String SkillNote(const SkillInputs& inputs, const ::Editor::UiDoc::UiKit* kit, const Vector<::Editor::UiDoc::TextStyle>& styles);

    // --------------------------------------------------------------------------------------------
    // The skill's steps (its trace) as acts of the run, as they are written: each action of the agent -
    // view, measure, write, patch, check, target, finalize - is a row with what it decided, the script's
    // report and its pictures; a step that rendered the layout gets the pixel difference and the likeness
    // --------------------------------------------------------------------------------------------
    class SkillActs
    {
    public:
        std::optional<double> score;     // How alike the latest reconstruction and the design are (0..1)
        std::optional<double> scoreFrom; // The first one measured

    public:
        SkillActs(const String& traceDir, const String& taskDir, const Ref<RunLog>& log, const UiImage& design, double w, double h);

        // Reads the trace and adds the steps not seen yet (at most every 1.5 s unless final)
        void Sync(bool final = false);

    private:
        String         mTraceDir;
        String         mTaskDir;
        Ref<RunLog>    mLog;
        UiImage        mDesign;
        double         mW = 0, mH = 0;
        Vector<String> mSeen;
        double         mLastSync = 0;
    };

    // What a build is given
    struct SkillBuildInput
    {
        Ref<AiCtx>                             ai;
        SkillInputs                            inputs;
        const ::Editor::UiDoc::UiKit*          kit = nullptr;
        Vector<::Editor::UiDoc::TextStyle>     styles;
        double                                 designDpi = 480;
        int                                    maxTurns = 60;
        String                                 hint;
        String                                 name;
        String                                 workDir;
        String                                 python;
        String                                 design; // The reference flattened (what the skill's design.png is)
        double                                 w = 0, h = 0;
        String                                 salt;   // Model answers of this run are remembered under it
        Ref<RunLog>                            log;
    };

    // What a build ended with
    struct SkillBuildResult
    {
        ::Editor::UiDoc::UiDoc             doc;
        Vector<::Editor::UiDoc::TextStyle> styles;   // Styles the converter made for labels no style fitted
        Json                               summary;  // The agent's summary
        Vector<String>                     warnings;
        bool                               ok = false; // The skill ended with a checked layout (else doc is empty)
        String                             error;
        bool                               cancelled = false; // Stopped: doc is the layout as far as it got
        std::optional<double>              score, scoreFrom;
    };

    // Runs the skill on prepared inputs and turns its result into the node's layout
    Coroutine<SkillBuildResult> RunSkillBuild(SkillBuildInput input);
}
