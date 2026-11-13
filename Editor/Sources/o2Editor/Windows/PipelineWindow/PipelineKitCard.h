#pragma once

#include <optional>

#include "o2/Events/KeyboardEventsListener.h"
#include "o2Editor/Pipeline/UiDoc/UiKitModel.h"
#include "o2Editor/Windows/PipelineWindow/PipelineKitLayout.h"
#include "o2Editor/Windows/PipelineWindow/PipelineNodeBody.h"
#include "o2Editor/Windows/PipelineWindow/UiLayout/UiWsWidgets.h"

// The UI kit card (port of AssetsLine UiKitBody.tsx, KitPopup.tsx, KitDevicePicker.tsx): the device the kit is made
// for, a head - what the kit holds, AI and its models - and a grid of cells, one per input (a picture or a text style),
// as many columns as the card is wide (PipelineKitLayout.h). A cell shows the element as it is in the kit and carries its
// input's port on its top-left corner; a click opens the element's settings in a popup beside it, which zooms with the
// canvas and follows its cell. The parts are drawn panels of the AI UI layout workspace (UiWsWidgets.h)
namespace Editor
{
    class PipelineKitPopup;
    class PipelineKitGrid;
    class PipelineKitDeviceChip;
    class PipelineKitGuides;

    // ----------------------------------------------------------------------------------------
    // One cell of the kit's grid as the card shows it: the input, what it is, what draws it
    // ----------------------------------------------------------------------------------------
    struct PipelineKitCell
    {
        String id;             // The input port
        bool   style = false;  // A text style input, else a picture
        bool   linked = false; // Something is wired into it
        String name;           // The element's name in the kit
        String meta;           // The type line: the element type (and asset), a style's font and size
        String purpose;        // What it is for (a style's description)
        bool   ai = false;     // Its meta was filled in by the AI
        bool   soft = false;   // Its picture has too few pixels for its size on the device
        bool   busy = false;   // The AI describes it now

        std::optional<UiDoc::UiKitElement> element; // The kit's element made of it, once the kit is rendered
        Ref<Bitmap>                        picture; // The input's picture before the kit has it, a style's "Aa"

        bool operator==(const PipelineKitCell& other) const { return id == other.id; }
    };

    // ----------------------------------------------------------------------------------------
    // The UI kit node's body: the device row, the head, the models and the grid; the open
    // element's popup is drawn above every card. Inputs sit on the cells' corners
    // ----------------------------------------------------------------------------------------
    class PipelineUiKitBody : public PipelineNodeBody
    {
    public:
        using PipelineNodeBody::PipelineNodeBody;

        void Build() override;
        void OnOutputChanged() override;
        void OnConfigChanged() override;
        void Update(float dt) override;

        // Every input sits on its cell's top-left corner; a link dropped on the grid becomes a new input
        bool InputsInBody() const override { return true; }
        bool InputsInCells() const override { return true; }
        bool HasBodyPort(const String& portId) const override;
        bool GetBodyPortOffset(const String& portId, Vec2F& offset) const override;
        bool IsAddInputAt(const Vec2F& point) const override;

        // Draws the open element's popup
        void DrawOverlay() override;

        // --- what the cells and the popup use ---

        // Returns the node shown
        const Ref<PipelineNode>& GetNode() const { return mNode; }

        // Returns the editor of the card
        Ref<PipelineEditor> GetEditor() const;

        // Returns an input's settings (config.items[port]), an empty object for none
        UiDoc::Json ItemOf(const String& portId) const;

        // Writes fields of an input's settings (a null value is stored as null) and drops others, as one undo step
        void SetItemFields(const String& portId, const Vector<Pair<String, UiDoc::Json>>& fields, const Vector<String>& removed = {});

        // Returns an input's name in the kit
        String NameOf(const PipelinePort& port, int index) const;

        // Returns the kit's element of an input, null before the kit has it
        const UiDoc::UiKitElement* ElementOf(const String& portId) const;

        // Returns the device the kit on screen was made for (the config's before the kit is rendered)
        UiDoc::UiTarget Device() const;

        // Returns the value wired into an input as the canvas shows it
        PipelineValue InputValue(const String& portId) const;

        // Opens an element's settings (a second open of the same one closes them); "" closes
        void OpenCell(const String& portId);

        // Returns the input whose settings are open, "" for none
        String GetOpenCell() const;

        // Removes an input with its settings and links
        void RemoveInput(const String& portId);

        // Fills the meta of one picture ("" = every picture) with the AI
        void Analyze(const String& portId);

        // Returns the grid of cells
        const Ref<PipelineKitGrid>& GetGrid() const { return mGrid; }

        // Returns the open element's popup, null while none is open
        Ref<PipelineKitPopup> GetPopup() const;

        // Returns the device chip
        const Ref<PipelineKitDeviceChip>& GetDeviceChip() const { return mDevice; }

    protected:
        Ref<PipelineKitDeviceChip> mDevice;  // The device the kit is made for
        Ref<Button>                mAiAll;   // The AI for every picture
        Ref<Label>                 mSummary; // What the kit holds
        Ref<PipelineKitGrid>       mGrid;    // The cells
        Ref<PipelineKitPopup>      mPopup;   // The open element's settings

        UiDoc::UiKit mKit;     // The node's own output as last read
        String       mKitText;
        float        mPoll = 0.0f;
        Vec2F        mRightPress; // The camera when the right button was pressed outside the popup
        bool         mRightPressed = false;
        bool         mPopupDirty = false; // The popup shows the settings again on the next update (never from its own field's callback)

    protected:
        void ReadKit();
        void RefreshCells();
        void SyncPopup();
        void PlacePopup();
        void WatchOutside();
        PipelineKitLayout::Grid Grid() const;
    };

    // ----------------------------------------------------------------------------------------
    // The kit's cells: a square picture each (the element as the kit made it, else the input's
    // picture; a style shows "Aa" in its look), the name and the type line under it, a mark for
    // what the AI filled and a warning for a soft picture, the bin on hover; then the "+ input"
    // cell. The ports on the cells' corners belong to the editor's link gesture
    // ----------------------------------------------------------------------------------------
    class PipelineKitGrid : public UiWsPanel
    {
    public:
        Function<void(const String&)> onOpen;   // A cell was clicked
        Function<void(const String&)> onRemove; // A cell's bin was clicked
        Function<void()>              onAdd;    // The "+ input" cell was clicked

    public:
        // Default constructor
        explicit PipelineKitGrid(RefCounter* refCounter);

        // Shows the cells; the open one is outlined
        void SetCells(const Vector<PipelineKitCell>& cells, const String& openId, bool canAdd);

        // Returns the cells shown
        const Vector<PipelineKitCell>& GetCells() const { return mCells; }

        // Returns the grid for the panel's width
        PipelineKitLayout::Grid GetGrid() const;

        // Returns the world rectangle of a cell's picture (of the add cell for ""), an empty one for none
        RectF GetCellRect(const String& id) const;

        // Returns the world rectangle of a cell with its captions
        RectF GetCellArea(const String& id) const;

        // Inside the panel, except at the cells' corners (their ports)
        bool IsUnderPoint(const Vec2F& point) override;

    protected:
        Vector<PipelineKitCell> mCells;
        String                  mOpen;
        bool                    mCanAdd = true;

        // Textures of the pictures shown, by cell
        struct Pic
        {
            Ref<Bitmap> bitmap;
            TextureRef  texture;
            Vec2I       size;
        };
        Map<String, Pic> mPics;

    protected:
        void DrawPanel(const UiWsDraw::Box& box) override;
        void OnHotClick(const String& id, const Input::Cursor& cursor) override;
        void DrawCell(const UiWsDraw::Box& box, const PipelineKitLayout::Grid& g, int index, const PipelineKitCell& cell);
        void DrawAddCell(const UiWsDraw::Box& box, const PipelineKitLayout::Grid& g);
    };

    // ----------------------------------------------------------------------------------------
    // An element's settings, beside its cell: a picture's name, type, its source with the trim box
    // and the draggable nine-slice guides, asset, variant, background, margins, its size on the
    // kit's device (inches), the density of its texture and the texture the kit made, the borders,
    // its purpose and description; a text style's name in the kit and its sample. Drawn above every
    // card in canvas space: it zooms with the canvas
    // ----------------------------------------------------------------------------------------
    class PipelineKitPopup : public UiWsPanel, public KeyboardEventsListener
    {
    public:
        static constexpr float width = 384.0f; // Width in canvas units
        static constexpr float gapToCell = 10.0f;

        WeakRef<PipelineUiKitBody> body;
        String                     portId;
        bool                       overlay = false; // Drawn only in the editor's overlay pass

    public:
        // Default constructor
        explicit PipelineKitPopup(RefCounter* refCounter);

        // Creates the fields for the input's settings as they are now
        void Rebuild();

        // Returns the height it wants
        float GetContentHeight() const { return mHeight; }

        // Puts it beside the grid, level with the cell (world rectangles): right of the grid when there is room in the visible
        // area, else left of it
        void Place(const RectF& cell, const RectF& grid, const RectF& visible);

        // Returns true while one of its fields is typed in
        bool HasFocusedField() const;

        // Returns a field by its key (name, variant, sizeW, sizeH, dpi, nineL..., purpose, description), null for none
        Ref<EditBox> GetField(const String& key) const;

        // Returns the source with the guides, null for a style
        const Ref<PipelineKitGuides>& GetGuides() const { return mGuides; }

        // Clicks a spot (tests): type, close, asset:<v>, bg:<v>, trim, dpi:kit|own, lock, sizeAuto, nineAuto, middle, ai, remove
        void Click(const String& id) { ClickHot(id); }

        // Returns the size lock
        bool IsLocked() const { return mLocked; }

        void Draw() override;

    protected:
        Map<String, Ref<EditBox>> mFields;
        Ref<PipelineKitGuides>    mGuides;
        float                     mHeight = 300.0f;
        bool                      mLocked = true;
        std::optional<double>     mKeptRatio;   // height / width kept while the sides are typed with the lock on
        Ref<Bitmap>               mStyleSample; // A style's sample
        TextureRef                mStyleTexture;
        String                    mStyleKey;

    protected:
        void DrawPanel(const UiWsDraw::Box& box) override;
        void OnHotClick(const String& id, const Input::Cursor& cursor) override;

        // Escape closes it, unless a field of it is typed in (there Escape drops what was typed)
        void OnKeyPressed(const Input::Key& key) override;

        // A press outside it and its cell closes it (on the cell: the click that follows toggles it)
        void OnCursorPressedOutside(const Input::Cursor& cursor) override;

        // Lays the content out (draw = false only places the fields); returns its height
        float Lay(const UiWsDraw::Box& box, bool draw);
        float LayElement(const UiWsDraw::Box& box, bool draw, float y);
        float LayDevice(const UiWsDraw::Box& box, bool draw, float y);
        float LayStyle(const UiWsDraw::Box& box, bool draw, float y);

        // Field helpers
        void Field(const String& key, const RectF& rect);
        Ref<EditBox> AddText(const String& key, const String& text, const String& placeholder, bool multiline,
                             const Function<void(const String&)>& onDone);
        Ref<EditBox> AddNumber(const String& key, const String& letter, std::optional<double> value, double min,
                               const Function<void(std::optional<double>)>& onValue);

        // The size row's write: a side typed (none = back to auto), the other following it while locked
        void SetSide(bool width, std::optional<double> value);
        void PickType();
    };

    // ----------------------------------------------------------------------------------------
    // The source picture with its trim box and the nine-slice guides (AssetsLine SourceGuides):
    // the guides drag, in input pixels; opposite guides never cross
    // ----------------------------------------------------------------------------------------
    class PipelineKitGuides : public PipelineImageView
    {
    public:
        Function<void(const UiDoc::UiNine&)> onLive; // A guide moved
        Function<void(const UiDoc::UiNine&)> onSet;  // A guide was let go

    public:
        // Default constructor: the four guides' handles
        explicit PipelineKitGuides(RefCounter* refCounter);

        // Sets the picture's own size (input pixels), the trim box and the borders shown; none hides the guides
        void SetGuides(int width, int height, const UiDoc::Json& trim, const std::optional<UiDoc::UiNine>& nine, bool automatic);

        void Draw() override;

    private:
        Vector<Ref<DragHandle>>      mHandles; // left, right, top, bottom
        int                          mW = 0, mH = 0;
        UiDoc::Json                  mTrim;
        std::optional<UiDoc::UiNine> mNine;
        bool                         mAuto = true;
        bool                         mDragging = false;

    private:
        void GuideLine(int side, Vec2F& a, Vec2F& b) const;
        bool IsOnGuide(int side, const Vec2F& p) const;
        void OnDragged(int side, const Vec2F& p);
    };

    // ----------------------------------------------------------------------------------------
    // The device a kit is made for: a chip on the card - the device's name, its screen in pixels
    // and its density - opening the known devices by kind and a screen typed by hand (kitDevice: an
    // id, or "custom" with kitDeviceCustom {w, h, dpi}); the typed numbers apply on Enter or when the
    // field is left, Escape drops them
    // ----------------------------------------------------------------------------------------
    class PipelineKitDeviceChip : public UiWsPanel
    {
    public:
        // Default constructor
        explicit PipelineKitDeviceChip(RefCounter* refCounter);

        // Binds it to the kit card
        void Bind(const Ref<PipelineUiKitBody>& body);

        // Returns the width it wants for its text
        float GetPreferredWidth() const;

        // Opens or closes the menu
        void ToggleMenu();

        // Returns true while the menu is open
        bool IsMenuOpen() const;

        // Returns the menu's panel (tests), null before the first open
        Ref<UiWsPanel> GetMenuPanel() const;

        // Picks a known device as the menu does
        void Pick(const String& deviceId);

        // Sets one side of the custom screen (w, h or dpi) as its fields do; the custom screen becomes the kit's
        void SetCustom(const String& key, std::optional<double> value);

        // Returns the custom screen the menu shows: the kit's own when it is custom, else the one typed before, else the device
        UiDoc::UiScreen CustomScreen() const;

        // Returns the kit's device
        UiDoc::UiTarget Target() const;

        // Returns the body it belongs to
        Ref<PipelineUiKitBody> GetBody() const { return mBody.Lock(); }

    protected:
        WeakRef<PipelineUiKitBody> mBody;
        Ref<UiWsPanel>             mMenuPanel;
        Ref<UiWsPopup>             mMenu;

    protected:
        void DrawPanel(const UiWsDraw::Box& box) override;
        void OnHotClick(const String& id, const Input::Cursor& cursor) override;
    };

    namespace PipelineKitCards
    {
        // The kind of a device for its icon (phone, tablet, desktop; custom for a typed screen)
        String TargetKind(const UiDoc::UiTarget& target);

        // The name of a device ("Custom" for a typed screen)
        String TargetName(const UiDoc::UiTarget& target);

        // "1179×2556 · 460 dpi"
        String ScreenText(const UiDoc::UiScreen& screen);

        // A dpi as the cards show it: one decimal at most
        String DpiText(double dpi);

        // The name of an element type ("header / ribbon") and of an asset type ("9-slice")
        String TypeName(const String& type);
        String AssetName(const String& asset);
    }
}
