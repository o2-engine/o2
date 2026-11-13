#pragma once

#include "o2Editor/Windows/PipelineWindow/UiLayout/UiWsContext.h"

namespace o2
{
    class EditBox;
}

namespace Editor
{
    class UiWsStage;

    // ---------------------------------------------------------------------------------------------------------------
    // Points of a request inside its text (port of AssetsLine uiLayout/PinInput.tsx and the workspace's point tool): a
    // point placed on the screen is a numbered token [1], [2]... put into the request field at the caret ("make [1] larger
    // and move it under [2]"); the stored draft (uiDraft) is that plain text, so the card's field and the workspace's show
    // the same request. The points (uiPoints, design pixels) are numbered in the order their tokens stand; deleting a token
    // removes its point. On the stage the points are numbered pins: the point tool places one with a click, a pin can be
    // dragged to another place
    // ---------------------------------------------------------------------------------------------------------------
    namespace UiWsPins
    {
        // A request as it stands: the text with its tokens renumbered in the order they stand, and the points' old numbers in
        // that order (a point missing from it was deleted from the text)
        struct Settled
        {
            String      text;
            Vector<int> order;
        };
        Settled Settle(const String& raw, int count);

        // The text with the token of point n put in at the caret (characters; past the end: the end), a space either side
        // unless one is there; caretAfter is the caret past the space after the token (one already there is gone past)
        String InsertPin(const String& text, int caret, int n, int& caretAfter);

        // The request typed into a field: writes uiDraft and, when the tokens changed, uiPoints (no undo step). Returns the
        // settled text (when it differs from what was typed, the field shows it)
        String Typed(UiWsContext& ctx, const String& text);

        // A point placed at design (x, y), or (index >= 0) a point moved there; field: the request field that takes the token
        // at the caret (characters; -1: its end), null: the token goes to the end of the request
        void Place(UiWsContext& ctx, o2::EditBox* field, double x, double y, int index = -1, int caret = -1);

        // The request field's text with a token typed or removed: the field settled and the node written (onChanged of an
        // EditBox that edits uiDraft)
        void OnFieldChanged(UiWsContext& ctx, o2::EditBox* field);

        // --- the stage -----------------------------------------------------------------------------------------------

        // Draws the points as numbered pins over the screen
        void Draw(const UiWsStage& stage, const UiWsContext& ctx);

        // A press on the stage: grabs a pin under it, or with the point tool places a point; true when it was the pins'
        bool Press(const UiWsStage& stage, UiWsContext& ctx, const Vec2F& world);

        // The pin grabbed follows the cursor; true while a pin is dragged
        bool Drag(const UiWsStage& stage, UiWsContext& ctx, const Vec2F& world);

        // Ends a pin's drag; true when one was dragged
        bool Release(const UiWsStage& stage, UiWsContext& ctx, const Vec2F& world);
    }
}
