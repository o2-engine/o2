#pragma once

#include <optional>

#include "o2/Utils/Serialization/DataValue.h"
#include "o2Editor/Pipeline/UiDoc/UiDocEdit.h"
#include "o2Editor/Pipeline/UiDoc/UiDocSkin.h"
#include "o2Editor/Pipeline/UiDoc/UiDocUiml.h"

// ------------------------------------------------------------------------------------------------
// What the AI UI layout workspace knows without drawing anything (port of AssetsLine
// frontend/src/nodeEditor/uiLayout/uiLayoutState.ts and the logic parts of UiWorkspace.tsx /
// DevicePicker.tsx): the view keys of a uiLayout node's config, the step queue written back into the
// config the way the web writes it (commitUser, undo / redo by action, revert), the screen after each
// step, the preview screens, the hierarchy's add / drop / rename edits and the inspector's edits. Every
// edit is a UiDoc op; the window (UiWorkspace.h) draws and drives these
// ------------------------------------------------------------------------------------------------
namespace Editor::UiDoc::Ws
{

    // --- the node's view keys -----------------------------------------------------------------

    // Line art or the kit: the node's uiView, else the kit when one is wired in
    String ViewOf(const Json& config, bool hasKit);

    // How many kit graphics have no name of their own (element_N) or no type and no description
    int KitUndescribed(const UiKit* kit);

    // The agent's model: config.agentModel, else the default
    String AgentModelOf(const Json& config);

    // The selected block (config.uiSelected) when the document has it, else ""
    String SelectedOf(const Json& config, const UiDoc& doc);

    // The open tab: agent | inspect | text | history (config.uiTab, else agent)
    String TabOf(const Json& config);

    // --- the step queue in the config -----------------------------------------------------------

    // Milliseconds since the epoch, now
    Int64 NowMs();

    // A hand action: it joins the run of hand edits since the agent's last answer; the redo list is emptied. fold = false
    // for a gesture (never folded into the action before). False when there are no ops
    bool CommitUser(DataValue& config, const Vector<UiOp>& ops, const String& label, Int64 nowMs, bool fold = true);

    // Undo: the last hand action, or the agent's last answer; false when there is nothing to undo
    bool UndoStep(DataValue& config);

    // Redo: the action or step undone last; false when there is none
    bool RedoStep(DataValue& config);

    // Back to the screen after `count` steps: the later steps go to the redo list (newest last); false when count is not
    // before the last step
    bool RevertTo(DataValue& config, int count);

    // -----------------------------------------------------------------------------------------
    // The screen before any step and after each one (docs[k] = after k steps), with the failed
    // edits of each step. Rebuilt from the first step that changed
    // -----------------------------------------------------------------------------------------
    class UiTimeline
    {
    public:
        // Reads the config's steps; steps that did not change keep their documents
        void Update(const Json& config);

        // The steps read last
        const Vector<UiStep>& GetSteps() const { return mSteps; }

        // The screen after k steps (0 = before any)
        const UiDoc& DocAfter(int k) const { return mDocs[Math::Clamp(k, 0, mDocs.Count() - 1)]; }

        // The edits of step k (1-based, as DocAfter) that failed
        const Vector<String>& ErrorsOf(int k) const { return mErrors[Math::Clamp(k, 0, mErrors.Count() - 1)]; }

        // How many documents (steps + 1)
        int Count() const { return mDocs.Count(); }

    private:
        Vector<UiStep>         mSteps;
        Vector<String>         mStepTexts; // Each step as JSON text, to find the first one that changed
        String                 mBaseText;  // What the base screen depends on (screenName)
        Vector<UiDoc>   mDocs;
        Vector<Vector<String>> mErrors;
    };

    // --- preview screens ------------------------------------------------------------------------

    // ------------------------------------------------------------------------------------------
    // The screen the layout is previewed on, as the node's config asks: the design, the reference,
    // a device or a custom screen, turned or not, met by DPI or by screen
    // ------------------------------------------------------------------------------------------
    struct UiPreviewSetting
    {
        String   device = "design";                      // design | reference | custom | a device id
        UiScreen custom = UiScreen{ 1080, 1920, UI_BASE_DPI };
        bool     landscape = false;
        String   scale = "dpi";                           // dpi | fit
    };

    // The preview setting of a node's config (uiDevice, uiDeviceCustom, uiLandscape, uiScale)
    UiPreviewSetting PreviewSettingOf(const Json& config);

    // -------------------------------------------------------------------------
    // The screen shown: what it is, its pixels and density, and the canvas solved
    // -------------------------------------------------------------------------
    struct UiPreview
    {
        String   id;       // design | reference | custom | a device id
        String   kind;     // design | reference | custom | phone | tablet | desktop
        String   name;     // The device's name ("" for the design, the reference and a custom screen)
        UiScreen screen;
        UiFrame  frame;
        bool     fallback = false; // The reference was asked for but none is connected: the design is shown
    };

    // The preview of a layout for a setting; reference = the picture the layout rebuilds (null when none)
    UiPreview PreviewOf(const UiDoc& doc, const UiPreviewSetting& setting, const UiScreen* reference);

    // Kit graphics drawn larger than their own pixels on the frame (they come out soft)
    Vector<String> SoftGraphics(const UiDoc& doc, const UiKit* kit, const UiFrame& frame);

    // The name a preview is shown under: Design, Reference, Custom or the device's name
    String PreviewName(const UiPreview& preview);

    // "1080×1920 · 480 dpi"
    String ScreenText(const UiScreen& screen);

    // --- the hierarchy --------------------------------------------------------------------------

    // The block words of the add menu, in the web's order (button, text, icon, image, panel, background, row, column, grid,
    // group, progress, toggle, slider, input, tab, badge, divider, slot)
    const Vector<String>& AddableWords();

    // A new block of the word with the id, as the add menu makes it (raw JSON in the web's key order)
    Json AddableNode(const String& word, const String& id);

    // The name of a type word in the menus: Button, Progress bar, Row...
    String TypeLabel(const String& word);

    // The type word of a block: its layout for a group with one, else its type
    String TypeWord(const UiNode& node);

    // True for blocks that never hold others (text, icon, image, divider, progress, slider, toggle, badge, input, background)
    bool IsLeaf(const UiNode& node);

    // True for blocks that can hold others: the screen, and everything but the leaves
    bool CanHold(const UiNode& node);

    // True when the block is the id or holds it
    bool Holds(const UiNode& node, const String& id);

    // Adds a block of the word into the selected block (or after a selected leaf, or into the screen): the op, its label
    // ("+ button1") and the new id
    UiOp AddBlockOp(const UiDoc& doc, const String& selected, const String& word, String& label, String& newId);

    // -------------------------------------------------------
    // Where a dragged row lands: before, after or inside a block
    // -------------------------------------------------------
    struct UiDropAt
    {
        String id;
        String zone; // before | after | inside

        bool operator==(const UiDropAt& other) const { return id == other.id && zone == other.zone; }
    };

    // The landing place of a row dragged over a row (f = how far down the row the pointer is, 0..1); "" target = below the
    // last row. False when it cannot land there (on itself or inside itself)
    bool ZoneAt(const UiDoc& doc, const String& dragId, const String& targetId, float f, bool targetFolded, UiDropAt& out);

    // The ops of a drop (the block keeps its place on the screen) and the label; false when it lands where it was
    bool DropOps(const UiDoc& doc, const UiRects& rects, const String& dragId, const UiDropAt& at, Vector<UiOp>& ops,
                 String& label);

    // A rename: of the screen ({op: screen, name}) or of a block ({op: update, id, set: {id}}); false when the name is
    // empty, the same or taken. The label is "old → new"
    bool RenameOp(const UiDoc& doc, const String& id, const String& name, UiOp& op, String& label);

    // Hide / show: the op and its label ("id: Hide" / "id: Show")
    UiOp VisibilityOp(const UiNode& node, String& label);

    // --- the inspector -------------------------------------------------------------------------

    // Sets a field of a block: an empty value (null, "" or a missing one) unsets it. The label is "id: key"
    UiOp SetFieldOp(const UiNode& node, const String& key, const Json& value, String& label);

    // A margin or padding side set (i: top, right, bottom, left); all zero unsets it
    UiOp SetBoxOp(const UiNode& node, const String& key, int side, std::optional<double> value, String& label);

    // A new anchor: the block keeps its place on the screen (offsets, size and margins follow); snap (Alt) also moves it to
    // the anchor (x, y and margins cleared). False when nothing changes
    bool AnchorOp(const UiDoc& doc, const UiRects& rects, const String& id, const String& anchor, bool snap, UiOp& op,
                  String& label);

    // An anchor in words: "Top, full width", "Centre", "Fills the parent"
    String AnchorWords(const Anchor& anchor);

    // The column (h) and row (v) headings of the anchor presets: left / centre / right / ↔, top / middle / bottom / ↕
    String AnchorColumnName(const String& h);
    String AnchorRowName(const String& v);

    // --- history and chat ------------------------------------------------------------------------

    // A step's title: the request, one hand action, or "Hand edits · n"
    String StepTitle(const UiStep& step);

    // The local time of an ISO time, HH:MM ("" when it is none)
    String LocalTime(const String& iso);

    // The sentences of the inspector's segment labels and the like
    String AlignLabel(const String& v);   // start | center | end | stretch
    String JustifyLabel(const String& v); // start | center | end | between | around | evenly
    String FitLabel(const String& v);     // contain | cover | stretch
}
