#pragma once

#include "o2/Events/CursorAreaEventsListener.h"
#include "o2/Utils/Editor/DragHandle.h"
#include "o2Editor/Windows/AnimationWindow/TrackControls/AnimationKeyDragHandle.h"

namespace o2
{
    class AnimationClip;
    class ContextMenu;
    class Sprite;
}

using namespace o2;

namespace Editor
{
    FORWARD_CLASS_REF(AnimationWindow);
    FORWARD_CLASS_REF(ITrackControl);

    // -------------------------------------------
    // Handles sheet, manages selection of handles
    // -------------------------------------------
    class KeyHandlesSheet : public Widget, public SelectableDragHandlesGroup, public CursorAreaEventsListener
    {
    public:
        // Default constructor
        explicit KeyHandlesSheet(RefCounter* refCounter);

        // Copy-constructor
        KeyHandlesSheet(RefCounter* refCounter, const KeyHandlesSheet& other);

        // Destructor
        ~KeyHandlesSheet();

        // Copy-operator
        KeyHandlesSheet& operator=(const KeyHandlesSheet& other);

        // Sets animation. Used for batch change of keys
        void SetAnimation(const Ref<AnimationClip>& animation);

        // Updates selection frame
        void Update(float dt) override;

        // Draws selection
        void Draw() override;

		// Draws selection frame
		void DrawSelectionFrame();

        // Updates draw order for correct handles and sheet input processing
        void UpdateInputDrawOrder();

        // Returns true if point is in this object
        bool IsUnderPoint(const Vec2F& point) override;

        // Registers animation track track control
        void RegTrackControl(const Ref<ITrackControl>& trackControl, const std::string& path);

        // Unregisters animation track track control
        void UnregTrackControl(const Ref<ITrackControl>& trackControl);

        // Unregisters all tracks controls
        void UnregAllTrackControls();

        // Adds selectable handle to group
        void AddHandle(const Ref<DragHandle>& handle) override;

        // Removes selectable handle from group
        void RemoveHandle(DragHandle* handle) override;

        // Adds handles to group and binds them to it
        void AddHandles(const Vector<Ref<DragHandle>>& handles) override;

        // Unbinds and removes handles from group; faster than removing one by one
        void RemoveHandles(const Vector<Ref<DragHandle>>& handles) override;

        // Sets selected keys handles
        void SetSelectedKeys(const Map<String, Vector<UInt64>>& keys);

        // Returns list of selected keys handles
        Map<String, Vector<UInt64>> GetSelectedKeys() const;

        // Removes keys
        void DeleteKeys(const Map<String, Vector<UInt64>>& keys, bool createAction = true);

        // Returns context menu
        const Ref<ContextMenu>& GetContextMenu() const;

        // Returns create menu category in editor
        static String GetCreateMenuCategory();

        // Dynamic cast to RefCounterable via Widget
        static Ref<RefCounterable> CastToRefCounterable(const Ref<KeyHandlesSheet>& ref);

        SERIALIZABLE(KeyHandlesSheet);
        CLONEABLE_REF(KeyHandlesSheet);

    private:
        RectF mSelectionFrameOffsets = RectF(-6, -4, 8, 3);
        RectF mSelectionFrameCursorOffsets = RectF(-2, -3, 2, 2);

        WeakRef<AnimationWindow> mAnimationWindow; // Animation window

        Vector<Ref<ITrackControl>>               mTrackControls;    // List of actual track controls
        Vector<Pair<String, Ref<ITrackControl>>> mTrackControlsMap; // Map of actual track controls, key is Animation track path

        Map<Ref<IAnimationTrack>, Vector<Ref<AnimationKeyDragHandle>>> mHandlesGroups; // All handles grouped by Animation track, used for fast searching handles for same Animation track

        Ref<ContextMenu> mContextMenu; // Keys context menu
        Vec2F            mContextMenuPressPoint; // Cursor position when right button were clicked. When trying to show context menu checking delta between current cursor position and this 

        bool mNeedUpdateSelectionFrame = false; // True when selection frame required to update

        bool mHandleHasMoved = false; // it is true when some handle was selected and moved, resets on handle pressing

        Ref<Sprite> mSelectionFrame; // Selected handles frame drawing sprite
        RectF       mSelectionRect;  // Current selected handles rectangle. The right and left is minimum and maximum handles positions, top and bottom is minimum and maximum handles lines

        Vec2F   mBeginSelectPoint;         // Begin frame selection point, where x is position on timeline, y is line number
        bool    mIsFrameSelecting = false; // It is true when user selection by frame now

        Vector<Ref<DragHandle>> mBeginSelectHandles; // handles list, that were selected before frame selecting

        Ref<DragHandle> mLeftFrameDragHandle;   // Left frame border drag handle, resizing selected handles rect
        Ref<DragHandle> mRightFrameDragHandle;  // Right frame border drag handle, resizing selected handles rect
        Ref<DragHandle> mCenterFrameDragHandle; // Center frame drag handle, moves selected handles

        DataDocument mBeforeChangeKeysData; // Serialized keys data before change

    private:
        // Initializes frame handles
        void InitializeHandles();

        // Initializes center handle, that moves selected keys on timeline
        void InitializeCenterHandle();

        // Initializes left handle, that moves selected keys on timeline relative to right selection rect position
        void InitializeLeftHandle();

        // Initializes right handle, that moves selected keys on timeline relative to left selection rect position
        void InitializeRightHandle();

        // Initializes context menu with copy, paste, delete etc
        void InitializeContextMenu();

        // Updates selection rectangle and drawing sprite
        void UpdateSelectionFrame();

        // Serialized keys into data
        void SerializeKeys(DataValue& data, const Map<String, Vector<UInt64>>& keys, float relativeTime);

        // Deserializes keys from data 
        void DeserializeKeys(const DataValue& data, Map<String, Vector<UInt64>>& keys, float relativeTime, bool generateNewUid = true);

        // Copies selected keys into buffer
        void CopyKeys();

        // Inserts keys from buffer under cursor
        void PasteKeys();

        // Called when selection is changed - some handle was added or removed from selection
        // Updating selection frame
        void OnSelectionChanged() override;

        // Called when selectable draggable handle was pressed, sends to track control that drag has began
        void OnHandleCursorPressed(const Ref<DragHandle>& handle, const Input::Cursor& cursor) override;

        // Called when selectable draggable handle was released, sends to track control that drag has completed
        void OnHandleCursorReleased(const Ref<DragHandle>& handle, const Input::Cursor& cursor) override;

        // Called when selectable handle moved, moves all selected handles position
        // Enables keys batch change
        void OnHandleMoved(const Ref<DragHandle>& handle, const Vec2F& cursorPos) override;

        // Called when cursor pressed on this
        void OnCursorPressed(const Input::Cursor& cursor) override;

        // Called when cursor released (only when cursor pressed this at previous time)
        void OnCursorReleased(const Input::Cursor& cursor) override;

        // Called when cursor pressing was broken (when scrolled scroll area or some other)
        void OnCursorPressBreak(const Input::Cursor& cursor) override;

        // Called when cursor pressed outside this
        void OnCursorPressedOutside(const Input::Cursor& cursor) override;

        // Called when cursor released outside this(only when cursor pressed this at previous time)
        void OnCursorReleasedOutside(const Input::Cursor& cursor) override;

        // Called when cursor stay down during frame
        void OnCursorStillDown(const Input::Cursor& cursor) override;

        // Called when cursor moved on this (or moved outside when this was pressed)
        void OnCursorMoved(const Input::Cursor& cursor) override;

        // Called when cursor enters this object
        void OnCursorEnter(const Input::Cursor& cursor) override;

        // Called when cursor exits this object
        void OnCursorExit(const Input::Cursor& cursor) override;

        // Called when cursor double clicked
        void OnCursorDblClicked(const Input::Cursor& cursor) override;

        // Called when right mouse button was pressed on this
        void OnCursorRightMousePressed(const Input::Cursor& cursor) override;

        // Called when right mouse button stay down on this
        void OnCursorRightMouseStayDown(const Input::Cursor& cursor) override;

        // Called when right mouse button was released (only when right mouse button pressed this at previous time)
        void OnCursorRightMouseReleased(const Input::Cursor& cursor) override;

        // Called when middle mouse button was pressed on this
        void OnCursorMiddleMousePressed(const Input::Cursor& cursor) override;

        // Called when middle mouse button stay down on this
        void OnCursorMiddleMouseStayDown(const Input::Cursor& cursor) override;

        // Called when middle mouse button was released (only when middle mouse button pressed this at previous time)
        void OnCursorMiddleMouseReleased(const Input::Cursor& cursor) override;

        REF_COUNTERABLE_IMPL(Widget, SelectableDragHandlesGroup);

        friend class AnimationWindow;
        friend class AnimationAddKeysAction;
        friend class AnimationDeleteKeysAction;
        friend class AnimationKeysChangeAction;

        template<typename AnimationTrackType>
        friend class KeyFramesTrackControl;
    };
}
// --- META ---

CLASS_BASES_META(Editor::KeyHandlesSheet)
{
    BASE_CLASS(o2::Widget);
    BASE_CLASS(o2::SelectableDragHandlesGroup);
    BASE_CLASS(o2::CursorAreaEventsListener);
}
END_META;
CLASS_FIELDS_META(Editor::KeyHandlesSheet)
{
    FIELD().PRIVATE().DEFAULT_VALUE(RectF(-6, -4, 8, 3)).NAME(mSelectionFrameOffsets);
    FIELD().PRIVATE().DEFAULT_VALUE(RectF(-2, -3, 2, 2)).NAME(mSelectionFrameCursorOffsets);
    FIELD().PRIVATE().NAME(mAnimationWindow);
    FIELD().PRIVATE().NAME(mTrackControls);
    FIELD().PRIVATE().NAME(mTrackControlsMap);
    FIELD().PRIVATE().NAME(mHandlesGroups);
    FIELD().PRIVATE().NAME(mContextMenu);
    FIELD().PRIVATE().NAME(mContextMenuPressPoint);
    FIELD().PRIVATE().DEFAULT_VALUE(false).NAME(mNeedUpdateSelectionFrame);
    FIELD().PRIVATE().DEFAULT_VALUE(false).NAME(mHandleHasMoved);
    FIELD().PRIVATE().NAME(mSelectionFrame);
    FIELD().PRIVATE().NAME(mSelectionRect);
    FIELD().PRIVATE().NAME(mBeginSelectPoint);
    FIELD().PRIVATE().DEFAULT_VALUE(false).NAME(mIsFrameSelecting);
    FIELD().PRIVATE().NAME(mBeginSelectHandles);
    FIELD().PRIVATE().NAME(mLeftFrameDragHandle);
    FIELD().PRIVATE().NAME(mRightFrameDragHandle);
    FIELD().PRIVATE().NAME(mCenterFrameDragHandle);
    FIELD().PRIVATE().NAME(mBeforeChangeKeysData);
}
END_META;
CLASS_METHODS_META(Editor::KeyHandlesSheet)
{

    typedef const Map<String, Vector<UInt64>>& _tmp1;
    typedef Map<String, Vector<UInt64>> _tmp2;
    typedef const Map<String, Vector<UInt64>>& _tmp3;
    typedef const Map<String, Vector<UInt64>>& _tmp4;
    typedef Map<String, Vector<UInt64>>& _tmp5;

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*, const KeyHandlesSheet&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetAnimation, const Ref<AnimationClip>&);
    FUNCTION().PUBLIC().SIGNATURE(void, Update, float);
    FUNCTION().PUBLIC().SIGNATURE(void, Draw);
    FUNCTION().PUBLIC().SIGNATURE(void, DrawSelectionFrame);
    FUNCTION().PUBLIC().SIGNATURE(void, UpdateInputDrawOrder);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsUnderPoint, const Vec2F&);
    FUNCTION().PUBLIC().SIGNATURE(void, RegTrackControl, const Ref<ITrackControl>&, const std::string&);
    FUNCTION().PUBLIC().SIGNATURE(void, UnregTrackControl, const Ref<ITrackControl>&);
    FUNCTION().PUBLIC().SIGNATURE(void, UnregAllTrackControls);
    FUNCTION().PUBLIC().SIGNATURE(void, AddHandle, const Ref<DragHandle>&);
    FUNCTION().PUBLIC().SIGNATURE(void, RemoveHandle, DragHandle*);
    FUNCTION().PUBLIC().SIGNATURE(void, AddHandles, const Vector<Ref<DragHandle>>&);
    FUNCTION().PUBLIC().SIGNATURE(void, RemoveHandles, const Vector<Ref<DragHandle>>&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetSelectedKeys, _tmp1);
    FUNCTION().PUBLIC().SIGNATURE(_tmp2, GetSelectedKeys);
    FUNCTION().PUBLIC().SIGNATURE(void, DeleteKeys, _tmp3, bool);
    FUNCTION().PUBLIC().SIGNATURE(const Ref<ContextMenu>&, GetContextMenu);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetCreateMenuCategory);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(Ref<RefCounterable>, CastToRefCounterable, const Ref<KeyHandlesSheet>&);
    FUNCTION().PRIVATE().SIGNATURE(void, InitializeHandles);
    FUNCTION().PRIVATE().SIGNATURE(void, InitializeCenterHandle);
    FUNCTION().PRIVATE().SIGNATURE(void, InitializeLeftHandle);
    FUNCTION().PRIVATE().SIGNATURE(void, InitializeRightHandle);
    FUNCTION().PRIVATE().SIGNATURE(void, InitializeContextMenu);
    FUNCTION().PRIVATE().SIGNATURE(void, UpdateSelectionFrame);
    FUNCTION().PRIVATE().SIGNATURE(void, SerializeKeys, DataValue&, _tmp4, float);
    FUNCTION().PRIVATE().SIGNATURE(void, DeserializeKeys, const DataValue&, _tmp5, float, bool);
    FUNCTION().PRIVATE().SIGNATURE(void, CopyKeys);
    FUNCTION().PRIVATE().SIGNATURE(void, PasteKeys);
    FUNCTION().PRIVATE().SIGNATURE(void, OnSelectionChanged);
    FUNCTION().PRIVATE().SIGNATURE(void, OnHandleCursorPressed, const Ref<DragHandle>&, const Input::Cursor&);
    FUNCTION().PRIVATE().SIGNATURE(void, OnHandleCursorReleased, const Ref<DragHandle>&, const Input::Cursor&);
    FUNCTION().PRIVATE().SIGNATURE(void, OnHandleMoved, const Ref<DragHandle>&, const Vec2F&);
    FUNCTION().PRIVATE().SIGNATURE(void, OnCursorPressed, const Input::Cursor&);
    FUNCTION().PRIVATE().SIGNATURE(void, OnCursorReleased, const Input::Cursor&);
    FUNCTION().PRIVATE().SIGNATURE(void, OnCursorPressBreak, const Input::Cursor&);
    FUNCTION().PRIVATE().SIGNATURE(void, OnCursorPressedOutside, const Input::Cursor&);
    FUNCTION().PRIVATE().SIGNATURE(void, OnCursorReleasedOutside, const Input::Cursor&);
    FUNCTION().PRIVATE().SIGNATURE(void, OnCursorStillDown, const Input::Cursor&);
    FUNCTION().PRIVATE().SIGNATURE(void, OnCursorMoved, const Input::Cursor&);
    FUNCTION().PRIVATE().SIGNATURE(void, OnCursorEnter, const Input::Cursor&);
    FUNCTION().PRIVATE().SIGNATURE(void, OnCursorExit, const Input::Cursor&);
    FUNCTION().PRIVATE().SIGNATURE(void, OnCursorDblClicked, const Input::Cursor&);
    FUNCTION().PRIVATE().SIGNATURE(void, OnCursorRightMousePressed, const Input::Cursor&);
    FUNCTION().PRIVATE().SIGNATURE(void, OnCursorRightMouseStayDown, const Input::Cursor&);
    FUNCTION().PRIVATE().SIGNATURE(void, OnCursorRightMouseReleased, const Input::Cursor&);
    FUNCTION().PRIVATE().SIGNATURE(void, OnCursorMiddleMousePressed, const Input::Cursor&);
    FUNCTION().PRIVATE().SIGNATURE(void, OnCursorMiddleMouseStayDown, const Input::Cursor&);
    FUNCTION().PRIVATE().SIGNATURE(void, OnCursorMiddleMouseReleased, const Input::Cursor&);
}
END_META;
// --- END META ---
