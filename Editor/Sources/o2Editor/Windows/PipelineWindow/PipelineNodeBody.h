#pragma once

#include "o2/Scene/UI/Widget.h"
#include "o2/Utils/Editor/FrameHandles.h"
#include "o2/Utils/Math/Basis.h"
#include "o2Editor/Pipeline/PipelineNodeType.h"
#include "o2Editor/Windows/PipelineWindow/PipelineControls.h"
#include "o2Editor/Windows/PipelineWindow/PipelineMediaViews.h"

using namespace o2;

namespace o2
{
    class Button;
    class DropDown;
    class EditBox;
    class HorizontalLayout;
    class Label;
    class Toggle;
}

namespace Editor
{
    class PipelineCropEditor;
    class PipelineEditor;
    class PipelineNodeWidget;

    // --------------------------------------------------------------------
    // Base of the type-specific node bodies: stacks rows top to bottom and
    // routes parameter edits to the editor (undo, dirty state, auto-apply)
    // --------------------------------------------------------------------
    class PipelineNodeBody : public Widget
    {
    public:
        // Default constructor
        explicit PipelineNodeBody(RefCounter* refCounter);

        // Binds the body to its owner widget, the edited node and the editor
        void Init(const Ref<PipelineNodeWidget>& owner);

        // Creates the rows of the node type; overridden by the concrete bodies
        virtual void Build() {}

        // Called when the runtime output of the node changed, refreshes the result views
        virtual void OnOutputChanged();

        // Called when the node config changed outside the body (undo, paste, another view)
        virtual void OnConfigChanged() {}

        // Returns the summed height of the rows with spacing when the body is width wide
        float GetPreferredHeight(float width) const;

        // Lays the rows out top to bottom in the given size, sharing the spare height between the flexible rows
        void Relayout(float width, float height);

        // Marks the row of the widget as content, drawn even when the view is zoomed far out
        void MarkContent(const Ref<Widget>& widget);

        // Draws the body; zoomed far out only the content rows are drawn
        void Draw() override;

        SERIALIZABLE(PipelineNodeBody);

    public:
        // -----------------------------------------------
        // Body row: a widget with its height in the stack
        // -----------------------------------------------
        struct Row
        {
            Ref<Widget> widget;           // Row widget
            float       height = 22.0f;   // Fixed height, or the minimum height when flexible
            bool        flexible = false; // True when the row takes a share of the spare height
            bool        content = false;  // True for what the node produced; it stays drawn when the view is zoomed far out

            Function<float(float)> heightForWidth; // Height (or minimum height) for the row width, used instead of height when set

            // Rows are equal when they hold the same widget
            bool operator==(const Row& other) const { return widget == other.widget; }
        };

        WeakRef<PipelineNodeWidget> mOwner;  // Owner node widget
        WeakRef<PipelineEditor>     mEditor; // Pipeline editor the owner belongs to
        Ref<PipelineNode>           mNode;   // Edited node

        Vector<Row> mRows;            // Rows stacked top to bottom
        float       mPadding = 10.0f; // Horizontal padding of the rows
        float       mSpacing = 6.0f;  // Vertical spacing between the rows

        Ref<PipelineImageView>  mImageView;  // Result image view, when the body has one
        Ref<PipelineTextView>   mTextView;   // Result text view, when the body has one
        Ref<PipelineAudioView>  mAudioView;  // Result audio player, when the body has one
        Ref<PipelineCropEditor> mCropEditor; // Result image with the crop frame, when the body has one
        Ref<PipelineVideoView>  mVideoView;  // Result video player, when the body has one

    public:
        // Appends a fixed-height row and adds its widget as a child
        Ref<Widget> AddRow(const Ref<Widget>& widget, float height);

        // Appends a row whose height follows the row width, for controls that wrap
        Ref<Widget> AddRow(const Ref<Widget>& widget, const Function<float(float)>& heightForWidth);

        // Appends a row that shares the spare height of the body, never shorter than minHeight
        Ref<Widget> AddFlexible(const Ref<Widget>& widget, float minHeight);

        // Appends a flexible row whose minimum height follows the row width
        Ref<Widget> AddFlexible(const Ref<Widget>& widget, const Function<float(float)>& minHeightForWidth);

        // Draws the content rows when zoomed far out; the composer draws its stage instead
        virtual void DrawFarContent();

        // Removes all rows and drops the result views
        void ClearRows();

        // Returns the height of the row for the row width
        static float RowHeight(const Row& row, float rowWidth);

        // Returns the node config string under key, or def when it is missing
        String GetString(const String& key, const String& def = "") const;

        // Returns the node config number under key, or def when it is missing
        float GetNumber(const String& key, float def = 0.0f) const;

        // Returns the node config flag under key, or def when it is missing
        bool GetBool(const String& key, bool def = false) const;

        // Writes a config string and notifies the editor; completed marks the end of an edit
        void SetString(const String& key, const String& value, bool completed = true);

        // Writes a config number and notifies the editor; completed marks the end of an edit
        void SetNumber(const String& key, float value, bool completed = true);

        // Writes a config flag and notifies the editor
        void SetBool(const String& key, bool value, bool completed = true);

        // Reports a change of the config key to the editor, which records undo and marks the pipeline dirty
        void Notify(const String& key, bool completed);

        // Rebuilds the whole body through the owner widget, for rows that depend on a toggle
        void RebuildBody();

        // Returns the current runtime output of the node
        PipelineValue GetOutput() const;

        // Returns the output as a bitmap, null when the output is not an image
        Ref<Bitmap> GetOutputBitmap() const;

        // Returns the uncropped source preview saved by the runtime, falling back to the output bitmap
        Ref<Bitmap> GetSourceOutputBitmap() const;

        // Returns the value connected to the named input port
        PipelineValue GetInput(const String& portName) const;

        // Adds the "Model" dropdown row bound to the "model" config key
        Ref<DropDown> AddModelRow(const Vector<String>& presets, const String& defaultModel);

        // Adds a labelled dropdown row bound to a config key
        Ref<DropDown> AddSelectRow(const String& label, const String& key, const Vector<String>& options, const String& def);

        // Adds a multiline edit bound to a config key; height <= 0 makes the row flexible
        Ref<EditBox> AddTextArea(const String& key, const String& placeholder, float height);

        // Adds a labelled single-line edit bound to a config key
        Ref<EditBox> AddTextRow(const String& label, const String& key, const String& placeholder = "");

        // Adds a checkbox row bound to a config flag
        Ref<Toggle> AddCheckbox(const String& caption, const String& key, bool def);

        // Adds a slider row bound to a config number
        Ref<PipelineSlider> AddSlider(const String& label, const String& key, float minValue, float maxValue, float step, float def, const String& suffix = "");

        // Adds a color field row bound to a hex color config key
        Ref<PipelineColorField> AddColor(const String& label, const String& key, const String& defHex);

        // Adds a row of exclusive (value, caption) segments bound to a config key; rebuilds the body on change
        void AddSegmented(const String& key, const Vector<Pair<String, String>>& options, const String& def);

        // Adds a muted hint line, growing the row for text that wraps
        void AddMutedLine(const String& text, float height = 18.0f);

        // Adds the "Inherit seed" toggle with the seed edit box
        void AddSeedRow();

        // Adds the transparent background toggle with the two-pass / chroma key settings
        void AddTransparencyBlock();

        // Adds the flexible result image view
        void AddResultImage(const String& hint, float minHeight = 120.0f);

        // Adds the flexible result text view
        void AddResultText(const String& hint, float minHeight = 70.0f);

        // Adds the result audio player row
        void AddResultAudio(const String& hint);

        // Adds the flexible result video player
        void AddResultVideo(const String& hint, float minHeight = 170.0f);

        // Adds the "Result" header with the crop toggle and the crop editor below it
        void AddCropSection(const String& hint, float minHeight = 150.0f);

        // Adds a button opening a file dialog; the pick is stored as an asset path or copied into uploads
        void AddFilePicker(const String& buttonCaption, const Vector<String>& extensions, bool image);
    };

    // -------------------------------------------------------------------------------
    // Image preview with a crop frame the user drags; writes crop {x,y,w,h} fractions
    // -------------------------------------------------------------------------------
    class PipelineCropEditor : public PipelineImageView
    {
    public:
        Function<void(bool completed)> onCropChanged; // Called when the crop frame moved; completed marks the end of a drag

    public:
        // Default constructor
        explicit PipelineCropEditor(RefCounter* refCounter);

        // Sets the node and the config key the crop fractions are stored under
        void SetNode(const Ref<PipelineNode>& node, const String& key = "crop");

        // Shows or hides the crop frame
        void SetCropEnabled(bool enabled);

        // Draws the image, shades the cut-away area and draws the frame handles
        void Draw() override;

        SERIALIZABLE(PipelineCropEditor);

    protected:
        Ref<PipelineNode> mNode;                // Node holding the crop config
        String            mKey;                 // Config key the crop object is stored under
        bool              mCropEnabled = false; // True when the crop frame is shown and editable

        Ref<FrameHandles> mFrame;                 // Draggable crop frame
        bool              mSyncing = false;       // True while the frame is set from config, mutes the transform callback
        bool              mFrameDragging = false; // True while the frame is pressed, config is not pushed back to the frame

    protected:
        // Places the frame over the image from the crop fractions in the config
        void SyncFrameFromConfig();

        // Called while the frame is dragged, writes the crop fractions to the config
        void OnFrameTransformed(const Basis& basis);

        // Called when the frame drag ended, reports the completed change
        void OnFrameCompleted();
    };

    namespace PipelineNodeBodies
    {
        // Creates the body for the node type and binds it to the owner, null for unknown types
        Ref<PipelineNodeBody> Create(const String& type, const Ref<PipelineNodeWidget>& owner);
    }
}
// --- META ---

CLASS_BASES_META(Editor::PipelineNodeBody)
{
    BASE_CLASS(o2::Widget);
}
END_META;
CLASS_FIELDS_META(Editor::PipelineNodeBody)
{
    FIELD().PUBLIC().NAME(mOwner);
    FIELD().PUBLIC().NAME(mEditor);
    FIELD().PUBLIC().NAME(mNode);
    FIELD().PUBLIC().NAME(mRows);
    FIELD().PUBLIC().DEFAULT_VALUE(10.0f).NAME(mPadding);
    FIELD().PUBLIC().DEFAULT_VALUE(6.0f).NAME(mSpacing);
    FIELD().PUBLIC().NAME(mImageView);
    FIELD().PUBLIC().NAME(mTextView);
    FIELD().PUBLIC().NAME(mAudioView);
    FIELD().PUBLIC().NAME(mCropEditor);
    FIELD().PUBLIC().NAME(mVideoView);
}
END_META;
CLASS_METHODS_META(Editor::PipelineNodeBody)
{

    typedef const Vector<Pair<String, String>>& _tmp1;

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().SIGNATURE(void, Init, const Ref<PipelineNodeWidget>&);
    FUNCTION().PUBLIC().SIGNATURE(void, Build);
    FUNCTION().PUBLIC().SIGNATURE(void, OnOutputChanged);
    FUNCTION().PUBLIC().SIGNATURE(void, OnConfigChanged);
    FUNCTION().PUBLIC().SIGNATURE(float, GetPreferredHeight, float);
    FUNCTION().PUBLIC().SIGNATURE(void, Relayout, float, float);
    FUNCTION().PUBLIC().SIGNATURE(void, MarkContent, const Ref<Widget>&);
    FUNCTION().PUBLIC().SIGNATURE(void, Draw);
    FUNCTION().PUBLIC().SIGNATURE(Ref<Widget>, AddRow, const Ref<Widget>&, float);
    FUNCTION().PUBLIC().SIGNATURE(Ref<Widget>, AddRow, const Ref<Widget>&, const Function<float(float)>&);
    FUNCTION().PUBLIC().SIGNATURE(Ref<Widget>, AddFlexible, const Ref<Widget>&, float);
    FUNCTION().PUBLIC().SIGNATURE(Ref<Widget>, AddFlexible, const Ref<Widget>&, const Function<float(float)>&);
    FUNCTION().PUBLIC().SIGNATURE(void, DrawFarContent);
    FUNCTION().PUBLIC().SIGNATURE(void, ClearRows);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(float, RowHeight, const Row&, float);
    FUNCTION().PUBLIC().SIGNATURE(String, GetString, const String&, const String&);
    FUNCTION().PUBLIC().SIGNATURE(float, GetNumber, const String&, float);
    FUNCTION().PUBLIC().SIGNATURE(bool, GetBool, const String&, bool);
    FUNCTION().PUBLIC().SIGNATURE(void, SetString, const String&, const String&, bool);
    FUNCTION().PUBLIC().SIGNATURE(void, SetNumber, const String&, float, bool);
    FUNCTION().PUBLIC().SIGNATURE(void, SetBool, const String&, bool, bool);
    FUNCTION().PUBLIC().SIGNATURE(void, Notify, const String&, bool);
    FUNCTION().PUBLIC().SIGNATURE(void, RebuildBody);
    FUNCTION().PUBLIC().SIGNATURE(PipelineValue, GetOutput);
    FUNCTION().PUBLIC().SIGNATURE(Ref<Bitmap>, GetOutputBitmap);
    FUNCTION().PUBLIC().SIGNATURE(Ref<Bitmap>, GetSourceOutputBitmap);
    FUNCTION().PUBLIC().SIGNATURE(PipelineValue, GetInput, const String&);
    FUNCTION().PUBLIC().SIGNATURE(Ref<DropDown>, AddModelRow, const Vector<String>&, const String&);
    FUNCTION().PUBLIC().SIGNATURE(Ref<DropDown>, AddSelectRow, const String&, const String&, const Vector<String>&, const String&);
    FUNCTION().PUBLIC().SIGNATURE(Ref<EditBox>, AddTextArea, const String&, const String&, float);
    FUNCTION().PUBLIC().SIGNATURE(Ref<EditBox>, AddTextRow, const String&, const String&, const String&);
    FUNCTION().PUBLIC().SIGNATURE(Ref<Toggle>, AddCheckbox, const String&, const String&, bool);
    FUNCTION().PUBLIC().SIGNATURE(Ref<PipelineSlider>, AddSlider, const String&, const String&, float, float, float, float, const String&);
    FUNCTION().PUBLIC().SIGNATURE(Ref<PipelineColorField>, AddColor, const String&, const String&, const String&);
    FUNCTION().PUBLIC().SIGNATURE(void, AddSegmented, const String&, _tmp1, const String&);
    FUNCTION().PUBLIC().SIGNATURE(void, AddMutedLine, const String&, float);
    FUNCTION().PUBLIC().SIGNATURE(void, AddSeedRow);
    FUNCTION().PUBLIC().SIGNATURE(void, AddTransparencyBlock);
    FUNCTION().PUBLIC().SIGNATURE(void, AddResultImage, const String&, float);
    FUNCTION().PUBLIC().SIGNATURE(void, AddResultText, const String&, float);
    FUNCTION().PUBLIC().SIGNATURE(void, AddResultAudio, const String&);
    FUNCTION().PUBLIC().SIGNATURE(void, AddResultVideo, const String&, float);
    FUNCTION().PUBLIC().SIGNATURE(void, AddCropSection, const String&, float);
    FUNCTION().PUBLIC().SIGNATURE(void, AddFilePicker, const String&, const Vector<String>&, bool);
}
END_META;

CLASS_BASES_META(Editor::PipelineCropEditor)
{
    BASE_CLASS(Editor::PipelineImageView);
}
END_META;
CLASS_FIELDS_META(Editor::PipelineCropEditor)
{
    FIELD().PUBLIC().NAME(onCropChanged);
    FIELD().PROTECTED().NAME(mNode);
    FIELD().PROTECTED().NAME(mKey);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mCropEnabled);
    FIELD().PROTECTED().NAME(mFrame);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mSyncing);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mFrameDragging);
}
END_META;
CLASS_METHODS_META(Editor::PipelineCropEditor)
{

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().SIGNATURE(void, SetNode, const Ref<PipelineNode>&, const String&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetCropEnabled, bool);
    FUNCTION().PUBLIC().SIGNATURE(void, Draw);
    FUNCTION().PROTECTED().SIGNATURE(void, SyncFrameFromConfig);
    FUNCTION().PROTECTED().SIGNATURE(void, OnFrameTransformed, const Basis&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnFrameCompleted);
}
END_META;
// --- END META ---
