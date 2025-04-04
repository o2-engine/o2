#pragma once

#include "o2/Application/VKCodes.h"
#include "o2/Events/CursorAreaEventsListener.h"
#include "o2/Render/IDrawable.h"
#include "o2/Scene/UI/Widget.h"
#include "o2/Utils/Function/Function.h"
#include "o2/Utils/Property.h"
#include "o2/Utils/Serialization/Serializable.h"

namespace o2
{
    class IRectDrawable;
    class ISelectableDragHandlesGroup;

    // -----------
    // Drag handle
    // -----------
    class DragHandle: public RefCounterable, public IDrawable, public CursorAreaEventsListener, virtual public ISerializable, virtual public ICloneableRef
    {
    public:
        PROPERTIES(DragHandle);
        PROPERTY(float, angle, SetAngle, GetAngle);          // Handle angle in radians property
        PROPERTY(Vec2F, position, SetPosition, GetPosition); // Current position property
        PROPERTY(bool, enabled, SetEnabled, IsEnabled);      // Is handle enabled property. Disabled handle don't drawn and interact
        PROPERTY(bool, selected, SetSelected, IsSelected);   // Is handle selected property

    public:
        CursorType  cursorType = CursorType::Arrow; // Cursor type when hovering and dragging
        bool        pixelPerfect = true;            // Is handle draws pixel perfect
        KeyboardKey snappingKey = VK_CONTROL;       // Snapping key, when it pressed handle uses checkSnappingFunc to correct position

    public:
		Function<void(const Vec2F&)> onChangedPos;      // On position changed event
		Function<void()>             onChangeCompleted; // Change completed event

		Function<void()> onPressed;    // Pressed cursor on handle event
		Function<void()> onDblClicked; // Double clicked cursor on handle event
		Function<void()> onReleased;   // Released cursor event

		Function<void(const Input::Cursor&)> onRightButtonPressed;  // Right mouse button pressed event
		Function<void(const Input::Cursor&)> onRightButtonReleased; // Right mouse button released event

		Function<void()> onHoverEnter; // Hovered cursor enter event
		Function<void()> onHoverExit;  // Hovered cursor exit event

        Function<void()> onBeganDragging; // Drag begin event

        Function<bool(const Vec2F&)> isPointInside; // Checking point inside function. When this is empty, using IRectDrawable inside checking function

        Function<Vec2F(const Vec2F&)> screenToLocalTransformFunc; // Screen position to local transformation function
        Function<Vec2F(const Vec2F&)> localToScreenTransformFunc; // Local position to screen transformation function
        Function<Vec2F(const Vec2F&)> checkPositionFunc;          // Position constraints checking function
        Function<Vec2F(const Vec2F&)> checkSnappingFunc;          // Checking snapping position when dragging with pressed snapping key

		Function<void()> onSelected;   // Selected event
		Function<void()> onDeselected; // Deselected event

    public:
        // Default constructor
        DragHandle();

        // Default constructor
        DragHandle(RefCounter* refCounter);

        // Constructor with views
        DragHandle(const Ref<IRectDrawable>& regular, const Ref<IRectDrawable>& hover = nullptr,
                   const Ref<IRectDrawable>& pressed = nullptr, const Ref<IRectDrawable>& selected = nullptr,
                   const Ref<IRectDrawable>& selectedHovered = nullptr, const Ref<IRectDrawable>& selectedPressed = nullptr);

        // Constructor with views
        DragHandle(RefCounter* refCounter, const Ref<IRectDrawable>& regular, const Ref<IRectDrawable>& hover = nullptr,
                   const Ref<IRectDrawable>& pressed = nullptr, const Ref<IRectDrawable>& selected = nullptr,
                   const Ref<IRectDrawable>& selectedHovered = nullptr, const Ref<IRectDrawable>& selectedPressed = nullptr);

        // Copy-constructor
        DragHandle(const DragHandle& other);

        // Copy-constructor
        DragHandle(RefCounter* refCounter, const DragHandle& other);

        // Destructor
        virtual ~DragHandle();

        // Copy-operator
        DragHandle& operator=(const DragHandle& other);

        // Draws handle
        void Draw() override;

        // Draws handle with clipping check
        void Draw(const RectF& screenClipRect);

        // Returns true if point is above this
        bool IsUnderPoint(const Vec2F& point) override;

        // Sets position
        void SetPosition(const Vec2F& position);

        // Returns handle screen position, transformed from position with localToScreenTransformFunc
        const Vec2F& GetScreenPosition() const;

        // Updates screen position from position with localToScreenTransformFunc
        virtual void UpdateScreenPosition();

        // Sets drag position of handle, updates handle final position after position checking
        void SetDragPosition(const Vec2F& position);

        // Returns drag position
        Vec2F GetDragPosition() const;

        // Returns position
        Vec2F GetPosition() const;

        // Returns handle dragging offset to cursor
        Vec2F GetDraggingOffset() const;

        // Returns position at beginning of dragging
        Vec2F GetDraggingBeginPosition() const;

        // Called when handle beginning to drag from outside, for example from selection group. Updates drag position and drag offset
        void BeginDrag(const Vec2F& cursor);

        // Sets is this selected
        virtual void SetSelected(bool selected);

        // Returns is this selected
        bool IsSelected() const;

        // Selects this
        void Select();

        // Deselects this
        void Deselect();

        // Sets selection group
        void SetSelectionGroup(const Ref<ISelectableDragHandlesGroup>& group);

        // Returns selection group
        const Ref<ISelectableDragHandlesGroup>& GetSelectionGroup() const;

        // Set handle enabled. Disabled handle don't drawn and interact
        virtual void SetEnabled(bool enabled);

        // Returns is handle enabled. Disabled handle don't drawn and interact
        bool IsEnabled() const;

        // Sets handle rotation angle in radians
        virtual void SetAngle(float rad);

        // Return handle rotation angle in radians
        float GetAngle() const;

        // Returns cursor position when it was pressed
        const Vec2F& GetPressedCursorPos() const;

        // Converts point from screen to local space
        virtual Vec2F ScreenToLocal(const Vec2F& point);

        // Converts point from local to screen space
        virtual Vec2F LocalToScreen(const Vec2F& point);

        // Sets regular drawable
        void SetRegularDrawable(const Ref<IRectDrawable>& IRectDrawable);

        // Returns regular drawable
        const Ref<IRectDrawable>& GetRegularDrawable() const;

        // Sets hovered drawable
        void SetHoverDrawable(const Ref<IRectDrawable>& IRectDrawable);

        // Returns hovered drawable
        const Ref<IRectDrawable>& GetHoverDrawable() const;

        // Sets pressed drawable
        void SetPressedDrawable(const Ref<IRectDrawable>& IRectDrawable);

        // Returns pressed drawable
        const Ref<IRectDrawable>& GetPressedDrawable() const;

        // Sets selected drawable
        void SetSelectedDrawable(const Ref<IRectDrawable>& IRectDrawable);

        // Returns selected drawable
        const Ref<IRectDrawable>& GetSelectedDrawable() const;

        // Sets selected hovered drawable
        void SetSelectedHoveredDrawable(const Ref<IRectDrawable>& IRectDrawable);

        // Returns selected hovered drawable
        const Ref<IRectDrawable>& GetSelectedHoveredDrawable() const;

        // Sets selected pressed drawable
        void SetSelectedPressedDrawable(const Ref<IRectDrawable>& IRectDrawable);

        // Returns selected pressed drawable
        const Ref<IRectDrawable>& GetSelectedPressedDrawable() const;

        // Sets size to all available drawable
        void SetDrawablesSize(const Vec2F& size);

        // Sets color to all drawable
        void SetDrawablesColor(const Color4& color);

        // Sets size to all available drawable
        void SetDrawablesSizePivot(const Vec2F& pivot);

        // Returns is listener scrollable
        bool IsScrollable() const override;

        SERIALIZABLE(DragHandle);
        CLONEABLE_REF(DragHandle);

    protected:
        Ref<IRectDrawable> mRegularDrawable;  // Regular view IRectDrawable @SERIALIZABLE
        Ref<IRectDrawable> mHoverDrawable;    // Hovered view IRectDrawable @SERIALIZABLE
        Ref<IRectDrawable> mPressedDrawable;  // Pressed view IRectDrawable @SERIALIZABLE

        Ref<IRectDrawable> mSelectedDrawable;        // Selected view IRectDrawable @SERIALIZABLE
        Ref<IRectDrawable> mSelectedHoverDrawable;   // Selected hovered view IRectDrawable @SERIALIZABLE
        Ref<IRectDrawable> mSelectedPressedDrawable; // Selected pressed view IRectDrawable @SERIALIZABLE

        bool   mEnabled = true; // Is handle enabled. Disabled handle don't drawn and interact

        Vec2F  mPosition;                      // Current handle position, checked by checkPositionFunc
        Vec2F  mScreenPosition;                // Handle screen position, transformed from mPosition with localToScreenTransformFunc
        Vec2F  mLastDrawnScreenPosition;       // Handle screen position on last draw
        int    mLastScreenPosUpdateFrame = -1; // Last screen position update frame index
        float  mAngle = 0.0f;                  // Handle rotation angle in radians

        Vec2F  mDragOffset;        // Dragging offset from cursor in local space to center
        Vec2F  mDragPosition;      // Current drag handle position
        Vec2F  mDragBeginPosition; // Position at beginning dragging

        int    mPressedCursorId;    // Id of pressed cursor
        Vec2F  mPressedCursorPos;   // Cursor pressed position
        bool   mIsDragging = false; // Is handle in dragging mode
        bool   mIsHovered = false;  // Is handle under cursor, used for hover IRectDrawable appearing

        bool                             mIsSelected = false; // Is this selected
        Ref<ISelectableDragHandlesGroup> mSelectGroup;        // Selection group

        float  mDragDistanceThreshold = 3.0f; // Drag distance threshold: object starts dragging when cursor moves more tan this distance

    protected:
        // Draws internal IRectDrawables with calculated screen position
        void DrawInternal();

        // Called when cursor pressed on this
        void OnCursorPressed(const Input::Cursor& cursor) override;

        // Called when cursor double clicked
        void OnCursorDblClicked(const Input::Cursor& cursor) override;

        // Called when cursor released (only when cursor pressed this at previous time)
        void OnCursorReleased(const Input::Cursor& cursor) override;

        // Called when cursor released outside this(only when cursor pressed this at previous time)
        void OnCursorReleasedOutside(const Input::Cursor& cursor) override;

        // Called when cursor pressing was broken (when scrolled scroll area or some other)
        void OnCursorPressBreak(const Input::Cursor& cursor) override;

        // Called when cursor stay down during frame
        void OnCursorStillDown(const Input::Cursor& cursor) override;

        // Called when cursor enters this object
        void OnCursorEnter(const Input::Cursor& cursor) override;

        // Called when cursor exits this object
        void OnCursorExit(const Input::Cursor& cursor) override;

        // Called when right mouse button was pressed on this, calls onRightButtonPressed event
        void OnCursorRightMousePressed(const Input::Cursor& cursor) override;

        // Called when right mouse button was released (only when right mouse button pressed this at previous time), calls onRightButtonReleased event
        void OnCursorRightMouseReleased(const Input::Cursor& cursor) override;

        // Updates IRectDrawables positions by handle screen position
        virtual void UpdateDrawablesPositions();

        // Called when this was selected
        virtual void OnSelected();

        // Called when this was unselected
        virtual void OnDeselected();

        REF_COUNTERABLE_IMPL(RefCounterable);

        friend class ISelectableDragHandlesGroup;
        friend class SelectableDragHandlesGroup;
    };

    // ---------------------------------------------------------
    // Widget drag handle. Can be attached to widget as children
    // ---------------------------------------------------------
    class WidgetDragHandle: public DragHandle, public Widget
    {
    public:
        Function<Vec2F(const Vec2F&)> widgetOffsetToLocalTransformFunc; // Widget offset relative to parent to local transformation function
        Function<Vec2F(const Vec2F&)> localToWidgetOffsetTransformFunc; // Local position to widget offset relative to parent transformation function

        Function<void()> onLayoutUpdated; // Called when this layout were updated

    public:
        // Default constructor
        explicit WidgetDragHandle(RefCounter* refCounter);

        // Constructor with views
        WidgetDragHandle(RefCounter* refCounter, const Ref<IRectDrawable>& regular, const Ref<IRectDrawable>& hover = nullptr, const Ref<IRectDrawable>& pressed = nullptr,
                         const Ref<IRectDrawable>& selected = nullptr, const Ref<IRectDrawable>& selectedHovered = nullptr,
                         const Ref<IRectDrawable>& selectedPressed = nullptr);

        // Copy-constructor
        WidgetDragHandle(RefCounter* refCounter, const WidgetDragHandle& other);

        // Copy-constructor
        WidgetDragHandle(const WidgetDragHandle& other):WidgetDragHandle(nullptr, other) {}

        // Destructor
        ~WidgetDragHandle();

        // Copy-operator
        WidgetDragHandle& operator=(const WidgetDragHandle& other);

        // Draws handle
        void Draw() override;

        // Set handle enabled. Disabled handle don't drawn and interact
        void SetEnabled(bool enabled) override;

        // Returns is handle enabled. Disabled handle don't drawn and interact
        bool IsEnabled() const OPTIONAL_OVERRIDE;

        // Converts point from screen to local space
        Vec2F ScreenToLocal(const Vec2F& point) override;

        // Converts point from local to screen space
        Vec2F LocalToScreen(const Vec2F& point) override;

        // Returns create menu category in editor
        static String GetCreateMenuCategory();

        // Dynamic cast to RefCounterable via Widget
        static Ref<RefCounterable> CastToRefCounterable(const Ref<WidgetDragHandle>& ref);

        SERIALIZABLE(WidgetDragHandle);

        Ref<RefCounterable> CloneRef() const override { return DynamicCast<Widget>(mmake<WidgetDragHandle>(*this)); }

    protected:
        // Hide public functions
        using DragHandle::screenToLocalTransformFunc;
        using DragHandle::localToScreenTransformFunc;

        // Updates layers layouts, calls after updating widget layout
        void UpdateLayersLayouts() override;

        // Beginning serialization callback
        void OnSerialize(DataValue& node) const override;

        // Called when deserialized
        void OnDeserialized(const DataValue& node) override;

        // Completion deserialization delta callback; calls widget's function
        void OnDeserializedDelta(const DataValue& node, const IObject& origin) override;

        REF_COUNTERABLE_IMPL(Widget);
    };

    // --------------------------------------------
    // Selectable draggable handles group interface
    // --------------------------------------------
    class ISelectableDragHandlesGroup: public RefCounterable
    {
    public:
        // Default constructor
        ISelectableDragHandlesGroup();

        // Default constructor
        ISelectableDragHandlesGroup(RefCounter *refCounter);

        // Returns selected handles in group
        virtual const Vector<Ref<DragHandle>>& GetSelectedHandles() const = 0;

        // Returns all handles in group 
        virtual Vector<Ref<DragHandle>> GetAllHandles() const = 0;

        // Selects handle
        virtual void SelectHandle(const Ref<DragHandle>& handle) = 0;

        // Deselects handle
        virtual void DeselectHandle(const Ref<DragHandle>& handle) = 0;

        // Adds selectable handle to group
        virtual void AddHandle(const Ref<DragHandle>& handle) = 0;

        // Removes selectable handle from group
        virtual void RemoveHandle(DragHandle* handle) = 0;

        // Deselects all in group
        virtual void DeselectAll();

        // Selects all in group
        virtual void SelectAll();

    protected:
        // Called when selectable draggable handle was pressed
        virtual void OnHandleCursorPressed(const Ref<DragHandle>& handle, const Input::Cursor& cursor) { }

        // Called when selectable draggable handle was released
        virtual void OnHandleCursorReleased(const Ref<DragHandle>& handle, const Input::Cursor& cursor) { }

        // Called when selectable handle was began to drag
        virtual void OnHandleBeganDragging(const Ref<DragHandle>& handle) { }

        // Called when selectable handle moved, moves all selected handles position
        virtual void OnHandleMoved(const Ref<DragHandle>& handle, const Vec2F& cursorPos) { }

        // Called when selectable handle completed changing
        virtual void OnHandleCompletedChange(const Ref<DragHandle>& handle) { }

        // Sets handle selected state without adding to selected handles array
        void SetHandleSelectedState(const Ref<DragHandle>& handle, bool selected);

        friend class DragHandle;
    };


    // ----------------------------------
    // Selectable draggable handles group
    // ----------------------------------
    class SelectableDragHandlesGroup: public ISelectableDragHandlesGroup
    {
    public:
        // Default constructor
        SelectableDragHandlesGroup();

        // Default constructor
        SelectableDragHandlesGroup(RefCounter* refCounter);

        // Destructor
        ~SelectableDragHandlesGroup();

        // Returns selected handles in group
        const Vector<Ref<DragHandle>>& GetSelectedHandles() const override;

        // Returns all handles in group 
        Vector<Ref<DragHandle>> GetAllHandles() const override;

        // Selects handle
        void SelectHandle(const Ref<DragHandle>& handle) override;

        // Deselects handle
        void DeselectHandle(const Ref<DragHandle>& handle) override;

        // Adds selectable handle to group
        void AddHandle(const Ref<DragHandle>& handle) override;

        // Removes selectable handle from group
        void RemoveHandle(DragHandle* handle) override;

        // Deselects all in group
        void DeselectAll() override;

        // Selects all in group
        void SelectAll() override;

    protected:
        Vector<Ref<DragHandle>> mSelectedHandles;
        Vector<Ref<DragHandle>> mHandles;

    protected:
        // Called when selection is changed - some handle was added or removed from selection
        virtual void OnSelectionChanged();

        // Called when selectable draggable handle was pressed
        void OnHandleCursorPressed(const Ref<DragHandle>& handle, const Input::Cursor& cursor) override;

        // Called when selectable draggable handle was released
        void OnHandleCursorReleased(const Ref<DragHandle>& handle, const Input::Cursor& cursor) override;

        // Called when selectable handle was began to drag
        void OnHandleBeganDragging(const Ref<DragHandle>& handle) override;

        // Called when selectable handle moved, moves all selected handles position
        void OnHandleMoved(const Ref<DragHandle>& handle, const Vec2F& cursorPos) override;

        friend class DragHandle;
    };
}
// --- META ---

CLASS_BASES_META(o2::DragHandle)
{
    BASE_CLASS(o2::RefCounterable);
    BASE_CLASS(o2::IDrawable);
    BASE_CLASS(o2::CursorAreaEventsListener);
    BASE_CLASS(o2::ISerializable);
    BASE_CLASS(o2::ICloneableRef);
}
END_META;
CLASS_FIELDS_META(o2::DragHandle)
{
    FIELD().PUBLIC().NAME(angle);
    FIELD().PUBLIC().NAME(position);
    FIELD().PUBLIC().NAME(enabled);
    FIELD().PUBLIC().NAME(selected);
    FIELD().PUBLIC().DEFAULT_VALUE(CursorType::Arrow).NAME(cursorType);
    FIELD().PUBLIC().DEFAULT_VALUE(true).NAME(pixelPerfect);
    FIELD().PUBLIC().DEFAULT_VALUE(VK_CONTROL).NAME(snappingKey);
    FIELD().PUBLIC().NAME(onChangedPos);
    FIELD().PUBLIC().NAME(onChangeCompleted);
    FIELD().PUBLIC().NAME(onPressed);
    FIELD().PUBLIC().NAME(onDblClicked);
    FIELD().PUBLIC().NAME(onReleased);
    FIELD().PUBLIC().NAME(onRightButtonPressed);
    FIELD().PUBLIC().NAME(onRightButtonReleased);
    FIELD().PUBLIC().NAME(onHoverEnter);
    FIELD().PUBLIC().NAME(onHoverExit);
    FIELD().PUBLIC().NAME(onBeganDragging);
    FIELD().PUBLIC().NAME(isPointInside);
    FIELD().PUBLIC().NAME(screenToLocalTransformFunc);
    FIELD().PUBLIC().NAME(localToScreenTransformFunc);
    FIELD().PUBLIC().NAME(checkPositionFunc);
    FIELD().PUBLIC().NAME(checkSnappingFunc);
    FIELD().PUBLIC().NAME(onSelected);
    FIELD().PUBLIC().NAME(onDeselected);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().NAME(mRegularDrawable);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().NAME(mHoverDrawable);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().NAME(mPressedDrawable);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().NAME(mSelectedDrawable);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().NAME(mSelectedHoverDrawable);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().NAME(mSelectedPressedDrawable);
    FIELD().PROTECTED().DEFAULT_VALUE(true).NAME(mEnabled);
    FIELD().PROTECTED().NAME(mPosition);
    FIELD().PROTECTED().NAME(mScreenPosition);
    FIELD().PROTECTED().NAME(mLastDrawnScreenPosition);
    FIELD().PROTECTED().DEFAULT_VALUE(-1).NAME(mLastScreenPosUpdateFrame);
    FIELD().PROTECTED().DEFAULT_VALUE(0.0f).NAME(mAngle);
    FIELD().PROTECTED().NAME(mDragOffset);
    FIELD().PROTECTED().NAME(mDragPosition);
    FIELD().PROTECTED().NAME(mDragBeginPosition);
    FIELD().PROTECTED().NAME(mPressedCursorId);
    FIELD().PROTECTED().NAME(mPressedCursorPos);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mIsDragging);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mIsHovered);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mIsSelected);
    FIELD().PROTECTED().NAME(mSelectGroup);
    FIELD().PROTECTED().DEFAULT_VALUE(3.0f).NAME(mDragDistanceThreshold);
}
END_META;
CLASS_METHODS_META(o2::DragHandle)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().CONSTRUCTOR(const Ref<IRectDrawable>&, const Ref<IRectDrawable>&, const Ref<IRectDrawable>&, const Ref<IRectDrawable>&, const Ref<IRectDrawable>&, const Ref<IRectDrawable>&);
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*, const Ref<IRectDrawable>&, const Ref<IRectDrawable>&, const Ref<IRectDrawable>&, const Ref<IRectDrawable>&, const Ref<IRectDrawable>&, const Ref<IRectDrawable>&);
    FUNCTION().PUBLIC().CONSTRUCTOR(const DragHandle&);
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*, const DragHandle&);
    FUNCTION().PUBLIC().SIGNATURE(void, Draw);
    FUNCTION().PUBLIC().SIGNATURE(void, Draw, const RectF&);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsUnderPoint, const Vec2F&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetPosition, const Vec2F&);
    FUNCTION().PUBLIC().SIGNATURE(const Vec2F&, GetScreenPosition);
    FUNCTION().PUBLIC().SIGNATURE(void, UpdateScreenPosition);
    FUNCTION().PUBLIC().SIGNATURE(void, SetDragPosition, const Vec2F&);
    FUNCTION().PUBLIC().SIGNATURE(Vec2F, GetDragPosition);
    FUNCTION().PUBLIC().SIGNATURE(Vec2F, GetPosition);
    FUNCTION().PUBLIC().SIGNATURE(Vec2F, GetDraggingOffset);
    FUNCTION().PUBLIC().SIGNATURE(Vec2F, GetDraggingBeginPosition);
    FUNCTION().PUBLIC().SIGNATURE(void, BeginDrag, const Vec2F&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetSelected, bool);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsSelected);
    FUNCTION().PUBLIC().SIGNATURE(void, Select);
    FUNCTION().PUBLIC().SIGNATURE(void, Deselect);
    FUNCTION().PUBLIC().SIGNATURE(void, SetSelectionGroup, const Ref<ISelectableDragHandlesGroup>&);
    FUNCTION().PUBLIC().SIGNATURE(const Ref<ISelectableDragHandlesGroup>&, GetSelectionGroup);
    FUNCTION().PUBLIC().SIGNATURE(void, SetEnabled, bool);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsEnabled);
    FUNCTION().PUBLIC().SIGNATURE(void, SetAngle, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetAngle);
    FUNCTION().PUBLIC().SIGNATURE(const Vec2F&, GetPressedCursorPos);
    FUNCTION().PUBLIC().SIGNATURE(Vec2F, ScreenToLocal, const Vec2F&);
    FUNCTION().PUBLIC().SIGNATURE(Vec2F, LocalToScreen, const Vec2F&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetRegularDrawable, const Ref<IRectDrawable>&);
    FUNCTION().PUBLIC().SIGNATURE(const Ref<IRectDrawable>&, GetRegularDrawable);
    FUNCTION().PUBLIC().SIGNATURE(void, SetHoverDrawable, const Ref<IRectDrawable>&);
    FUNCTION().PUBLIC().SIGNATURE(const Ref<IRectDrawable>&, GetHoverDrawable);
    FUNCTION().PUBLIC().SIGNATURE(void, SetPressedDrawable, const Ref<IRectDrawable>&);
    FUNCTION().PUBLIC().SIGNATURE(const Ref<IRectDrawable>&, GetPressedDrawable);
    FUNCTION().PUBLIC().SIGNATURE(void, SetSelectedDrawable, const Ref<IRectDrawable>&);
    FUNCTION().PUBLIC().SIGNATURE(const Ref<IRectDrawable>&, GetSelectedDrawable);
    FUNCTION().PUBLIC().SIGNATURE(void, SetSelectedHoveredDrawable, const Ref<IRectDrawable>&);
    FUNCTION().PUBLIC().SIGNATURE(const Ref<IRectDrawable>&, GetSelectedHoveredDrawable);
    FUNCTION().PUBLIC().SIGNATURE(void, SetSelectedPressedDrawable, const Ref<IRectDrawable>&);
    FUNCTION().PUBLIC().SIGNATURE(const Ref<IRectDrawable>&, GetSelectedPressedDrawable);
    FUNCTION().PUBLIC().SIGNATURE(void, SetDrawablesSize, const Vec2F&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetDrawablesColor, const Color4&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetDrawablesSizePivot, const Vec2F&);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsScrollable);
    FUNCTION().PROTECTED().SIGNATURE(void, DrawInternal);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorPressed, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorDblClicked, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorReleased, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorReleasedOutside, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorPressBreak, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorStillDown, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorEnter, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorExit, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorRightMousePressed, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorRightMouseReleased, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateDrawablesPositions);
    FUNCTION().PROTECTED().SIGNATURE(void, OnSelected);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDeselected);
}
END_META;

CLASS_BASES_META(o2::WidgetDragHandle)
{
    BASE_CLASS(o2::DragHandle);
    BASE_CLASS(o2::Widget);
}
END_META;
CLASS_FIELDS_META(o2::WidgetDragHandle)
{
    FIELD().PUBLIC().NAME(widgetOffsetToLocalTransformFunc);
    FIELD().PUBLIC().NAME(localToWidgetOffsetTransformFunc);
    FIELD().PUBLIC().NAME(onLayoutUpdated);
}
END_META;
CLASS_METHODS_META(o2::WidgetDragHandle)
{

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*, const Ref<IRectDrawable>&, const Ref<IRectDrawable>&, const Ref<IRectDrawable>&, const Ref<IRectDrawable>&, const Ref<IRectDrawable>&, const Ref<IRectDrawable>&);
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*, const WidgetDragHandle&);
    FUNCTION().PUBLIC().CONSTRUCTOR(const WidgetDragHandle&);
    FUNCTION().PUBLIC().SIGNATURE(void, Draw);
    FUNCTION().PUBLIC().SIGNATURE(void, SetEnabled, bool);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsEnabled);
    FUNCTION().PUBLIC().SIGNATURE(Vec2F, ScreenToLocal, const Vec2F&);
    FUNCTION().PUBLIC().SIGNATURE(Vec2F, LocalToScreen, const Vec2F&);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetCreateMenuCategory);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(Ref<RefCounterable>, CastToRefCounterable, const Ref<WidgetDragHandle>&);
    FUNCTION().PUBLIC().SIGNATURE(Ref<RefCounterable>, CloneRef);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateLayersLayouts);
    FUNCTION().PROTECTED().SIGNATURE(void, OnSerialize, DataValue&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDeserialized, const DataValue&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDeserializedDelta, const DataValue&, const IObject&);
}
END_META;
// --- END META ---
