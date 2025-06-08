#pragma once

#include "o2Editor/Windows/PropertiesWindow/IPropertiesViewer.h"

using namespace o2;

namespace o2
{
    class SceneEditableObject;
    class VerticalLayout;
    class WidgetLayer;
}

namespace Editor
{
    FORWARD_CLASS_REF(IWidgetLayerHeaderViewer);
    FORWARD_CLASS_REF(IWidgetLayerLayoutViewer);
    FORWARD_CLASS_REF(IWidgetLayerPropertiesViewer);

    // ------------------------------
    // Widget layer properties viewer
    // ------------------------------
    class WidgetLayerViewer : public IPropertiesViewer
    {
    public:
        WidgetLayerViewer();

        // Virtual destructor
        ~WidgetLayerViewer();

        // Returns viewing object type
        const Type* GetViewingObjectType() const override;

        // Sets header viewer
        void SetHeaderViewer(const Ref<IWidgetLayerHeaderViewer>& viewer);

        // Sets transform viewer
        void SetLayoutViewer(const Ref<IWidgetLayerLayoutViewer>& viewer);

        // Adds new available actor properties viewer type
        void SetActorPropertiesViewer(const Ref<IWidgetLayerPropertiesViewer>& viewer);

        // Updates properties values
        void Refresh() override;

        IOBJECT(WidgetLayerViewer);

    protected:
        Vector<WidgetLayer*> mTargetLayers; // Current target layers

        Ref<IWidgetLayerHeaderViewer>     mHeaderViewer;     // Layer header viewer
        Ref<IWidgetLayerLayoutViewer>     mLayoutViewer;     // Layer layout viewer
        Ref<IWidgetLayerPropertiesViewer> mPropertiesViewer; // Layer properties viewer

        Ref<VerticalLayout> mViewersLayout; // Viewers layout

    protected:
        // Called when some actors on scene were changed
        void OnSceneObjectsChanged(const Vector<Ref<SceneEditableObject>>& objects);

        // Sets target objects
        void SetTargets(const Vector<IObject*>& targets) override;

        // Enable viewer event function
        void OnPropertiesEnabled() override;

        // Disable viewer event function
        void OnPropertiesDisabled() override;

        // Updates viewer
        void Update(float dt) override;

        // Draws something
        void Draw() override;
    };

}
// --- META ---

CLASS_BASES_META(Editor::WidgetLayerViewer)
{
    BASE_CLASS(Editor::IPropertiesViewer);
}
END_META;
CLASS_FIELDS_META(Editor::WidgetLayerViewer)
{
    FIELD().PROTECTED().NAME(mTargetLayers);
    FIELD().PROTECTED().NAME(mHeaderViewer);
    FIELD().PROTECTED().NAME(mLayoutViewer);
    FIELD().PROTECTED().NAME(mPropertiesViewer);
    FIELD().PROTECTED().NAME(mViewersLayout);
}
END_META;
CLASS_METHODS_META(Editor::WidgetLayerViewer)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().SIGNATURE(const Type*, GetViewingObjectType);
    FUNCTION().PUBLIC().SIGNATURE(void, SetHeaderViewer, const Ref<IWidgetLayerHeaderViewer>&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetLayoutViewer, const Ref<IWidgetLayerLayoutViewer>&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetActorPropertiesViewer, const Ref<IWidgetLayerPropertiesViewer>&);
    FUNCTION().PUBLIC().SIGNATURE(void, Refresh);
    FUNCTION().PROTECTED().SIGNATURE(void, OnSceneObjectsChanged, const Vector<Ref<SceneEditableObject>>&);
    FUNCTION().PROTECTED().SIGNATURE(void, SetTargets, const Vector<IObject*>&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnPropertiesEnabled);
    FUNCTION().PROTECTED().SIGNATURE(void, OnPropertiesDisabled);
    FUNCTION().PROTECTED().SIGNATURE(void, Update, float);
    FUNCTION().PROTECTED().SIGNATURE(void, Draw);
}
END_META;
// --- END META ---
