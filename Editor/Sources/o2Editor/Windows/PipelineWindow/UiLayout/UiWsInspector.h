#pragma once

#include "o2Editor/Windows/PipelineWindow/UiLayout/UiWsPickers.h"
#include "o2Editor/Windows/PipelineWindow/UiLayout/UiWsStylePicker.h"

namespace Editor
{
    // -----------------------------------------------------------------------------------------------
    // The workspace's inspector (port of the Inspector of AssetsLine UiWorkspace.tsx): every property of
    // the selected block by its type - the name and the type, the placement (anchor presets: 4 x 4
    // pictures with ↔ / ↕ for stretched axes, the block keeps its place, Alt+click moves it to the anchor
    // too), offsets / nudge, size, margins, grow, the look (kit graphic picker with thumbnails, variant,
    // tint, fit), opacity, visibility, the children's layout (gap, padding, align, justify, the grid's
    // cells), the content (text, placeholder, the text style picked from previews, align, font size and
    // colour, value, on) and the note; for the screen: a notice while it is in older units with the step that
    // brings it to its device, the device (the kit's, read-only), its name, size and where the size comes
    // from, the DPI (the device's once in its pixels, read-only), "same as the reference", "as the kit's
    // device", backdrop and colour. Number pairs
    // carry their letter inside the field, colours their hex. Every change is a hand action (typing into
    // one field folds within 4 s)
    // -----------------------------------------------------------------------------------------------
    class UiWsInspector : public UiWsPanel
    {
    public:
        // Default constructor
        explicit UiWsInspector(RefCounter* refCounter);

        // Returns the field of a key shown now (num:x, num:w, box:margin:0, text:variant, area:text, area:note, name,
        // screen:w, screen:dpi, hex:color...), null when it is not shown
        Ref<EditBox> GetField(const String& key) const;

        // Returns the keys of the fields shown now
        Vector<String> GetFieldKeys() const;

        // Picks a choice the way its control does (tests): seg:<key>:<value>, sel:<key>:<value>, kit:<key>:<name>,
        // anchor:<name> (alt = Alt held), toggle:<key>
        void Choose(const String& id, bool alt = false);

        // Sets a slider's value as its drag does (opacity, value)
        void SetSlider(const String& key, float value);

        void OnScrolled(float scroll) override;
        void Update(float dt) override;

    protected:
        Map<String, Ref<EditBox>> mFields;
        Vector<String>            mUsed;          // The fields placed this frame
        String                    mFieldsFor;     // The block the fields show
        float                     mScroll = 0.0f;
        float                     mContentHeight = 0.0f;
        bool                      mAltClick = false;

        String mSliderKey;  // The slider dragged
        RectF  mSliderTrack;

        Ref<UiWsPopup>        mPopup;
        Ref<UiWsOptionsPanel> mOptions;
        Ref<UiWsKitGridPanel> mKitGrid;
        Ref<UiWsStyleListPanel> mStyles; // The text style picker's list
        String                mPopupFor;

    protected:
        void DrawPanel(const UiWsDraw::Box& box) override;
        void OnHotClick(const String& id, const Input::Cursor& cursor) override;
        void OnCursorPressed(const Input::Cursor& cursor) override;
        void OnCursorStillDown(const Input::Cursor& cursor) override;
        void OnCursorReleased(const Input::Cursor& cursor) override;

        // The block the inspector shows: the selected one, else the screen
        const UiDoc::UiNode* Shown() const;

        // A field placed at a rectangle (made on first use)
        Ref<EditBox> PlaceField(const String& key, const RectF& rect, const Function<Ref<EditBox>()>& make);

        // Writes a field of the shown block (an empty value unsets it)
        void Set(const String& key, const UiDoc::Json& value);

        // Writes a screen op
        void Screen(const UiDoc::UiOp& op, const String& label);

        // Opens a select's options under the rectangle
        void OpenOptions(const String& key, const RectF& anchor, const Vector<Pair<String, String>>& options, const String& current);

        // Opens the kit graphic picker for a key (kit)
        void OpenKitGrid(const RectF& anchor, const String& current);

        // Opens the text style picker's list for the shown block, right-aligned under the button
        void OpenStyles(const RectF& anchor);

        // Draws the screen's own rows of the root inspector (UiWsInspectorRoot.cpp) from y (the first row's top), moving y past
        // the last row: the older-units notice, the device, the size and where it comes from, the DPI, the two ways to set
        // the design
        void DrawRootScreen(const UiWsDraw::Box& box, float& y);

        // A click on one of the screen's rows; true when it was one (bring, asref, askit)
        bool OnRootClick(const String& id);

        // A slider's value at a world x
        void SlideTo(float x);
    };
}
