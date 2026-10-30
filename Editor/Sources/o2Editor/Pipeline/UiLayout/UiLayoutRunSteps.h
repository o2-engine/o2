#pragma once

#include <set>
#include <string>

#include "o2Editor/Pipeline/UiLayout/UiLayoutAgent.h"
#include "o2Editor/Pipeline/UiLayout/UiLayoutRun.h"

namespace Editor::UiLayout
{
    // What is behind the UI: a colour, a sprite plate or nothing
    struct UiBackdrop
    {
        String kind = "none"; // color, image, none
        String color;         // #RRGGBB of a colour
        String key;           // Sprite key of a plate
    };

    // Everything one run works on, shared by its steps
    struct RunState: public RefCounterable
    {
        Ref<AiCtx>      ai;
        Ref<Trace>      trace;
        ImageToUiConfig cfg;

        UiImage design;    // Flattened on white
        String  designPng; // Its PNG, as the models see it

        UiDoc                      doc;
        OrderedMap<Ref<SpriteCut>> sprites;
        UiBackdrop                 background;
    };

    // The folders of a node's work folder, absolute and without a trailing slash
    struct WorkDirs
    {
        String work;
        String input;   // input/: the design and the fonts again
        String design;  // input/design.png
        String assets;  // sprites/
        String fonts;   // fonts/
        String task;    // task/: the skill's task folder
        String scratch; // agent-view/
    };

    // Makes the work folder and returns its folders
    WorkDirs MakeWorkDirs(const String& workDir);

    // What the run without edits ended with
    struct BaseRun
    {
        bool   ok = false;
        String error;
        Json   summary; // The agent's summary
        String note;    // The --sprites-note it was given; the rounds pass it again
    };

    // The run without edits: the analysis, the sprites and the backdrop, the skill's inputs laid out in a fresh work folder
    // (the kept states of earlier rounds removed), the agent from a fresh task folder. The trace is reset by the caller
    Coroutine<BaseRun> RunBase(Ref<RunState> st, WorkDirs dirs);

    // The agent's arguments every run of a work folder shares: the folders, the turns, the names, the fonts
    AgentRun BaseAgentRun(const WorkDirs& dirs, const Ref<Trace>& trace, const ImageToUiConfig& cfg, const String& note);

    // Returns the borders of a .png.meta the run wrote: "spriteBorder: {x: L, y: B, z: R, w: T}"
    std::optional<NineSlice> ParseNineSliceMeta(const String& text);

    // A sprite file of the sliced-asset folder
    struct AssetFile
    {
        String                   file; // <name>.png
        String                   look; // Look key, "background" for the plate
        std::optional<NineSlice> nine;
    };

    // The file name of a look's sprite, AssetsLine's assetFileName: the key made safe and a kind prefix unless the key says
    // it already (the skill sorts its usage contact sheets by name) - panel_ for panels and inputs, button_, bar_ for
    // progress tracks, icon_ for anything else under 10 % of the screen
    String AssetFileName(const String& key, const String& kind, double area, double screen);

    // Every asset file a ui_structure.json names, states and variants included, a leading ./ dropped, in order of appearance
    Vector<String> AssetRefs(const Json& structure);

    // What the agent is told about the sliced assets (PORTING.md --sprites-note)
    String SpritesNote(const Vector<AssetFile>& files, const UiBackdrop& background);

    // The .png.meta of nine-slice borders in Unity's order: x left, y bottom, z right, w top
    String NineSliceMeta(const NineSlice& nine);

    // Writes the sprites of the looks (and the plate as bg_backdrop.png) into the folder, nine-sliced ones with a .png.meta
    Vector<AssetFile> WriteAssets(const String& dir, const Vector<LookPlan>& plans, const OrderedMap<Ref<SpriteCut>>& sprites,
                                  double screen);

    // Cuts the looks, four at a time, into sprites; a look that cannot be cut falls back to the plain crop of its box
    Coroutine<bool> CutAll(Ref<RunState> st, UiDoc* doc, Vector<LookPlan>* plans, OrderedMap<Ref<SpriteCut>>* sprites);

    // The background: a colour or a model plate, or nothing
    Coroutine<bool> MakeBackground(Ref<RunState> st);

    // Where a background plate repeats the design: a text's ink or a small piece of artwork (where its sprite is opaque)
    // the model kept; a clean plate is flat there or shows its own texture, which does not follow the element's shapes
    Vector<Box> PlateLeftovers(const UiImage& plate, const UiImage& design, const UiDoc& doc, const OrderedMap<Ref<SpriteCut>>& sprites);

    // The analysis boxes coloured by kind
    UiImage OverlayKinds(const UiImage& design, const UiDoc& doc);
}
