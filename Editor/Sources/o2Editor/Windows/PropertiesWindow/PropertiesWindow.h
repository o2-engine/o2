#pragma once

#include "o2/Utils/Singleton.h"
#include "o2Editor/Properties/IPropertyField.h"
#include "o2Editor/Windows/IEditorWindow.h"

using namespace o2;

namespace o2
{
    class HorizontalLayout;
    class Label;
    class VerticalLayout;
}

// Editor properties window accessor macros
#define o2EditorPropertiesWindow PropertiesWindow::Instance()

namespace Editor
{
    FORWARD_CLASS_REF(DefaultPropertiesViewer);
    FORWARD_CLASS_REF(IPropertiesViewer);
    FORWARD_CLASS_REF(PropertiesContext);

    // ------------------------
    // Editor properties window
    // ------------------------
    class PropertiesWindow: public Singleton<PropertiesWindow>, public IEditorWindow
    {
    public:
        // Default constructor
        PropertiesWindow(RefCounter* refCounter);

        // Destructor
        ~PropertiesWindow();

        // Resets targets objects
        void ResetTargets();

        // Sets target object
        void SetTarget(IObject* target);

        // Sets target objects
        void SetTargets(const Vector<IObject*>& targets, const Function<void()>& targetsChangedDelegate = Function<void()>());

        // Returns target object
        Vector<IObject*> GetTargets() const;

        // Updates window: check next viewer and targets and updates current viewer
        void Update(float dt) override;

        // Draws current viewer
        void Draw() override;

        // Returns is targets changed
        bool IsTargetsChanged() const;

        // Dynamic cast to RefCounterable via Singleton<PropertiesWindow>
        static Ref<RefCounterable> CastToRefCounterable(const Ref<PropertiesWindow>& ref);

        IOBJECT(PropertiesWindow); 
        REF_COUNTERABLE_IMPL(IEditorWindow, Singleton<PropertiesWindow>);

    protected:
        Vector<IObject*> mTargets; // Target objects

        Ref<IPropertiesViewer>         mCurrentViewer; // Current properties viewer
        Vector<Ref<IPropertiesViewer>> mViewers;       // All available object types viewers
        Ref<DefaultPropertiesViewer>   mDefaultViewer; // Default properties viewer

        Function<void()> mOnTargetsChangedDelegate; // Called when targets array changing
        bool             mTargetsChanged = false;   // True when targets was changed    

        float mRefreshDelay = 0.1f;         // Values refreshing delay
        float mRefreshRemainingTime = 0.5f; // Time to next values refreshing

    protected:
        // Initializes window
        void InitializeWindow();

        // Initializes window context menu
        void InitializeWindowContext();

        // Initializes viewers
        void InitializeViewers();

        // Called when private fields visibility changed
        void OnPrivateFieldsVisibleChanged(bool visible);

        // Called when some property field was changed
        void OnPropertyChanged(const Ref<IPropertyField>& field);
    };
}
// --- META ---

CLASS_BASES_META(Editor::PropertiesWindow)
{
    BASE_CLASS(o2::Singleton<PropertiesWindow>);
    BASE_CLASS(Editor::IEditorWindow);
}
END_META;
CLASS_FIELDS_META(Editor::PropertiesWindow)
{
    FIELD().PROTECTED().NAME(mTargets);
    FIELD().PROTECTED().NAME(mCurrentViewer);
    FIELD().PROTECTED().NAME(mViewers);
    FIELD().PROTECTED().NAME(mDefaultViewer);
    FIELD().PROTECTED().NAME(mOnTargetsChangedDelegate);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mTargetsChanged);
    FIELD().PROTECTED().DEFAULT_VALUE(0.1f).NAME(mRefreshDelay);
    FIELD().PROTECTED().DEFAULT_VALUE(0.5f).NAME(mRefreshRemainingTime);
}
END_META;
CLASS_METHODS_META(Editor::PropertiesWindow)
{

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().SIGNATURE(void, ResetTargets);
    FUNCTION().PUBLIC().SIGNATURE(void, SetTarget, IObject*);
    FUNCTION().PUBLIC().SIGNATURE(void, SetTargets, const Vector<IObject*>&, const Function<void()>&);
    FUNCTION().PUBLIC().SIGNATURE(Vector<IObject*>, GetTargets);
    FUNCTION().PUBLIC().SIGNATURE(void, Update, float);
    FUNCTION().PUBLIC().SIGNATURE(void, Draw);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsTargetsChanged);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(Ref<RefCounterable>, CastToRefCounterable, const Ref<PropertiesWindow>&);
    FUNCTION().PROTECTED().SIGNATURE(void, InitializeWindow);
    FUNCTION().PROTECTED().SIGNATURE(void, InitializeWindowContext);
    FUNCTION().PROTECTED().SIGNATURE(void, InitializeViewers);
    FUNCTION().PROTECTED().SIGNATURE(void, OnPrivateFieldsVisibleChanged, bool);
    FUNCTION().PROTECTED().SIGNATURE(void, OnPropertyChanged, const Ref<IPropertyField>&);
}
END_META;
// --- END META ---
