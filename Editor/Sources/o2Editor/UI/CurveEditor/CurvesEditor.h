#pragma once

#include "o2/Assets/Types/ImageAsset.h"
#include "o2/Render/Camera.h"
#include "o2/Render/Font.h"
#include "o2/Render/Mesh.h"
#include "o2/Utils/Editor/DragHandle.h"
#include "o2/Utils/Editor/FrameHandles.h"
#include "o2/Utils/Math/Curve.h"
#include "o2Editor/Actions/ActionsList.h"
#include "o2Editor/Actions/IAction.h"
#include "o2Editor/UI/FrameScrollView.h"

using namespace o2;

namespace o2
{
    class ContextMenu;
    class EditBox;
    class Window;
}

namespace Editor
{
    // ---------------------
    // Curves editing widget
    // ---------------------
    class CurvesEditor : public FrameScrollView, public SelectableDragHandlesGroup
    {
    public:
        Ref<ActionsList> actionsListDelegate; // Actions fall down list. When it is null, editor uses local actions list

    public:
        // Default constructor
        CurvesEditor(RefCounter* refCounter);

        // Copy-constructor
        CurvesEditor(RefCounter* refCounter, const CurvesEditor& other);

        // Destructor
        ~CurvesEditor();

        // Copy-operator
        CurvesEditor& operator=(const CurvesEditor& other);

        // Draws widget, updates render target 
        void Draw() override;

        // Updates drawables, states and widget
        void Update(float dt) override;

        // Returns list of curves
        Map<String, Ref<Curve>> GetCurves() const;

        // Adds editing curve with color. If color is default it will be randomized
        void AddCurve(const String& id, const Ref<Curve>& curve, const Color4& color = Color4(44, 62, 80));

        // Removed curve from editing
        void RemoveCurve(const Ref<Curve>& curve);

        // Removed curve from editing
        void RemoveCurve(const String& id);

        // Removes all editing curves
        void RemoveAllCurves();

        // Sets curve color
        void SetCurveColor(const Ref<Curve>& curve, const Color4& color);

        // Sets selection rectangle sprite image
        void SetSelectionSpriteImage(const AssetRef<ImageAsset>& image);

        // Sets captions text font
        void SetTextFont(const Ref<Font>& font);

        // Sets captions offsets 
        void SetTextBorder(const BorderF& border);

        // Sets main key handle images
        void SetMainHandleImages(const AssetRef<ImageAsset>& regular, const AssetRef<ImageAsset>& hover, const AssetRef<ImageAsset>& pressed,
                                 const AssetRef<ImageAsset>& selected);

        // Sets support key handle images
        void SetSupportHandleImages(const AssetRef<ImageAsset>& regular, const AssetRef<ImageAsset>& hover, const AssetRef<ImageAsset>& pressed,
                                    const AssetRef<ImageAsset>& selected);

        // Sets range key handle images
        void SetRangeHandleImages(const AssetRef<ImageAsset>& regular, const AssetRef<ImageAsset>& hover, const AssetRef<ImageAsset>& pressed,
                                  const AssetRef<ImageAsset>& selected);

        // Enables curves scale adjusting. When it is true, all curves adopts their size to be in the same view range
        void SetAdjustCurvesScale(bool enable);

        // Updates layout
        void UpdateSelfTransform() override;

        // Returns context menu
        const Ref<ContextMenu>& GetContextMenu() const;

        // Dynamic cast to RefCounterable via Component
        static Ref<RefCounterable> CastToRefCounterable(const Ref<CurvesEditor>& ref);

        SERIALIZABLE(CurvesEditor);
        CLONEABLE_REF(CurvesEditor);

    public:
        struct CurveInfo;

        // --------------------------------------
        // Drag handle specified to curves editor
        // --------------------------------------
        struct CurveHandle : public DragHandle
        {
            Ref<CurveInfo> curveInfo; // Curve info

        public:
            // Default constructor
            CurveHandle();

            // Constructor with views
            CurveHandle(const Ref<Sprite>& regular, const Ref<Sprite>& hover = nullptr, const Ref<Sprite>& pressed = nullptr,
                        const Ref<Sprite>& selected = nullptr, const Ref<Sprite>& selectedHovered = nullptr, const Ref<Sprite>& selectedPressed = nullptr);

            // Copy-constructor
            CurveHandle(const CurveHandle& other);

            // Destructor
            ~CurveHandle() override;

            // Copy-operator
            CurveHandle& operator=(const CurveHandle& other);

            // Returns handle position with curve view transformations
            Vec2F GetLocalPosition() const;

            // Converts local to view position (transformed by curve view)
            Vec2F LocalToCurveView(const Vec2F& point) const;

            // Converts view position (transformed by curve view) to local
            Vec2F CurveViewToLocal(const Vec2F& point) const;

            // Converts point from screen to local space
            Vec2F ScreenToLocal(const Vec2F& point) override;

            // Converts point from local to screen space
            Vec2F LocalToScreen(const Vec2F& point) override;

            SERIALIZABLE(CurveHandle);
        };

        // ------------------------------------------------------
        // Curve point handles. Contains main and support handles
        // ------------------------------------------------------
        struct KeyHandles: public RefCounterable
        {
            WeakRef<CurvesEditor> curveEditor; // Owner curves editor

            Ref<CurveHandle> mainHandle;         // Main point handle
            Ref<CurveHandle> leftSupportHandle;  // Left support handle
            Ref<CurveHandle> rightSupportHandle; // Right support handle
            Ref<CurveHandle> topRangeHandle;     // Top range handle
            Ref<CurveHandle> bottomRangeHandle;  // Bottom range handle

            int    curveKeyIdx = 0; // Curve key index
            UInt64 curveKeyUid;     // Curve key unique id

        public:
            // Default constructor
            KeyHandles() {}

            // Constructor
            KeyHandles(const CurveHandle& mainSample, const CurveHandle& supportSample, 
                       const CurveHandle& rangeSample, const Ref<CurvesEditor>& editor, const Color4& color);

            // Draws handles
            void Draw(const RectF& camRect);

            // Returns true if some handle is selected
            bool IsSomeHandleSelected() const;
        };

        // ----------------------------------------------------------
        // Editing curve info. Contains curve, handles and other data
        // ----------------------------------------------------------
        struct CurveInfo : public RefCounterable
        {
            WeakRef<CurvesEditor> editor; // Owner curves editor

            String     curveId; // Curve id
            Ref<Curve> curve;   // Editing curve

            Vector<Ref<KeyHandles>> handles; // Curve key handles

            Vector<Pair<Vec2F, Vec2F>> approximatedPoints; // Cached approximated curve points

            Color4 color; // Curve draw color

            Vec2F viewScale;  // Adjust curve normalized view scale
            Vec2F viewOffset; // Adjust curve normalized view offset

            bool disableChangesHandling = false; // Disable changes handling

        public:
            // Default constructor
            CurveInfo();

            // Destructor
            ~CurveInfo();

            // Updates handles positions and turns on/off support handles
            void UpdateHandles();

            // Updates approximated points
            void UpdateApproximatedPoints();

            // Adjusts curve view scale for normalized view
            void AdjustScale();

            // Called when curve changed, updates handles and approximated points
            void OnCurveChanged();

            // Begins manual curve change
            void BeginCurveManualChange();

            // Ends manual curve change
            void CompleteCurveManualChange();
        };

        // ----------------------------------------------
        // Curve copying info. Contains curve id and keys
        // ----------------------------------------------
        class CurveCopyInfo : public ISerializable, public RefCounterable
        {
        public:
            String             curveId; // Curve id @SERIALIZABLE
            Vector<Curve::Key> keys;    // List of copying keys @SERIALIZABLE

            SERIALIZABLE(CurveCopyInfo);
        };

        // -----------------------------------------------
        // Selected handles info. Contains index and flags
        // -----------------------------------------------
        struct SelectedHandlesInfo
        {
            int  index = 0;                  // Selected handle index
            bool mainHandle = false;         // Main handle selected
            bool leftSupportHandle = false;  // Left support handle selected
            bool rightSupportHandle = false; // Right support handle selected

            bool operator==(const SelectedHandlesInfo& other) const;
        };

        // -------------------------------------------------------------
        // Curve keys info. Contains curve id, keys and selected handles
        // -------------------------------------------------------------
        struct CurveKeysInfo
        {
            String curveId; // Curve id

            Vector<Curve::Key>          keys;            // Curve keys
            Vector<SelectedHandlesInfo> selectedHandles; // Selected handles

            bool operator==(const CurveKeysInfo& other) const;
        };

    protected:
        Ref<ContextMenu> mContextMenu; // Context menu for editing keys properties, copying, pasting and other

        CurveHandle    mMainHandleSample;      // Main handle sample, uses to copy sprites @SERIALIZABLE
        CurveHandle    mSupportHandleSample;   // Support handle sample, uses to copy sprites @SERIALIZABLE
        CurveHandle    mRangeHandleSample;  // Top range handle sample, uses to copy sprites @SERIALIZABLE
        Ref<CurveInfo> mHandleSamplesStubInfo; // Empty curve info, used int handles samples

        Vector<Ref<CurveInfo>> mCurves; // Editing curves infos list

        Vector<Ref<CurveHandle>>        mSupportHandles;                                            // Support points handles list
        Ref<SelectableDragHandlesGroup> mSupportHandlesGroup = mmake<SelectableDragHandlesGroup>(); // Support points handles selection group. They are must be selectable separately from main handles

        Vector<Ref<CurveHandle>> mSelectingHandlesBuf; // Potentially selecting handles while selecting

        Ref<Sprite> mSelectionSprite; // Selection sprite @SERIALIZABLE

        bool mAdjustCurvesScale = true; // When it is true, all curves adopts their size to be in the same view range

        Ref<Font> mTextFont;   // Captions text font @SERIALIZABLE
        Ref<Text> mTextLeft;   // Captions text drawable at left border
        Ref<Text> mTextRight;  // Captions text drawable at right border
        Ref<Text> mTextTop;    // Captions text drawable at top border
        Ref<Text> mTextBottom; // Captions text drawable at bottom border
        BorderF   mTextBorder; // Captions offsets from border

        Ref<Mesh> mCurvesMesh; // Mesh for drawing curves ranges

        Vec2F mSelectingPressedPoint; // Point, where cursor was pressed, selection starts here, in local space

        Ref<FrameHandles> mTransformFrame;                // Keys transformation frame
        bool              mTransformFrameVisible = false; // Is transform frame visible. it visible when 2 or more main handles was selected
        Basis             mTransformFrameBasis;           // Basis of transform frame in screen space

        bool mIsViewScrolling = false; // Is scrolling view at this time

        bool mNeedAdjustView = false; // True when need to adjust view scale. This works in update

        Vector<CurveKeysInfo> mBeforeTransformKeys; // Stored selected keys before handles transformed

        ActionsList mActionsList; // Local actions list. It uses when actionFallDown is null

    protected:
        // Called when visible was changed. Sets context menu items priority
        void OnEnabled() override;

        // Called when visible was changed. Sets context menu items priority
        void OnDisabled() override;

        // Called when scrolling
        void OnScrolled(float scroll) override;

        // Searches curve by id
        Ref<Curve> FindCurve(const String& id);

        // Initializes context menu items
        void InitializeContextMenu();

        // Initializes text drawables by font and sets aligning
        void InitializeTextDrawables();

        // Recalculates view area by curves approximated points
        void RecalculateViewArea();

        // Redraws content into render target
        void RedrawContent() override;

        // Draws grid and captions
        void DrawGrid() override;

        // Draws curves
        void DrawCurves();

        // Draws handles
        void DrawHandles();

        // Draw selection sprite
        void DrawSelection();

        // Draws transformation frame
        void DrawTransformFrame();

        // Adds curve key main and support handles and initializes them
        void AddCurveKeyHandles(const Ref<CurveInfo>& info, int keyId);

        // Removes curve key handles
        void RemoveCurveKeyHandles(const Ref<CurveInfo>& info, int keyId);

        // Called when one of main curve key handles was moved. Updates graphics and handles
        void OnCurveKeyMainHandleDragged(const Ref<CurveInfo>& info, const Ref<KeyHandles>& handles, const Vec2F& position);

        // Called when main curve key handle was pressed, calls OnTransformBegin and checks to add value range. Updates graphics and handles
        void OnCurveKeyMainHandlePressed(const Ref<CurveInfo>& info, const Ref<KeyHandles>& handles);

        // Called when one of left support curve key handles was moved. Updates graphics and handles
        void OnCurveKeyLeftSupportHandleDragged(const Ref<CurveInfo>& info, const Ref<KeyHandles>& handles, const Vec2F& position);

        // Called when one of right support curve key handles was moved. Updates graphics and handles
        void OnCurveKeyRightSupportHandleDragged(const Ref<CurveInfo>& info, const Ref<KeyHandles>& handles, const Vec2F& position);

        // Called when one of top range curve key handles was moved. Updates graphics and handles
        void OnCurveKeyTopRangeHandleDragged(const Ref<CurveInfo>& info, const Ref<KeyHandles>& handles, const Vec2F& position);

        // Called when one of bottom range curve key handles was moved. Updates graphics and handles
        void OnCurveKeyBottomRangeHandleDragged(const Ref<CurveInfo>& info, const Ref<KeyHandles>& handles, const Vec2F& position);

        // Checks left support handle constrains and returns filtered position
        Vec2F CheckLeftSupportHandlePosition(const Ref<CurveInfo>& info, const Ref<KeyHandles>& handles, const Vec2F& position);

        // Checks right support handle constrains and returns filtered position
        Vec2F CheckRightSupportHandlePosition(const Ref<CurveInfo>& info, const Ref<KeyHandles>& handles, const Vec2F& position);

        // Checks top range handle constrains and returns filtered position
        Vec2F CheckTopRangeHandlePosition(const Ref<CurveInfo>& info, const Ref<KeyHandles>& handles, const Vec2F& position);

        // Checks bottom range handle constrains and returns filtered position
        Vec2F CheckBottomRangeHandlePosition(const Ref<CurveInfo>& info, const Ref<KeyHandles>& handles, const Vec2F& position);

        // Smooths key support points and updates handles
        void SmoothKey(const Ref<CurveInfo>& info, int idx);

        // Called when cursor double clicked, adds new point in curve
        void OnCursorDblClicked(const Input::Cursor& cursor) override;

        // Called when cursor pressed on this
        void OnCursorPressed(const Input::Cursor& cursor) override;

        // Called when cursor released (only when cursor pressed this at previous time)
        void OnCursorReleased(const Input::Cursor& cursor) override;

        // Called when cursor stay down during frame
        void OnCursorStillDown(const Input::Cursor& cursor) override;

        // Called when right mouse button stay down on this, overriding from scroll view to call context menu
        void OnCursorRightMouseStayDown(const Input::Cursor& cursor) override;

        // Called when right mouse button was released (only when right mouse button pressed this at previous time), overriding from scroll view to call context menu
        void OnCursorRightMouseReleased(const Input::Cursor& cursor) override;

        // Called when selectable draggable handle was released
        void OnHandleCursorReleased(const Ref<DragHandle>& handle, const Input::Cursor& cursor) override;

        // Called when selectable handle was began to drag
        void OnHandleBeganDragging(const Ref<DragHandle>& handle) override;

        // Called when selectable handle moved, moves all selected handles position
        void OnHandleMoved(const Ref<DragHandle>& handle, const Vec2F& cursorPos) override;

        // Called when selectable handle completed changing
        void OnHandleCompletedChange(const Ref<DragHandle>& handle) override;

        // Called when selection is changed - some handle was added or removed from selection
        void OnSelectionChanged() override;

        // Checks supports handles visibility
        void CheckHandlesVisible();

        // Updates transformation frame by selected handles
        void UpdateTransformFrame();

        // Returns is transform frame visible. it will be visible when 2 or more main handles was selected
        bool IsTransformFrameVisible() const;

        // Sets all selected keys supports type
        void SetSelectedKeysSupportsType(Curve::Key::Type type);

        // Called when transform frame was transformed
        void OnTransformFrameTransformed(const Basis& basis);

        // Called when beginning transforming some handles. Stores selected keys before changing
        void OnTransformBegin();

        // Called when transform completed. Checks that selected keys changed, creates action for undo/redo
        void OnTransformCompleted();

        // Transforms point from local to curve view coords
        Vec2F LocalToCurveView(const Vec2F& point, const Vec2F& viewScale, const Vec2F& viewOffset) const;

        // Transforms point from curve view to local coords
        Vec2F CurveViewToLocal(const Vec2F& point, const Vec2F& viewScale, const Vec2F& viewOffset) const;

        // Stores undo action, clears redo actions
        void DoneAction(const Ref<IAction>& action);


        // On context menu edit pressed. Shows key edit window
        void OnEditPressed();

        // Sets handles types for selected keys and checks context menu items
        void SetHandlesTypesChecked(Curve::Key::Type type);

        // On context menu copy pressed. Stores keys into buffer
        void OnCopyPressed();

        // On context menu cut pressed. Stores keys into buffer and removes them
        void OnCutPressed();

        // On context menu paste pressed. Inserts keys from buffer at cursor position
        void OnPastePressed();

        // On context menu delete pressed. Deletes selected keys
        void OnDeletePressed();

        // On context menu insert pressed. Inserts keys from buffer and moves keys to right
        void OnInsertPressed();

        // On context menu undo pressed. Reverts previous action and puts into actions stack
        void OnUndoPressed();

        // On context menu redo pressed. Restores action from stack
        void OnRedoPressed(); 
        
        REF_COUNTERABLE_IMPL(FrameScrollView, SelectableDragHandlesGroup);

        friend class CurveAddKeysAction;
        friend class CurveDeleteKeysAction;
        friend class CurveKeysChangeAction;
    };
}
// --- META ---

CLASS_BASES_META(Editor::CurvesEditor)
{
    BASE_CLASS(Editor::FrameScrollView);
    BASE_CLASS(o2::SelectableDragHandlesGroup);
}
END_META;
CLASS_FIELDS_META(Editor::CurvesEditor)
{
    FIELD().PUBLIC().NAME(actionsListDelegate);
    FIELD().PROTECTED().NAME(mContextMenu);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().NAME(mMainHandleSample);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().NAME(mSupportHandleSample);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().NAME(mRangeHandleSample);
    FIELD().PROTECTED().NAME(mHandleSamplesStubInfo);
    FIELD().PROTECTED().NAME(mCurves);
    FIELD().PROTECTED().NAME(mSupportHandles);
    FIELD().PROTECTED().DEFAULT_VALUE(mmake<SelectableDragHandlesGroup>()).NAME(mSupportHandlesGroup);
    FIELD().PROTECTED().NAME(mSelectingHandlesBuf);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().NAME(mSelectionSprite);
    FIELD().PROTECTED().DEFAULT_VALUE(true).NAME(mAdjustCurvesScale);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().NAME(mTextFont);
    FIELD().PROTECTED().NAME(mTextLeft);
    FIELD().PROTECTED().NAME(mTextRight);
    FIELD().PROTECTED().NAME(mTextTop);
    FIELD().PROTECTED().NAME(mTextBottom);
    FIELD().PROTECTED().NAME(mTextBorder);
    FIELD().PROTECTED().NAME(mCurvesMesh);
    FIELD().PROTECTED().NAME(mSelectingPressedPoint);
    FIELD().PROTECTED().NAME(mTransformFrame);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mTransformFrameVisible);
    FIELD().PROTECTED().NAME(mTransformFrameBasis);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mIsViewScrolling);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mNeedAdjustView);
    FIELD().PROTECTED().NAME(mBeforeTransformKeys);
    FIELD().PROTECTED().NAME(mActionsList);
}
END_META;
CLASS_METHODS_META(Editor::CurvesEditor)
{

    typedef Map<String, Ref<Curve>> _tmp1;

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*, const CurvesEditor&);
    FUNCTION().PUBLIC().SIGNATURE(void, Draw);
    FUNCTION().PUBLIC().SIGNATURE(void, Update, float);
    FUNCTION().PUBLIC().SIGNATURE(_tmp1, GetCurves);
    FUNCTION().PUBLIC().SIGNATURE(void, AddCurve, const String&, const Ref<Curve>&, const Color4&);
    FUNCTION().PUBLIC().SIGNATURE(void, RemoveCurve, const Ref<Curve>&);
    FUNCTION().PUBLIC().SIGNATURE(void, RemoveCurve, const String&);
    FUNCTION().PUBLIC().SIGNATURE(void, RemoveAllCurves);
    FUNCTION().PUBLIC().SIGNATURE(void, SetCurveColor, const Ref<Curve>&, const Color4&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetSelectionSpriteImage, const AssetRef<ImageAsset>&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetTextFont, const Ref<Font>&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetTextBorder, const BorderF&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetMainHandleImages, const AssetRef<ImageAsset>&, const AssetRef<ImageAsset>&, const AssetRef<ImageAsset>&, const AssetRef<ImageAsset>&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetSupportHandleImages, const AssetRef<ImageAsset>&, const AssetRef<ImageAsset>&, const AssetRef<ImageAsset>&, const AssetRef<ImageAsset>&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetRangeHandleImages, const AssetRef<ImageAsset>&, const AssetRef<ImageAsset>&, const AssetRef<ImageAsset>&, const AssetRef<ImageAsset>&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetAdjustCurvesScale, bool);
    FUNCTION().PUBLIC().SIGNATURE(void, UpdateSelfTransform);
    FUNCTION().PUBLIC().SIGNATURE(const Ref<ContextMenu>&, GetContextMenu);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(Ref<RefCounterable>, CastToRefCounterable, const Ref<CurvesEditor>&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnEnabled);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDisabled);
    FUNCTION().PROTECTED().SIGNATURE(void, OnScrolled, float);
    FUNCTION().PROTECTED().SIGNATURE(Ref<Curve>, FindCurve, const String&);
    FUNCTION().PROTECTED().SIGNATURE(void, InitializeContextMenu);
    FUNCTION().PROTECTED().SIGNATURE(void, InitializeTextDrawables);
    FUNCTION().PROTECTED().SIGNATURE(void, RecalculateViewArea);
    FUNCTION().PROTECTED().SIGNATURE(void, RedrawContent);
    FUNCTION().PROTECTED().SIGNATURE(void, DrawGrid);
    FUNCTION().PROTECTED().SIGNATURE(void, DrawCurves);
    FUNCTION().PROTECTED().SIGNATURE(void, DrawHandles);
    FUNCTION().PROTECTED().SIGNATURE(void, DrawSelection);
    FUNCTION().PROTECTED().SIGNATURE(void, DrawTransformFrame);
    FUNCTION().PROTECTED().SIGNATURE(void, AddCurveKeyHandles, const Ref<CurveInfo>&, int);
    FUNCTION().PROTECTED().SIGNATURE(void, RemoveCurveKeyHandles, const Ref<CurveInfo>&, int);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCurveKeyMainHandleDragged, const Ref<CurveInfo>&, const Ref<KeyHandles>&, const Vec2F&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCurveKeyMainHandlePressed, const Ref<CurveInfo>&, const Ref<KeyHandles>&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCurveKeyLeftSupportHandleDragged, const Ref<CurveInfo>&, const Ref<KeyHandles>&, const Vec2F&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCurveKeyRightSupportHandleDragged, const Ref<CurveInfo>&, const Ref<KeyHandles>&, const Vec2F&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCurveKeyTopRangeHandleDragged, const Ref<CurveInfo>&, const Ref<KeyHandles>&, const Vec2F&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCurveKeyBottomRangeHandleDragged, const Ref<CurveInfo>&, const Ref<KeyHandles>&, const Vec2F&);
    FUNCTION().PROTECTED().SIGNATURE(Vec2F, CheckLeftSupportHandlePosition, const Ref<CurveInfo>&, const Ref<KeyHandles>&, const Vec2F&);
    FUNCTION().PROTECTED().SIGNATURE(Vec2F, CheckRightSupportHandlePosition, const Ref<CurveInfo>&, const Ref<KeyHandles>&, const Vec2F&);
    FUNCTION().PROTECTED().SIGNATURE(Vec2F, CheckTopRangeHandlePosition, const Ref<CurveInfo>&, const Ref<KeyHandles>&, const Vec2F&);
    FUNCTION().PROTECTED().SIGNATURE(Vec2F, CheckBottomRangeHandlePosition, const Ref<CurveInfo>&, const Ref<KeyHandles>&, const Vec2F&);
    FUNCTION().PROTECTED().SIGNATURE(void, SmoothKey, const Ref<CurveInfo>&, int);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorDblClicked, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorPressed, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorReleased, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorStillDown, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorRightMouseStayDown, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorRightMouseReleased, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnHandleCursorReleased, const Ref<DragHandle>&, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnHandleBeganDragging, const Ref<DragHandle>&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnHandleMoved, const Ref<DragHandle>&, const Vec2F&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnHandleCompletedChange, const Ref<DragHandle>&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnSelectionChanged);
    FUNCTION().PROTECTED().SIGNATURE(void, CheckHandlesVisible);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateTransformFrame);
    FUNCTION().PROTECTED().SIGNATURE(bool, IsTransformFrameVisible);
    FUNCTION().PROTECTED().SIGNATURE(void, SetSelectedKeysSupportsType, Curve::Key::Type);
    FUNCTION().PROTECTED().SIGNATURE(void, OnTransformFrameTransformed, const Basis&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnTransformBegin);
    FUNCTION().PROTECTED().SIGNATURE(void, OnTransformCompleted);
    FUNCTION().PROTECTED().SIGNATURE(Vec2F, LocalToCurveView, const Vec2F&, const Vec2F&, const Vec2F&);
    FUNCTION().PROTECTED().SIGNATURE(Vec2F, CurveViewToLocal, const Vec2F&, const Vec2F&, const Vec2F&);
    FUNCTION().PROTECTED().SIGNATURE(void, DoneAction, const Ref<IAction>&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnEditPressed);
    FUNCTION().PROTECTED().SIGNATURE(void, SetHandlesTypesChecked, Curve::Key::Type);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCopyPressed);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCutPressed);
    FUNCTION().PROTECTED().SIGNATURE(void, OnPastePressed);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDeletePressed);
    FUNCTION().PROTECTED().SIGNATURE(void, OnInsertPressed);
    FUNCTION().PROTECTED().SIGNATURE(void, OnUndoPressed);
    FUNCTION().PROTECTED().SIGNATURE(void, OnRedoPressed);
}
END_META;

CLASS_BASES_META(Editor::CurvesEditor::CurveHandle)
{
    BASE_CLASS(o2::DragHandle);
}
END_META;
CLASS_FIELDS_META(Editor::CurvesEditor::CurveHandle)
{
    FIELD().PUBLIC().NAME(curveInfo);
}
END_META;
CLASS_METHODS_META(Editor::CurvesEditor::CurveHandle)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(const Ref<Sprite>&, const Ref<Sprite>&, const Ref<Sprite>&, const Ref<Sprite>&, const Ref<Sprite>&, const Ref<Sprite>&);
    FUNCTION().PUBLIC().CONSTRUCTOR(const CurveHandle&);
    FUNCTION().PUBLIC().SIGNATURE(Vec2F, GetLocalPosition);
    FUNCTION().PUBLIC().SIGNATURE(Vec2F, LocalToCurveView, const Vec2F&);
    FUNCTION().PUBLIC().SIGNATURE(Vec2F, CurveViewToLocal, const Vec2F&);
    FUNCTION().PUBLIC().SIGNATURE(Vec2F, ScreenToLocal, const Vec2F&);
    FUNCTION().PUBLIC().SIGNATURE(Vec2F, LocalToScreen, const Vec2F&);
}
END_META;

CLASS_BASES_META(Editor::CurvesEditor::CurveCopyInfo)
{
    BASE_CLASS(o2::ISerializable);
    BASE_CLASS(o2::RefCounterable);
}
END_META;
CLASS_FIELDS_META(Editor::CurvesEditor::CurveCopyInfo)
{
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().NAME(curveId);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().NAME(keys);
}
END_META;
CLASS_METHODS_META(Editor::CurvesEditor::CurveCopyInfo)
{
}
END_META;
// --- END META ---
