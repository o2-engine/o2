#pragma once

#include <optional>

#include "o2Editor/Windows/PipelineWindow/UiLayout/UiWorkspaceModel.h"

// ------------------------------------------------------------------------------------------------
// The AI UI layout's screen and units without drawing anything (port of AssetsLine uiLayout/RefCompare.tsx's
// settings, uiLayoutState.ts's units and preview words, StylePicker.tsx's samples): comparing the layout with
// its reference picture, the layout's device (the kit's) and a layout still in older units, the words of the
// screen picker and the text style previews. The card and the workspace read these
// ------------------------------------------------------------------------------------------------
namespace Editor::UiDoc::Ws
{
    // --- comparing with the reference ---------------------------------------------------------

    // -----------------------------------------------------------------------------------------
    // How the layout is compared with its reference picture (UI-only keys of the node): on, the mode
    // (alpha: the reference over the screen see-through; split: the reference left of a divider, the
    // layout right of it; diff: the per-pixel difference), the reference's or the difference's opacity
    // and where the divider stands (0..1 of the screen's width)
    // -----------------------------------------------------------------------------------------
    struct UiRefSetting
    {
        bool   on = false;
        String mode = "alpha"; // alpha | split | diff
        double alpha = 0;
        double split = 0.5;
    };

    // The modes in the order the controls show them: alpha, split, diff
    const Vector<String>& RefModes();

    // The node's compare settings (uiRefOn, uiRefMode, uiRefOverlay, uiRefSplit); never on without a reference. A pipeline
    // saved before uiRefOn had the opacity alone: above zero = on
    UiRefSetting RefSettingOf(const Json& config, bool hasReference);

    // What the stage's switch writes: off, or on with an opacity one can see through
    Vector<Pair<String, Json>> RefTogglePatch(const UiRefSetting& setting);

    // What a mode picked writes; withOff: the card's menu, where "off" is one of the choices and a mode switches it on
    Vector<Pair<String, Json>> RefModePatch(const UiRefSetting& setting, const String& mode, bool withOff);

    // What the amount slider writes: the divider's place (split), else the opacity (rounded to 0.05; switches it on)
    Vector<Pair<String, Json>> RefAmountPatch(const UiRefSetting& setting, double value);

    // The amount shown by the slider: the divider's place or the opacity
    double RefAmount(const UiRefSetting& setting);

    // The label of a mode (Off, Alpha, Split, Diff)
    String RefModeLabel(const String& mode);

    // --- the layout's device and units ------------------------------------------------------------

    // The device a layout is laid out for: its kit's, else the default (iPhone 16)
    UiTarget LayoutTarget(const UiKit* kit);

    // The kind of device a target is (its icon): a known device's, a typed screen by its shape
    String TargetKind(const UiTarget& target);

    // A layout with blocks that is not in its device's pixels yet (an empty one is no concern: the agent and the first hand
    // edit put it there)
    bool InOlderUnits(const UiDoc& doc, const UiTarget& target);

    // The screen op that goes before the first hand edit of an empty layout still at another density: the device's pixels -
    // the reference's area on the device when there is one, the device's screen otherwise. None when the layout has blocks or
    // is in its device's pixels already
    std::optional<UiOp> FirstScreenOp(const UiDoc& doc, const UiTarget& target, const UiScreen* reference);

    // The one op that brings an older layout to its device's pixels (a screen op with scale and fromDpi); none when it is
    // not in older units. scale: how much every pixel value grows
    std::optional<UiOp> BringOp(const UiDoc& doc, const UiTarget& target, double* scale = nullptr);

    // The reference picture (w x h, carrying its density or none) as the screen the layout rebuilds: for a layout in its
    // device's pixels, or an empty one, the picture's area on the device at the device's density; a layout still in older
    // units keeps the picture's own pixels and density (the base one when it carries none)
    UiScreen ReferenceScreenOf(const UiDoc& doc, const UiTarget& target, double pictureW, double pictureH,
                               std::optional<double> pictureDpi);

    // The notice of a layout in older units: "This layout is in older units (1080×1920 at 480 dpi). Bring it to ..."
    String OlderText(const UiDoc& doc, const UiTarget& target);

    // Its short form for the title bar: "Older units · 1080×1920 at 480 dpi"
    String OlderShort(const UiDoc& doc);

    // What the button says it does: "One step: every size goes to the pixels of iPhone 16 (...)..."
    String BringTitle(const UiTarget& target);

    // The label of the step that brings a layout over: "Units → iPhone 16"
    String BringLabel(const UiTarget& target);

    // --- the screen picker's words ---------------------------------------------------------------

    // "1179×2556"
    String SizeText(double w, double h);

    // What a screen adds to the design, layout pixels: the canvas is the design plus this
    UiSize FrameExtra(const UiDoc& doc, const UiFrame& frame);

    // "+460 px height" / "+120 px width": what the shown screen adds to the design ("" when the canvas is the design)
    String AddedText(const UiDoc& doc, const UiPreview& preview);

    // The canvas a screen gives the layout, in its pixels ("1179×2556"): the design fitted into the screen at the layout's DPI
    String CanvasText(const UiDoc& doc, double screenW, double screenH);

    // The icon kind of the shown screen: the kit's device by its kind, the others by theirs
    String PreviewIconKind(const UiPreview& preview, const UiTarget& target);

    // --- text style previews -------------------------------------------------------------------

    // The words a style's preview says: the block's own text when it is short enough to read in a row (22 characters),
    // else the style's sample
    String StyleSampleText(const UiNode& n, const TextStyle& style);

    // The sample of a style by its role: "1 250", "Play", "Forest Quest", "Tap to continue", "Hello, world"
    String DefaultStyleSample(const TextStyle& style);

    // A screen just large enough for the words in the style, its effects included, on the Text style card's dark paper:
    // the lettering fills a preview of it
    UiDoc TightStyleDoc(const TextStyle& style, const String& text);

    // The style "auto" comes to for a block: the kit's style for its role (a button's own label is a button label)
    TextStyle AutoStyleOf(const UiNode& n, const UiKit* kit);
}
