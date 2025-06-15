#pragma once

#include "o2/Scene/UI/Widgets/Window.h"

using namespace o2;

namespace Editor
{
    FORWARD_CLASS_REF(DockWindowPlace);

    // -----------------------------
    // Dockable editor window widget
    // -----------------------------
    class DockableWindow: public o2::Window
    {
    public:
        // Default constructor
        DockableWindow(RefCounter* refCounter);

        // Copy-constructor
        DockableWindow(RefCounter* refCounter, const DockableWindow& other);

        // Destructor
        ~DockableWindow();

        // Copy-operator
        DockableWindow& operator=(const DockableWindow& other);

        // Updates drawables, states and widget
        void Update(float dt) override;

        // Draws widget
        void Draw() override;

        // Returns is window docked
        bool IsDocked() const;

        // Return pointer to docking sample sprite
        const Ref<Sprite>& GetDockingFrameSample() const; 

        // Sets icon sprite
        void SetIcon(const Ref<Sprite>& icon) override;

        // Returns icon sprite
        Ref<Sprite> GetIcon() const override;

        // Sets icon layer layout
        void SetIconLayout(const Layout& layout) override;

        // Returns icon layer layout
        Layout GetIconLayout() const override;

        // Sets caption text
        void SetCaption(const WString& caption) override;

        // Returns caption text
        WString GetCaption() const override;

        // Sets tab size
        void SetTabWidth(float width);

        // Sets tab width
        float GetTabWidth() const;

        // Sets current window tab as active tab
        void SetTabActive();

        // Returns is this window active and tabbed and tab is active
        bool IsTabActive() const;

        // Sets Automatically calculating tab width when changing caption
        void SetAutoCalcuclatingTabWidth(bool enable);

        // Is Automatically calculating tab width when changing caption
        bool IsAutoCalcuclatingTabWidth() const;

        // Places this into empty dock
        void PlaceDock(const Ref<DockWindowPlace>& targetDock);

        // Undocks this window, using when window moved out from dock
        void Undock();

        // Updates layout
        void UpdateSelfTransform() override;

        // Returns true if point is under drawable
        bool IsUnderPoint(const Vec2F& point) override;

        // Returns create menu category in editor
        static String GetCreateMenuCategory();

        SERIALIZABLE(DockableWindow);
        CLONEABLE_REF(DockableWindow);

    protected:
        const String mTabLayerPath = "tab/main";
        const String mTabIconLayerPath = "tab/main/icon";
        const String mTabCaptionLayerPath = "tab/main/caption";
        const String mIconLayerPath = "back/icon";
        const String mCaptionLayerPath = "back/caption";

        const float mDockSizeCoef = 0.2f;
        const float mDockBorder = 1.5f;

        bool        mDocked = false;      // Is window docked
        Ref<Sprite> mDockingFrameSample;  // Docking frame sample @SERIALIZABLE
        RectF       mDockingFrameCurrent; // Docking sample current frame
        RectF       mDockingFrameTarget;  // Docking sample target  frame

        Ref<AnimationPlayer> mDockingFrameAppearance = mmake<AnimationPlayer>(); // Docking frame appearance tween

        Vec2F mNonDockSize; // Size of widget before docking
        Vec2F mDragOffset;  // Offset from cursor to dragging anchor point

        bool  mTabState = false;             // Is window as tab
        int   mTabPosition = 0;              // Tab index
        bool  mTabActive = false;            // Is active tab
        float mTabWidth = 150.0f;            // Width of tab layer "tab/main"
        bool  mAutoCalculateTabWidth = true; // Automatically calculating tab width when changing caption

    protected:
        // Called when visible was changed
        void OnEnabled() override;

        // Called when visible was changed, undocks
        void OnDisabled() override;

        // Called when widget was selected, enables active tab
        void OnFocused() override;

        //Initialize animation for frame appearance
        void InitializeDockFrameAppearanceAnim();

        // Initializes drag handles
        void InitializeDragHandles();

        // Called when head handle double clicked, trying to dock window
        void OnHeadDblCKicked(const Input::Cursor& cursor);

        // Called when window was moved
        void OnMoved(const Input::Cursor& cursor);

        // Called when window moving was completed
        void OnMoveCompleted(const Input::Cursor&);

        // Called when window going to move
        void OnMoveBegin(const Input::Cursor&);

        // Searches dock place under cursor and returns parameters of them
        bool TraceDock(Ref<DockWindowPlace>& targetDock, Side& dockPosition, RectF& dockZoneRect);

        // Places this into target dock as non line arranged
        void PlaceNonLineDock(const Ref<DockWindowPlace>& targetDock, Side dockPosition);

        // Places this into target dock as line arranged
        void PlaceLineDock(const Ref<DockWindowPlace>& targetDock, Side dockPosition, RectF dockZoneRect);

        // Sets window as one of tab
        void SetTabState(float offset, int position, bool isFirst);

        // Sets window regular state, not as tab
        void SetNonTabState();

        // Sets tab as active
        void SetActiveTab();

        // Sets window docked
        void SetDocked(bool docked);

        // Recalculates tab width by caption size
        void RecalculateTabWidth();

        friend class WindowsLayout;
        friend class DockWindowPlace;
    };
}
// --- META ---

CLASS_BASES_META(Editor::DockableWindow)
{
    BASE_CLASS(o2::Window);
}
END_META;
CLASS_FIELDS_META(Editor::DockableWindow)
{
    FIELD().PROTECTED().DEFAULT_VALUE("tab/main").NAME(mTabLayerPath);
    FIELD().PROTECTED().DEFAULT_VALUE("tab/main/icon").NAME(mTabIconLayerPath);
    FIELD().PROTECTED().DEFAULT_VALUE("tab/main/caption").NAME(mTabCaptionLayerPath);
    FIELD().PROTECTED().DEFAULT_VALUE("back/icon").NAME(mIconLayerPath);
    FIELD().PROTECTED().DEFAULT_VALUE("back/caption").NAME(mCaptionLayerPath);
    FIELD().PROTECTED().DEFAULT_VALUE(0.2f).NAME(mDockSizeCoef);
    FIELD().PROTECTED().DEFAULT_VALUE(1.5f).NAME(mDockBorder);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mDocked);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().NAME(mDockingFrameSample);
    FIELD().PROTECTED().NAME(mDockingFrameCurrent);
    FIELD().PROTECTED().NAME(mDockingFrameTarget);
    FIELD().PROTECTED().DEFAULT_VALUE(mmake<AnimationPlayer>()).NAME(mDockingFrameAppearance);
    FIELD().PROTECTED().NAME(mNonDockSize);
    FIELD().PROTECTED().NAME(mDragOffset);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mTabState);
    FIELD().PROTECTED().DEFAULT_VALUE(0).NAME(mTabPosition);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mTabActive);
    FIELD().PROTECTED().DEFAULT_VALUE(150.0f).NAME(mTabWidth);
    FIELD().PROTECTED().DEFAULT_VALUE(true).NAME(mAutoCalculateTabWidth);
}
END_META;
CLASS_METHODS_META(Editor::DockableWindow)
{

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*, const DockableWindow&);
    FUNCTION().PUBLIC().SIGNATURE(void, Update, float);
    FUNCTION().PUBLIC().SIGNATURE(void, Draw);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsDocked);
    FUNCTION().PUBLIC().SIGNATURE(const Ref<Sprite>&, GetDockingFrameSample);
    FUNCTION().PUBLIC().SIGNATURE(void, SetIcon, const Ref<Sprite>&);
    FUNCTION().PUBLIC().SIGNATURE(Ref<Sprite>, GetIcon);
    FUNCTION().PUBLIC().SIGNATURE(void, SetIconLayout, const Layout&);
    FUNCTION().PUBLIC().SIGNATURE(Layout, GetIconLayout);
    FUNCTION().PUBLIC().SIGNATURE(void, SetCaption, const WString&);
    FUNCTION().PUBLIC().SIGNATURE(WString, GetCaption);
    FUNCTION().PUBLIC().SIGNATURE(void, SetTabWidth, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetTabWidth);
    FUNCTION().PUBLIC().SIGNATURE(void, SetTabActive);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsTabActive);
    FUNCTION().PUBLIC().SIGNATURE(void, SetAutoCalcuclatingTabWidth, bool);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsAutoCalcuclatingTabWidth);
    FUNCTION().PUBLIC().SIGNATURE(void, PlaceDock, const Ref<DockWindowPlace>&);
    FUNCTION().PUBLIC().SIGNATURE(void, Undock);
    FUNCTION().PUBLIC().SIGNATURE(void, UpdateSelfTransform);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsUnderPoint, const Vec2F&);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetCreateMenuCategory);
    FUNCTION().PROTECTED().SIGNATURE(void, OnEnabled);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDisabled);
    FUNCTION().PROTECTED().SIGNATURE(void, OnFocused);
    FUNCTION().PROTECTED().SIGNATURE(void, InitializeDockFrameAppearanceAnim);
    FUNCTION().PROTECTED().SIGNATURE(void, InitializeDragHandles);
    FUNCTION().PROTECTED().SIGNATURE(void, OnHeadDblCKicked, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnMoved, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnMoveCompleted, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnMoveBegin, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(bool, TraceDock, Ref<DockWindowPlace>&, Side&, RectF&);
    FUNCTION().PROTECTED().SIGNATURE(void, PlaceNonLineDock, const Ref<DockWindowPlace>&, Side);
    FUNCTION().PROTECTED().SIGNATURE(void, PlaceLineDock, const Ref<DockWindowPlace>&, Side, RectF);
    FUNCTION().PROTECTED().SIGNATURE(void, SetTabState, float, int, bool);
    FUNCTION().PROTECTED().SIGNATURE(void, SetNonTabState);
    FUNCTION().PROTECTED().SIGNATURE(void, SetActiveTab);
    FUNCTION().PROTECTED().SIGNATURE(void, SetDocked, bool);
    FUNCTION().PROTECTED().SIGNATURE(void, RecalculateTabWidth);
}
END_META;
// --- END META ---
