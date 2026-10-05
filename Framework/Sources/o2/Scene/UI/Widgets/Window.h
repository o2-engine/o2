#pragma once

#include "o2/Render/Sprite.h"
#include "o2/Scene/UI/Widgets/ScrollArea.h"
#include "o2/Events/CursorEventsArea.h"

namespace o2
{
    class Text;
    class ContextMenu;

    // ----------------------------------------------------
    // Window with caption, icon, options and close buttons
    // ----------------------------------------------------
    class Window: public ScrollArea
    {
    public:
        PROPERTIES(Window);
        PROPERTY(WString, caption, SetCaption, GetCaption); // Window caption property
        PROPERTY(Ref<IRectDrawable>, icon, SetIcon, GetIconDrawable); // Window icon drawable property

        Function<void()> onOpened; // On window opened delegate
        Function<void()> onClosed; // On window closed delegate

    public:
        // Default constructor
        explicit Window(RefCounter* refCounter);

        // Copy-constructor
        Window(RefCounter* refCounter, const Window& other);

        // Destructor
        ~Window();

        // Copy-operator
        Window& operator=(const Window& other);

        // Draws widget
        void Draw() override;

        // Shows window as modal
        void ShowModal();

        // Sets icon drawable
        virtual void SetIcon(const Ref<IRectDrawable>& icon);

        // Returns icon as sprite, null when it is a drawable of another type
        virtual Ref<Sprite> GetIcon() const;

        // Returns icon drawable
        virtual Ref<IRectDrawable> GetIconDrawable() const;

        // Sets icon layer layout
        virtual void SetIconLayout(const Layout& layout);

        // Returns icon layer layout
        virtual Layout GetIconLayout() const;

        // Sets caption text
        virtual void SetCaption(const WString& caption);

        // Returns caption text
        virtual WString GetCaption() const;

        // Returns options context menu
        const Ref<ContextMenu>& GetOptionsMenu() const;

        // Sets drag handle areas layouts
        void SetDragAreaLayouts(const Layout& head, const Layout& top, const Layout& bottom, const Layout& left,
                                const Layout& right, const Layout& leftTop, const Layout& rightTop, const Layout& leftBottom,
                                const Layout& rightBottom);

        // Returns is this widget can be selected
        bool IsFocusable() const override;

        // Sets is window modal
        void SetModal(bool isModal);

        // Returns is window modal
        bool IsModal() const;

        // Updates layout
        void UpdateSelfTransform() override;

        // Returns back cursor events listener
        CursorEventsArea& GetBackCursorListener();

        // Returns create menu group in editor
        static String GetCreateMenuGroup();

        SERIALIZABLE(Window);
        CLONEABLE_REF(Window);

    protected:
        static constexpr auto mIconLayerPath = "icon";
        static constexpr auto mCaptionLayerPath = "caption";

        Ref<ContextMenu> mOptionsMenu; // Window options context menu

        Ref<CursorEventsArea> mBackCursorArea;   // Cursor area listener at back of window, for catching events
        Ref<CursorEventsArea> mWindowCursorArea; // Cursor area listener for window frame

        Ref<CursorEventsArea> mHeadDragHandle;     // Head drag handle, for moving window
        Layout                mHeadDragAreaLayout; // Head drag handle layout @SERIALIZABLE
        RectF                 mHeadDragAreaRect;   // Head drag handle rect

        Ref<CursorEventsArea> mTopDragHandle;     //    Top drag handle, for resizing window
        Layout                mTopDragAreaLayout; //    Top drag handle layout @SERIALIZABLE
        RectF                 mTopDragAreaRect;   //    Top drag handle rect

        Ref<CursorEventsArea> mBottomDragHandle;     // Bottom drag handle, for resizing window
        Layout                mBottomDragAreaLayout; // Bottom drag handle layout @SERIALIZABLE
        RectF                 mBottomDragAreaRect;   // Bottom drag handle rect

        Ref<CursorEventsArea> mLeftDragHandle;     // Left drag handle, for resizing window
        Layout                mLeftDragAreaLayout; // Left drag handle layout @SERIALIZABLE
        RectF                 mLeftDragAreaRect;   // Left drag handle rect

        Ref<CursorEventsArea> mRightDragHandle;     // Right drag handle, for resizing window
        Layout                mRightDragAreaLayout; // Right drag handle layout @SERIALIZABLE
        RectF                 mRightDragAreaRect;   // Right drag handle rect

        Ref<CursorEventsArea> mLeftTopDragHandle;     // Left Top drag handle, for resizing window
        Layout                mLeftTopDragAreaLayout; // Left Top drag handle layout @SERIALIZABLE
        RectF                 mLeftTopDragAreaRect;   // Left Top drag handle rect
                                                     
        Ref<CursorEventsArea> mRightTopDragHandle;     // Right Top drag handle, for resizing window
        Layout                mRightTopDragAreaLayout; // Right Top drag handle layout @SERIALIZABLE
        RectF                 mRightTopDragAreaRect;   // Right Top drag handle rect
                                                    
        Ref<CursorEventsArea> mLeftBottomDragHandle;     // Left Bottom drag handle, for resizing window
        Layout                mLeftBottomDragAreaLayout; // Left Bottom drag handle layout @SERIALIZABLE
        RectF                 mLeftBottomDragAreaRect;   // Left Bottom drag handle rect
                                                     
        Ref<CursorEventsArea> mRightBottomDragHandle;     // Right Bottom drag handle, for resizing window
        Layout                mRightBottomDragAreaLayout; // Right Bottom drag handle layout @SERIALIZABLE
        RectF                 mRightBottomDragAreaRect;   // Right Bottom drag handle rect

    protected:
        // Called when widget was selected
        void OnFocused() override;

        // Called when actor enabled in hierarchy, calls onOpened
        void OnEnabled() override;

        // Called when actor disabled in hierarchy, calls onCLosed
        void OnDisabled()override;

        // Called when widget state was added
        void OnStateAdded(const Ref<WidgetState>& state) override;

        // Initializes context menu and options button
        void InitializeContextMenu();

        // Initializes context menu items - close window
        virtual void InitializeContextItems();

        // Restores close button, options button and context menu from window elements
        void RestoreControls();

        // Initializes handles
        void InitializeHandles();

        // Sets drag handles interactable. Using when changing visibility
        void SetHandlesInteractable(bool interactable);

        // Binds all drag handles interactable parameter to window visibility
        void BindHandlesInteractableToVisibility();

        // Called when child widget was selected
        void OnChildFocused(const Ref<Widget>& child) override;

        // Called when cursor pressed on this
        void OnCursorPressed(const Input::Cursor& cursor) override;

        REF_COUNTERABLE_IMPL(Widget);
    };
}
// --- META ---

CLASS_BASES_META(o2::Window)
{
    BASE_CLASS(o2::ScrollArea);
}
END_META;
CLASS_FIELDS_META(o2::Window)
{
    FIELD().PUBLIC().NAME(caption);
    FIELD().PUBLIC().NAME(icon);
    FIELD().PUBLIC().NAME(onOpened);
    FIELD().PUBLIC().NAME(onClosed);
    FIELD().PROTECTED().NAME(mOptionsMenu);
    FIELD().PROTECTED().NAME(mBackCursorArea);
    FIELD().PROTECTED().NAME(mWindowCursorArea);
    FIELD().PROTECTED().NAME(mHeadDragHandle);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().NAME(mHeadDragAreaLayout);
    FIELD().PROTECTED().NAME(mHeadDragAreaRect);
    FIELD().PROTECTED().NAME(mTopDragHandle);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().NAME(mTopDragAreaLayout);
    FIELD().PROTECTED().NAME(mTopDragAreaRect);
    FIELD().PROTECTED().NAME(mBottomDragHandle);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().NAME(mBottomDragAreaLayout);
    FIELD().PROTECTED().NAME(mBottomDragAreaRect);
    FIELD().PROTECTED().NAME(mLeftDragHandle);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().NAME(mLeftDragAreaLayout);
    FIELD().PROTECTED().NAME(mLeftDragAreaRect);
    FIELD().PROTECTED().NAME(mRightDragHandle);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().NAME(mRightDragAreaLayout);
    FIELD().PROTECTED().NAME(mRightDragAreaRect);
    FIELD().PROTECTED().NAME(mLeftTopDragHandle);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().NAME(mLeftTopDragAreaLayout);
    FIELD().PROTECTED().NAME(mLeftTopDragAreaRect);
    FIELD().PROTECTED().NAME(mRightTopDragHandle);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().NAME(mRightTopDragAreaLayout);
    FIELD().PROTECTED().NAME(mRightTopDragAreaRect);
    FIELD().PROTECTED().NAME(mLeftBottomDragHandle);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().NAME(mLeftBottomDragAreaLayout);
    FIELD().PROTECTED().NAME(mLeftBottomDragAreaRect);
    FIELD().PROTECTED().NAME(mRightBottomDragHandle);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().NAME(mRightBottomDragAreaLayout);
    FIELD().PROTECTED().NAME(mRightBottomDragAreaRect);
}
END_META;
CLASS_METHODS_META(o2::Window)
{

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*, const Window&);
    FUNCTION().PUBLIC().SIGNATURE(void, Draw);
    FUNCTION().PUBLIC().SIGNATURE(void, ShowModal);
    FUNCTION().PUBLIC().SIGNATURE(void, SetIcon, const Ref<IRectDrawable>&);
    FUNCTION().PUBLIC().SIGNATURE(Ref<Sprite>, GetIcon);
    FUNCTION().PUBLIC().SIGNATURE(Ref<IRectDrawable>, GetIconDrawable);
    FUNCTION().PUBLIC().SIGNATURE(void, SetIconLayout, const Layout&);
    FUNCTION().PUBLIC().SIGNATURE(Layout, GetIconLayout);
    FUNCTION().PUBLIC().SIGNATURE(void, SetCaption, const WString&);
    FUNCTION().PUBLIC().SIGNATURE(WString, GetCaption);
    FUNCTION().PUBLIC().SIGNATURE(const Ref<ContextMenu>&, GetOptionsMenu);
    FUNCTION().PUBLIC().SIGNATURE(void, SetDragAreaLayouts, const Layout&, const Layout&, const Layout&, const Layout&, const Layout&, const Layout&, const Layout&, const Layout&, const Layout&);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsFocusable);
    FUNCTION().PUBLIC().SIGNATURE(void, SetModal, bool);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsModal);
    FUNCTION().PUBLIC().SIGNATURE(void, UpdateSelfTransform);
    FUNCTION().PUBLIC().SIGNATURE(CursorEventsArea&, GetBackCursorListener);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetCreateMenuGroup);
    FUNCTION().PROTECTED().SIGNATURE(void, OnFocused);
    FUNCTION().PROTECTED().SIGNATURE(void, OnEnabled);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDisabled);
    FUNCTION().PROTECTED().SIGNATURE(void, OnStateAdded, const Ref<WidgetState>&);
    FUNCTION().PROTECTED().SIGNATURE(void, InitializeContextMenu);
    FUNCTION().PROTECTED().SIGNATURE(void, InitializeContextItems);
    FUNCTION().PROTECTED().SIGNATURE(void, RestoreControls);
    FUNCTION().PROTECTED().SIGNATURE(void, InitializeHandles);
    FUNCTION().PROTECTED().SIGNATURE(void, SetHandlesInteractable, bool);
    FUNCTION().PROTECTED().SIGNATURE(void, BindHandlesInteractableToVisibility);
    FUNCTION().PROTECTED().SIGNATURE(void, OnChildFocused, const Ref<Widget>&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorPressed, const Input::Cursor&);
}
END_META;
// --- END META ---
