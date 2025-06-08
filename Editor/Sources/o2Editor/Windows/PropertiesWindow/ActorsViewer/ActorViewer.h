#pragma once

#include "o2Editor/Windows/PropertiesWindow/IPropertiesViewer.h"

using namespace o2;

namespace o2
{
    class Actor;
    class Button;
    class VerticalLayout;
    class Widget;
}

namespace Editor
{
    FORWARD_CLASS_REF(AddComponentPanel);
    FORWARD_CLASS_REF(DefaultActorComponentViewer);
    FORWARD_CLASS_REF(DefaultActorPropertiesViewer);
    FORWARD_CLASS_REF(IActorComponentViewer);
    FORWARD_CLASS_REF(IActorHeaderViewer);
    FORWARD_CLASS_REF(IActorPropertiesViewer);
    FORWARD_CLASS_REF(IActorTransformViewer);

    // ------------------------
    // Actors properties viewer
    // ------------------------
    class ActorViewer: public IPropertiesViewer
    {
    public:
        // Default constructor
        ActorViewer(RefCounter* refCounter);

        // Virtual destructor
        ~ActorViewer();

        // Returns viewing object type
        const Type* GetViewingObjectType() const override;

        // Sets header viewer
        void SetActorHeaderViewer(const Ref<IActorHeaderViewer>& viewer);

        // Sets transform viewer
        void SetActorTransformViewer(const Ref<IActorTransformViewer>& viewer);

        // Adds new available component viewer type
        void AddComponentViewerType(const Ref<IActorComponentViewer>& viewer);

        // Adds new available actor properties viewer type
        void AddActorPropertiesViewerType(const Ref<IActorPropertiesViewer>& viewer);

        // Updates properties values
        void Refresh() override;

        IOBJECT(ActorViewer);

    protected:
        typedef Map<const Type*, Vector<Ref<IActorComponentViewer>>> TypeCompViewersMap;
        typedef Map<const Type*, Ref<IActorPropertiesViewer>> TypeActorViewersMap;

        Vector<Actor*> mTargetActors; // Current target actors
                                        
        Ref<IActorHeaderViewer>    mHeaderViewer;    // Actor header viewer
        Ref<IActorTransformViewer> mTransformViewer; // Actor transform viewer

        Ref<IActorPropertiesViewer>         mActorPropertiesViewer;           // Actor properties viewer
        Ref<DefaultActorPropertiesViewer>   mDefaultActorPropertiesViewer;    // Default actor properties viewer sample
        Vector<Ref<IActorPropertiesViewer>> mAvailableActorPropertiesViewers; // Available actor properties viewers                                                                           
        TypeActorViewersMap                 mActorPropertiesViewersPool;      // Actor properties viewers pool

        Vector<Pair<const Type*, Vector<Ref<Component>>>> mComponentGroupsTypes;       // List of components group types for targets
        Vector<Ref<IActorComponentViewer>>                mComponentsViewers;          // Components viewers
        Ref<DefaultActorComponentViewer>                  mDefaultComponentViewer;     // Default component viewer sample
        Vector<Ref<IActorComponentViewer>>                mAvailableComponentsViewers; // Available components' viewers                                                                           
        TypeCompViewersMap                                mComponentViewersPool;       // Components viewers pool

        Ref<AddComponentPanel> mAddComponentPanel; // Add component panel. Shown by clicking on filter field
                                        
        Ref<VerticalLayout> mViewersLayout; // Viewers layout

    protected:
        // Called when some actors on scene were changed
        void OnSceneObjectsChanged(const Vector<Ref<SceneEditableObject>>& objects);

        // Sets target objects
        void SetTargets(const Vector<IObject*>& targets) override;

        // Sets target actor properties
        void SetTargetsActorProperties(const Vector<IObject*>& targets, Vector<Ref<Widget>>& viewersWidgets);

        // Sets target components: gets common components and initializes them
        void SetTargetsComponents(const Vector<IObject*>& targets, Vector<Ref<Widget>>& viewersWidgets);

        // Returns list of grouped by types components
        Vector<Pair<const Type*, Vector<Ref<Component>>>> GetGroupedComponents() const;

        // Enable viewer event function
        void OnPropertiesEnabled() override;

        // Disable viewer event function
        void OnPropertiesDisabled() override;

        friend class AddComponentPanel;
    };

}
// --- META ---

CLASS_BASES_META(Editor::ActorViewer)
{
    BASE_CLASS(Editor::IPropertiesViewer);
}
END_META;
CLASS_FIELDS_META(Editor::ActorViewer)
{
    FIELD().PROTECTED().NAME(mTargetActors);
    FIELD().PROTECTED().NAME(mHeaderViewer);
    FIELD().PROTECTED().NAME(mTransformViewer);
    FIELD().PROTECTED().NAME(mActorPropertiesViewer);
    FIELD().PROTECTED().NAME(mDefaultActorPropertiesViewer);
    FIELD().PROTECTED().NAME(mAvailableActorPropertiesViewers);
    FIELD().PROTECTED().NAME(mActorPropertiesViewersPool);
    FIELD().PROTECTED().NAME(mComponentGroupsTypes);
    FIELD().PROTECTED().NAME(mComponentsViewers);
    FIELD().PROTECTED().NAME(mDefaultComponentViewer);
    FIELD().PROTECTED().NAME(mAvailableComponentsViewers);
    FIELD().PROTECTED().NAME(mComponentViewersPool);
    FIELD().PROTECTED().NAME(mAddComponentPanel);
    FIELD().PROTECTED().NAME(mViewersLayout);
}
END_META;
CLASS_METHODS_META(Editor::ActorViewer)
{

    typedef Vector<Pair<const Type*, Vector<Ref<Component>>>> _tmp1;

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().SIGNATURE(const Type*, GetViewingObjectType);
    FUNCTION().PUBLIC().SIGNATURE(void, SetActorHeaderViewer, const Ref<IActorHeaderViewer>&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetActorTransformViewer, const Ref<IActorTransformViewer>&);
    FUNCTION().PUBLIC().SIGNATURE(void, AddComponentViewerType, const Ref<IActorComponentViewer>&);
    FUNCTION().PUBLIC().SIGNATURE(void, AddActorPropertiesViewerType, const Ref<IActorPropertiesViewer>&);
    FUNCTION().PUBLIC().SIGNATURE(void, Refresh);
    FUNCTION().PROTECTED().SIGNATURE(void, OnSceneObjectsChanged, const Vector<Ref<SceneEditableObject>>&);
    FUNCTION().PROTECTED().SIGNATURE(void, SetTargets, const Vector<IObject*>&);
    FUNCTION().PROTECTED().SIGNATURE(void, SetTargetsActorProperties, const Vector<IObject*>&, Vector<Ref<Widget>>&);
    FUNCTION().PROTECTED().SIGNATURE(void, SetTargetsComponents, const Vector<IObject*>&, Vector<Ref<Widget>>&);
    FUNCTION().PROTECTED().SIGNATURE(_tmp1, GetGroupedComponents);
    FUNCTION().PROTECTED().SIGNATURE(void, OnPropertiesEnabled);
    FUNCTION().PROTECTED().SIGNATURE(void, OnPropertiesDisabled);
}
END_META;
// --- END META ---
