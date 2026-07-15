#pragma once

#include "o2/Utils/Basic/IObject.h"
#include "o2/Utils/Singleton.h"
#include "o2Editor/Properties/IPropertyField.h"
#include "o2Editor/Windows/IEditorWindow.h"
#include "o2Editor/Windows/PropertiesWindow/PropertiesViewerSelector.h"

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
        Function<void(const Vector<IObject*>& targets, const Ref<IPropertyField>& field, bool byUser)> onPropertyChanged; // Called when property changed

        Function<void(const Vector<IObject*>& targets, const String& path, 
                      const Vector<DataDocument>& before, const Vector<DataDocument>& after)> onPropertyChangeCompleted; // Called when property change completed

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

        Ref<IPropertiesViewer> mCurrentViewer; // Current properties viewer

        PropertiesViewerSelector mViewersSelector; // Owns available viewers and picks one by target type @IGNORE

        Function<void()> mOnTargetsChangedDelegate; // Called when targets array changing
        bool             mTargetsChanged = false;   // True when targets was changed    

        float mRefreshDelay = 0.1f;         // Values refreshing delay
        float mRefreshRemainingTime = 0.5f; // Time to next values refreshing

    protected:
        // Initializes window
        void InitializeWindow();

        // Initializes window context menu
        void InitializeWindowContext();

        // Called when private fields visibility changed
        void OnPrivateFieldsVisibleChanged(bool visible);

        // Called when some property field was changed
        void OnPropertyChanged(const Vector<IObject*>& targets, const Ref<IPropertyField>& field, bool byUser);

        // Called when some property change completed
        void OnPropertyChangeCompleted(const Vector<IObject*>& targets, const String& path, const Vector<DataDocument>& before, 
                                       const Vector<DataDocument>& after);

    public:
        // Deserializes the values into the targets' fields by the property path. Action-mode
        // property fields never write their proxies, so non-scene targets (asset metas etc.)
        // are applied here directly instead of through the scene undo action
        static void ApplyPropertyToObjects(const Vector<IObject*>& targets, const String& path,
                                           const Vector<DataDocument>& values);

    protected:

		// Called when window was focused, calls focus delegate from scene tree
        void OnFocusedWindow();
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
    FIELD().PUBLIC().NAME(onPropertyChanged);
    FIELD().PUBLIC().NAME(onPropertyChangeCompleted);
    FIELD().PROTECTED().NAME(mTargets);
    FIELD().PROTECTED().NAME(mCurrentViewer);
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
    FUNCTION().PROTECTED().SIGNATURE(void, OnPrivateFieldsVisibleChanged, bool);
    FUNCTION().PROTECTED().SIGNATURE(void, OnPropertyChanged, const Vector<IObject*>&, const Ref<IPropertyField>&, bool);
    FUNCTION().PROTECTED().SIGNATURE(void, OnPropertyChangeCompleted, const Vector<IObject*>&, const String&, const Vector<DataDocument>&, const Vector<DataDocument>&);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(void, ApplyPropertyToObjects, const Vector<IObject*>&, const String&, const Vector<DataDocument>&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnFocusedWindow);
}
END_META;
// --- END META ---
