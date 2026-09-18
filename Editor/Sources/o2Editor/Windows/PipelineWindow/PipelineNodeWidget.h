#pragma once

#include "o2/Scene/UI/Widget.h"
#include "o2/Utils/Editor/DragHandle.h"
#include "o2Editor/Pipeline/PipelineNodeType.h"
#include "o2Editor/Windows/PipelineWindow/PipelineControls.h"

using namespace o2;

namespace o2
{
    class Button;
    class EditBox;
    class Label;
    class Sprite;
    class Text;
}

namespace Editor
{
    class PipelineEditor;
    class PipelineNodeBody;

    // ---------------------------------------------------------------------------------
    // Runtime state of a node shown by the editor: run state, freshness and last output
    // ---------------------------------------------------------------------------------
    struct PipelineNodeRuntime
    {
        String state = "idle";   // Run state: idle / queued / running / done / error
        String error;            // Message of the last failed run
        bool   fresh = false;    // True when the output is up to date with the inputs
        bool   applying = false; // True while an instant node applies its config

        int retryAttempt = 0, retryMax = 0, retryStatus = 0; // Current retry, retry limit and the status code of the failed try

        PipelineValue output;         // Last run result
        String        previewPath;    // Preview image file of the result
        String        srcPreviewPath; // Preview image file of the source, empty when there is none

        Map<String, PipelineValue> portOutputs; // Last result of each output of a per-port node, by port id
    };

    // ----------------------------------------------------------------------
    // Node card on the pipeline canvas: header with title and run button,
    // port rows on both sides, the type-specific body and resize handles on
    // every edge and corner
    // ----------------------------------------------------------------------
    class PipelineNodeWidget : public Widget
    {
    public:
        static const float headerHeight;  // Height of the title band
        static const float padTop;        // Gap between the header and the first port row
        static const float portRow;       // Height of one port row
        static const float padBottom;     // Gap below the body
        static const float portRadius;    // Radius of the drawn port circle
        static const float portHitRadius; // Radius of the port hit area
        static const float defaultWidth;  // Card width when neither the node nor the schema sets one
        static const float minWidth;      // Smallest card width
        static const float minHeight;     // Smallest card height

        // ----------------------------------------------------------
        // Port view: port data, its layers and custom input controls
        // ----------------------------------------------------------
        struct PortView
        {
            PipelinePort     port;         // Port data copied from the node
            bool             input = true; // True for an input port, false for an output
            Vec2F            localPos;     // Port center relative to the card left-top, y down
            Ref<IRectDrawable> circle;     // Type-shaped marker coloured by the port type, drawn by the editor above every card
            Ref<WidgetLayer> label;        // Port name text layer, fixed ports only
            Ref<EditBox>     nameEdit;     // Name edit box, custom inputs only
            Ref<Button>      deleteButton; // Remove button, custom inputs only

            // Returns true when the other view refers to the same port and direction
            bool operator==(const PortView& other) const { return port.id == other.port.id && input == other.input; }
        };

        // ---------------------------------------------------
        // Edge or corner resize handle and the sides it moves
        // ---------------------------------------------------
        struct ResizeHandle
        {
            Ref<DragHandle> handle;         // Invisible drag handle over the edge band
            bool            left = false;   // Moves the left side
            bool            right = false;  // Moves the right side
            bool            top = false;    // Moves the top side
            bool            bottom = false; // Moves the bottom side

            // Returns true when the other entry wraps the same drag handle
            bool operator==(const ResizeHandle& other) const { return handle == other.handle; }
        };

    public:
        Ref<DragHandle> dragHandle; // Moves the card

    public:
        // Default constructor
        explicit PipelineNodeWidget(RefCounter* refCounter);

        // Constructor with the owner editor and the node; builds the header, handles, ports and body
        PipelineNodeWidget(RefCounter* refCounter, const Ref<PipelineEditor>& editor, const Ref<PipelineNode>& node);

        // Destructor
        ~PipelineNodeWidget() override;

        // Returns the pipeline node shown by this card
        const Ref<PipelineNode>& GetNode() const { return mNode; }

        // Returns true and its offset in the body when the body places the output port itself
        bool IsBodyPort(const String& portId, Vec2F& offset) const;

        // Returns the owner editor, null when it was destroyed
        Ref<PipelineEditor> GetEditor() const;

        // Returns the runtime state of the node: run state, freshness and last output
        PipelineNodeRuntime& GetRuntime() { return mRuntime; }

        // Returns the schema of the node type, null for an unknown type
        const PipelineNodeSchema* GetSchema() const { return mSchema; }

        // Rebuilds ports and the body from the node data
        void Rebuild();

        // Re-reads position and size from the node and lays the card out
        void UpdateFromNode();

        // Returns the card rectangle in canvas space (y up)
        RectF GetCardRect() const;

        // Returns the card size: the node size, else the schema default width and the auto height
        Vec2F GetCardSize() const;

        // Returns the card width: the node width, else the schema default width
        float GetCardWidth() const;

        // Returns the port center in canvas space; the card corner when the port is missing
        Vec2F GetPortPosition(const String& portId, bool input) const;

        // Returns the port whose hit circle contains the canvas point, null when none
        const PortView* FindPortAt(const Vec2F& canvasPoint) const;

        // Returns true when the canvas point hits the add-input button
        bool IsAddInputAt(const Vec2F& canvasPoint) const;

        // Returns true when the canvas point is inside the header band
        bool IsHeaderAt(const Vec2F& canvasPoint) const;

        // Sets the selected flag and the "selected" visual state
        void SetSelected(bool selected);

        // Returns true when the card is selected
        bool IsSelected() const { return mSelected; }

        // Updates the visual states and the header buttons from the runtime state
        void ApplyRuntime();

        // Returns the body height the current node type wants
        float GetBodyHeight() const;

        // Returns the card height fitting the header, the port rows and the body
        float GetAutoHeight() const;

        // Returns the height of the port rows, including the add-input row
        float GetPortsHeight() const;

        // Notifies the body that the node output changed
        void OnOutputChanged();

        // Notifies the body that the node config changed
        void OnConfigChanged();

        // Sets the culled flag; a culled card is neither updated nor drawn
        void SetCulled(bool culled) { mCulled = culled; }

        // Returns true when the card is outside the view
        bool IsCulled() const { return mCulled; }

        // Sets the detail flag; without details the controls are skipped and only the frame, title, port names and body content are drawn
        void SetDetailed(bool detailed) { mDetailed = detailed; }

        // Returns true when the card draws its controls
        bool IsDetailed() const { return mDetailed; }

        // Updates the card unless it is culled
        void Update(float dt) override;

        // Updates the children unless the card is culled; without details the body updates for a few frames after a change
        void UpdateChildren(float dt) override;

        // Draws the card: nothing when culled; without details the layers and the body content only
        void Draw() override;

        // Draws the port circles in canvas space; the editor calls it after all cards so ports stay above every frame
        void DrawPorts();

        // Draws the state and selection outlines around the card art, scaled with the camera but never thinner than a pixel
        void DrawOutline();

        // Returns the icon image path of a node type
        static String IconForType(const String& nodeType);

        // Returns the editor-green variant of the node type icon, for menus that cannot tint
        static String MenuIconForType(const String& nodeType);

        SERIALIZABLE(PipelineNodeWidget);

    protected:
        WeakRef<PipelineEditor>   mEditor;           // Owner editor
        Ref<PipelineNode>         mNode;             // Node shown by this card
        const PipelineNodeSchema* mSchema = nullptr; // Schema of the node type, null for an unknown type
        PipelineNodeRuntime       mRuntime;          // Runtime state of the node
        bool                      mSelected = false; // True when the card is selected
        bool                      mCulled = false;   // True when the card is outside the view
        bool                      mDetailed = true;  // False when the view is zoomed too far out for the controls
        int                       mFarUpdateFrames = 0; // Frames the body still updates without details; layout and content changes reset it

        Ref<WidgetLayer>      mTitleLayer;     // Title text layer
        Ref<WidgetLayer>      mIconLayer;      // Node type icon layer
        Ref<Button>           mPlayButton;     // Run / stop button
        Ref<WidgetLayer>      mDotLayer;       // Status dot before the play button: fresh, stale, idle or running
        Ref<Button>           mErrorButton;    // Opens the last error, shown in the error state
        Ref<Label>            mRetryLabel;     // Retry counter badge, shown while retrying
        Ref<Button>           mAddInputButton; // Adds a custom input, shown when the schema allows it
        Ref<Widget>           mBodyHost;       // Container of the type-specific body
        Ref<PipelineNodeBody> mBody;           // Type-specific body

        Vector<PortView> mInputs;  // Input port views
        Vector<PortView> mOutputs; // Output port views

        Vector<ResizeHandle> mResizeHandles;   // Edge and corner handles resizing the card
        RectF                mResizeStartRect; // Card rectangle when the resize drag began
        bool                 mResizing = false; // True while a resize handle is dragged

    protected:
        // Creates the icon, the title, the header buttons and the body host
        void BuildHeader();

        // Creates the eight invisible resize handles over the edges and corners
        void BuildResizeHandles(const Ref<PipelineEditor>& editor);

        // Returns true when the canvas point lies in the band of the resize handle and not on a port
        bool IsResizeHandleAt(const ResizeHandle& handle, const Vec2F& p) const;

        // Recreates port layers and custom input controls from the node ports
        void BuildPorts();

        // Recreates the type-specific body
        void BuildBody();

        // Places port circles, labels and custom input controls in rows
        void LayoutPorts();

        // Places the body host below the port rows
        void LayoutBody();

        // Updates the play, error and retry controls from the runtime state
        void UpdateHeaderButtons();

        // Called when the drag handle moves; forwards to the editor
        void OnDragged(const Vec2F& position);

        // Called when the drag is completed; forwards to the editor
        void OnDragCompleted();

        // Called when a resize handle moves; moves the sides it owns, writes the node position and size and lays the card out
        void OnResizeDragged(const ResizeHandle& handle, const Vec2F& position);

        // Called when the resize is completed; forwards to the editor
        void OnResizeCompleted();

        // Called when the play button is pressed; runs the node or stops the current run
        void OnPlayPressed();

        // Called when the add-input button is pressed; asks the editor for a new custom input
        void OnAddInputPressed();

        // Called when a custom input name edit is completed; renames the port through the editor
        void OnCustomInputRenamed(const String& portId, const String& name);

        // Called when a custom input delete button is pressed; removes the port through the editor
        void OnCustomInputDeleted(const String& portId);

        // Opens the node context menu through the editor
        void OpenContextMenu();

        // Returns the color of the port type
        Color4 ColorOfPortType(PipelinePortType type) const;

        // Returns the icon image path of the node type
        String IconForType() const;
    };
}
// --- META ---

CLASS_BASES_META(Editor::PipelineNodeWidget)
{
    BASE_CLASS(o2::Widget);
}
END_META;
CLASS_FIELDS_META(Editor::PipelineNodeWidget)
{
    FIELD().PUBLIC().NAME(dragHandle);
    FIELD().PROTECTED().NAME(mEditor);
    FIELD().PROTECTED().NAME(mNode);
    FIELD().PROTECTED().DEFAULT_VALUE(nullptr).NAME(mSchema);
    FIELD().PROTECTED().NAME(mRuntime);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mSelected);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mCulled);
    FIELD().PROTECTED().DEFAULT_VALUE(true).NAME(mDetailed);
    FIELD().PROTECTED().DEFAULT_VALUE(0).NAME(mFarUpdateFrames);
    FIELD().PROTECTED().NAME(mTitleLayer);
    FIELD().PROTECTED().NAME(mIconLayer);
    FIELD().PROTECTED().NAME(mPlayButton);
    FIELD().PROTECTED().NAME(mDotLayer);
    FIELD().PROTECTED().NAME(mErrorButton);
    FIELD().PROTECTED().NAME(mRetryLabel);
    FIELD().PROTECTED().NAME(mAddInputButton);
    FIELD().PROTECTED().NAME(mBodyHost);
    FIELD().PROTECTED().NAME(mBody);
    FIELD().PROTECTED().NAME(mInputs);
    FIELD().PROTECTED().NAME(mOutputs);
    FIELD().PROTECTED().NAME(mResizeHandles);
    FIELD().PROTECTED().NAME(mResizeStartRect);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mResizing);
}
END_META;
CLASS_METHODS_META(Editor::PipelineNodeWidget)
{

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*, const Ref<PipelineEditor>&, const Ref<PipelineNode>&);
    FUNCTION().PUBLIC().SIGNATURE(const Ref<PipelineNode>&, GetNode);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsBodyPort, const String&, Vec2F&);
    FUNCTION().PUBLIC().SIGNATURE(Ref<PipelineEditor>, GetEditor);
    FUNCTION().PUBLIC().SIGNATURE(PipelineNodeRuntime&, GetRuntime);
    FUNCTION().PUBLIC().SIGNATURE(const PipelineNodeSchema*, GetSchema);
    FUNCTION().PUBLIC().SIGNATURE(void, Rebuild);
    FUNCTION().PUBLIC().SIGNATURE(void, UpdateFromNode);
    FUNCTION().PUBLIC().SIGNATURE(RectF, GetCardRect);
    FUNCTION().PUBLIC().SIGNATURE(Vec2F, GetCardSize);
    FUNCTION().PUBLIC().SIGNATURE(float, GetCardWidth);
    FUNCTION().PUBLIC().SIGNATURE(Vec2F, GetPortPosition, const String&, bool);
    FUNCTION().PUBLIC().SIGNATURE(const PortView*, FindPortAt, const Vec2F&);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsAddInputAt, const Vec2F&);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsHeaderAt, const Vec2F&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetSelected, bool);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsSelected);
    FUNCTION().PUBLIC().SIGNATURE(void, ApplyRuntime);
    FUNCTION().PUBLIC().SIGNATURE(float, GetBodyHeight);
    FUNCTION().PUBLIC().SIGNATURE(float, GetAutoHeight);
    FUNCTION().PUBLIC().SIGNATURE(float, GetPortsHeight);
    FUNCTION().PUBLIC().SIGNATURE(void, OnOutputChanged);
    FUNCTION().PUBLIC().SIGNATURE(void, OnConfigChanged);
    FUNCTION().PUBLIC().SIGNATURE(void, SetCulled, bool);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsCulled);
    FUNCTION().PUBLIC().SIGNATURE(void, SetDetailed, bool);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsDetailed);
    FUNCTION().PUBLIC().SIGNATURE(void, Update, float);
    FUNCTION().PUBLIC().SIGNATURE(void, UpdateChildren, float);
    FUNCTION().PUBLIC().SIGNATURE(void, Draw);
    FUNCTION().PUBLIC().SIGNATURE(void, DrawPorts);
    FUNCTION().PUBLIC().SIGNATURE(void, DrawOutline);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, IconForType, const String&);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, MenuIconForType, const String&);
    FUNCTION().PROTECTED().SIGNATURE(void, BuildHeader);
    FUNCTION().PROTECTED().SIGNATURE(void, BuildResizeHandles, const Ref<PipelineEditor>&);
    FUNCTION().PROTECTED().SIGNATURE(bool, IsResizeHandleAt, const ResizeHandle&, const Vec2F&);
    FUNCTION().PROTECTED().SIGNATURE(void, BuildPorts);
    FUNCTION().PROTECTED().SIGNATURE(void, BuildBody);
    FUNCTION().PROTECTED().SIGNATURE(void, LayoutPorts);
    FUNCTION().PROTECTED().SIGNATURE(void, LayoutBody);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateHeaderButtons);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDragged, const Vec2F&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDragCompleted);
    FUNCTION().PROTECTED().SIGNATURE(void, OnResizeDragged, const ResizeHandle&, const Vec2F&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnResizeCompleted);
    FUNCTION().PROTECTED().SIGNATURE(void, OnPlayPressed);
    FUNCTION().PROTECTED().SIGNATURE(void, OnAddInputPressed);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCustomInputRenamed, const String&, const String&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCustomInputDeleted, const String&);
    FUNCTION().PROTECTED().SIGNATURE(void, OpenContextMenu);
    FUNCTION().PROTECTED().SIGNATURE(Color4, ColorOfPortType, PipelinePortType);
    FUNCTION().PROTECTED().SIGNATURE(String, IconForType);
}
END_META;
// --- END META ---
