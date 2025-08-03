#pragma once

#include "o2/Events/KeyboardEventsListener.h"
#include "o2/Scene/UI/Widgets/ScrollArea.h"

namespace o2
{
    // ----------------------------------------------------------------------------------------
    // Popup widget. Draws on top of all UI, can be opened only one. Closes other automatically
    // ----------------------------------------------------------------------------------------
    class PopupWidget: public ScrollArea, public KeyboardEventsListener
    {
    public:
        bool fitByChildren = true; // Fit size when opening by children @SERIALIZABLE

    public:
        // Default constructor
        explicit PopupWidget(RefCounter* refCounter);

        // Copy-constructor
        PopupWidget(RefCounter* refCounter, const PopupWidget& other);

        // Destructor
        ~PopupWidget();

        // Copy operator
        PopupWidget& operator=(const PopupWidget& other);

        // Updates drawables, states and widget
        void Update(float dt) override;

        // Draws widget
        void Draw() override;

        // Show from parent context
        virtual void Show(const Ref<PopupWidget>& parent, const Vec2F& position = o2Input.GetCursorPos());

        // Shows context
        void Show(const Vec2F& position = o2Input.GetCursorPos());

        // Sets minimal fitting size
        void SetMinFitSize(float size);

        // Returns is listener scrollable
        bool IsScrollable() const override;

        // Returns true when input events can be handled by down listeners
        bool IsInputTransparent() const override;

        // Fits size by items
        virtual void FitSizeAndPosition(const Vec2F& position);

        // Returns the layer on which this popup was opened
        const WeakRef<CursorAreaEventListenersLayer>& GetOpenedLayer() const;

        // Returns create menu group in editor
        static String GetCreateMenuGroup();

        SERIALIZABLE(PopupWidget);
        CLONEABLE_REF(PopupWidget);

    protected:
        static WeakRef<PopupWidget> mVisiblePopup; // Current visible popup widget

        float mFitSizeMin = 40.0f;    // Minimal fitting size @SERIALIZABLE

        WeakRef<PopupWidget> mParentPopup; // Parent visible popup widget
        WeakRef<PopupWidget> mChildPopup;  // Child visible popup widget

        bool mShownAtFrame = false; // Is popup was shown at current frame

        WeakRef<CursorAreaEventListenersLayer> mOpenedLayer; // Layer on which this popup was opened

    protected:
        // Checks widget clipping by area
        void CheckClipping(const RectF& clipArea) override;

        // Called when visible was changed
        void OnEnabled() override;

        // Called when visible was changed
        void OnDisabled() override;

        // Fits position by size and screen
        virtual void FitPosition(const Vec2F& position, Vec2F size);

        // Returns content size for fitting by children
        virtual Vec2F GetContentSize() const;

        // Hides context with his parent
        virtual void HideWithParent();

        // Hides context with his child
        virtual void HideWithChild();

        // Special drawing for contexts
        virtual void SpecialDraw();

        // Called when cursor pressing was broken (when scrolled scroll area or some other)
        void OnCursorPressBreak(const Input::Cursor& cursor) override;

        // Called when cursor released outside this(only when cursor pressed this at previous time)
        void OnCursorReleasedOutside(const Input::Cursor& cursor) override;

        // Called when key was released
        void OnKeyPressed(const Input::Key& key) override;

        REF_COUNTERABLE_IMPL(RefCounterable);

        friend class ContextMenu;
        friend class MenuPanel;
        friend class UIManager;
    };
}
// --- META ---

CLASS_BASES_META(o2::PopupWidget)
{
    BASE_CLASS(o2::ScrollArea);
    BASE_CLASS(o2::KeyboardEventsListener);
}
END_META;
CLASS_FIELDS_META(o2::PopupWidget)
{
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(true).NAME(fitByChildren);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(40.0f).NAME(mFitSizeMin);
    FIELD().PROTECTED().NAME(mParentPopup);
    FIELD().PROTECTED().NAME(mChildPopup);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mShownAtFrame);
    FIELD().PROTECTED().NAME(mOpenedLayer);
}
END_META;
CLASS_METHODS_META(o2::PopupWidget)
{

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*, const PopupWidget&);
    FUNCTION().PUBLIC().SIGNATURE(void, Update, float);
    FUNCTION().PUBLIC().SIGNATURE(void, Draw);
    FUNCTION().PUBLIC().SIGNATURE(void, Show, const Ref<PopupWidget>&, const Vec2F&);
    FUNCTION().PUBLIC().SIGNATURE(void, Show, const Vec2F&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetMinFitSize, float);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsScrollable);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsInputTransparent);
    FUNCTION().PUBLIC().SIGNATURE(void, FitSizeAndPosition, const Vec2F&);
    FUNCTION().PUBLIC().SIGNATURE(const WeakRef<CursorAreaEventListenersLayer>&, GetOpenedLayer);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetCreateMenuGroup);
    FUNCTION().PROTECTED().SIGNATURE(void, CheckClipping, const RectF&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnEnabled);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDisabled);
    FUNCTION().PROTECTED().SIGNATURE(void, FitPosition, const Vec2F&, Vec2F);
    FUNCTION().PROTECTED().SIGNATURE(Vec2F, GetContentSize);
    FUNCTION().PROTECTED().SIGNATURE(void, HideWithParent);
    FUNCTION().PROTECTED().SIGNATURE(void, HideWithChild);
    FUNCTION().PROTECTED().SIGNATURE(void, SpecialDraw);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorPressBreak, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorReleasedOutside, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnKeyPressed, const Input::Key&);
}
END_META;
// --- END META ---
