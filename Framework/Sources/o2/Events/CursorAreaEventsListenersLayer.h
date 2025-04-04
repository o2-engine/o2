#pragma once
#include "o2/Events/CursorAreaEventsListener.h"
#include "o2/Render/Camera.h"
#include "o2/Utils/Math/Basis.h"
#include "o2/Utils/Types/Containers/Vector.h"

namespace o2
{
    FORWARD_CLASS_REF(DragableObject);

    class CursorAreaEventListenersLayer: public RefCounterable, public CursorAreaEventsListener
    {
    public:
		Basis  drawnTransform; // Transform at drawing moment
		Basis  viewPortBasis;  // Camera view port basis before layer content drawing moment
		Basis  renderBasis;    // Camera render basis after layer content drawing moment
		Camera camera;         // Camera used to draw this layer

		Vector<WeakRef<CursorAreaEventsListener>> cursorEventAreaListeners; // List of contained cursor event listeners

        bool isEditor      = false; // Is this layer drawn in editor scope
        bool isTransparent = false; // Is this layer transparent to input other listeners

		String name; // Layer name, used to debug

    public:
        // Destructor. Unregisters from events system
        ~CursorAreaEventListenersLayer();

        // Called when layer sub listeners has began draw with actual camera
        void OnBeginDraw();

        // Called when layer sub listeners has ended to draw
        void OnEndDraw();

        // Called when layer has drawn
        void OnDrawn(const Basis& transform);

        // Converts point to local coordinates from parent layer
        Vec2F ToLocal(const Vec2F& point) const;

        // Converts from local point from parent layer
        Vec2F FromLocal(const Vec2F& point) const;

        // Converts point to local coordinates from screen coordinates
        Vec2F ScreenToLocal(const Vec2F& point) const;

        // Converts from local point from screen coordinates
        Vec2F ScreenFromLocal(const Vec2F& point) const;

        // Updates and processes events
        void Update();

        // Post update events
        void PostUpdate();

        // Breaks cursor event. All pressed listeners will be unpressed with specific event OnPressBreak
        void BreakCursorEvent();

        // Unregistering cursor area events listener
        void UnregCursorAreaListener(CursorAreaEventsListener* listener);

        // Unregistering drag events listener
        void UnregDragListener(DragableObject* listener);

        // Returns all cursor listeners under cursor arranged by depth
        Vector<Ref<CursorAreaEventsListener>> GetAllCursorListenersUnderCursor(const Vec2F& cursorPos) const;

        // Returns true if point is in this object
        bool IsUnderPoint(const Vec2F& point) override;

        // Returns true when input events can be handled by down listeners
        bool IsInputTransparent() const override;

		// Returns debug name
		String GetDebugName() const override;

    private:
        bool mEnabled = false;

        Basis mLocalToWorldTransform = Basis::Identity();

        WeakRef<CursorAreaEventListenersLayer> mParentLayer; // Layer in which this

        Map<CursorId, Vector<Ref<CursorAreaEventsListener>>> mPressedListeners;             // Pressed listeners for all pressed cursors
        Vector<Ref<CursorAreaEventsListener>>                mRightButtonPressedListeners;  // Right mouse button pressed listener
        Vector<Ref<CursorAreaEventsListener>>                mMiddleButtonPressedListeners; // Middle mouse button pressed listener

        Map<CursorId, Vector<Ref<CursorAreaEventsListener>>> mUnderCursorListeners;     // Under cursor listeners for each cursor
        Map<CursorId, Vector<Ref<CursorAreaEventsListener>>> mLastUnderCursorListeners; // Under cursor listeners for each cursor on last frame

        Vector<Ref<DragableObject>> mDragListeners; // Drag events listeners

    private:
        // Called when cursor enters this object
        void OnCursorEnter(const Input::Cursor& cursor) override;

        // Called when cursor exits this object
        void OnCursorExit(const Input::Cursor& cursor) override;

        // Converts cursor to local coordinates
        Input::Cursor ConvertLocalCursor(const Input::Cursor& cursor) const;

        // processes cursor tracing for cursor
        void ProcessCursorTracing(const Input::Cursor& cursor);

        // Processes cursor enter event
        void ProcessCursorEnter();

        // Processes cursor exit event
        void ProcessCursorExit();

        // Processes cursor pressed event
        void ProcessCursorPressed(const Input::Cursor& cursor);

        // Processes cursor down event
        void ProcessCursorDown(const Input::Cursor& cursor);

        // Processes cursor released event
        void ProcessCursorReleased(const Input::Cursor& cursor);

        // Processes mouse right button pressed event
        void ProcessRBPressed();

        // Processes mouse right button down event
        void ProcessRBDown();

        // Processes mouse right button released event
        void ProcessRBReleased();

        // Processes mouse middle button pressed event
        void ProcessMBPressed();

        // Processes mouse middle button down event
        void ProcessMBDown();

        // Processes mouse middle button released event
        void ProcessMBReleased();

        // Processes scrolling event
        void ProcessScrolling();

        REF_COUNTERABLE_IMPL(RefCounterable);

        friend class EventSystem;
    };
}
