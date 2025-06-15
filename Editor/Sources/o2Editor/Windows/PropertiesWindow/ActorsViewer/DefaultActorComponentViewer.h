#pragma once

#include "o2Editor/Properties/IPropertyField.h"
#include "o2Editor/Properties/PropertiesContext.h"
#include "o2Editor/Windows/PropertiesWindow/ActorsViewer/IActorComponentViewer.h"

namespace Editor
{
    FORWARD_CLASS_REF(IObjectPropertiesViewer);

    // ------------------------------
    // Default actor component viewer
    // ------------------------------
    class DefaultActorComponentViewer: public IActorComponentViewer
    {
    public:
        // Default constructor. Initializes data widget
        DefaultActorComponentViewer();

        // Virtual destructor
        ~DefaultActorComponentViewer();

        // Sets target actors
        void SetTargetComponents(const Vector<Ref<Component>>& components) override;

        // Returns viewing component type 
        const Type* GetComponentType() const override;

        // Updates all component values
        void Refresh() override;

        IOBJECT(DefaultActorComponentViewer);

    protected:
        const Type* mComponentType = nullptr; // Target component type

        Ref<IObjectPropertiesViewer> mViewer; //Component properties viewer

    protected:
        // Called when some property changed, marks Actor as changed and calls default Undo create callback
        void OnPropertyChanged(const String& path, const Vector<DataDocument>& before, 
                               const Vector<DataDocument>& after);

        // Enable viewer event function
        void OnPropertiesEnabled() override;

        // Disable viewer event function
        void OnPropertiesDisabled() override;
    };

    template<typename _component_type>
    class TActorComponentViewer: public DefaultActorComponentViewer
    {
    public:
        // Sets target actors
        void SetTargetComponents(const Vector<Ref<Component>>& components) override;

        // Returns viewing component type 
        const Type* GetComponentType() const override;

        IOBJECT(TActorComponentViewer<_component_type>);

    protected:
        Vector<Ref<_component_type>> mTargetComponents;
    };

    template<typename _component_type>
    void TActorComponentViewer<_component_type>::SetTargetComponents(const Vector<Ref<Component>>& components)
    {
        DefaultActorComponentViewer::SetTargetComponents(components);
        mTargetComponents = DynamicCastVector<_component_type>(components);
    }

    template<typename _component_type>
    const Type* TActorComponentViewer<_component_type>::GetComponentType() const
    {
        return &TypeOf(_component_type);
    }
}
// --- META ---

CLASS_BASES_META(Editor::DefaultActorComponentViewer)
{
    BASE_CLASS(Editor::IActorComponentViewer);
}
END_META;
CLASS_FIELDS_META(Editor::DefaultActorComponentViewer)
{
    FIELD().PROTECTED().DEFAULT_VALUE(nullptr).NAME(mComponentType);
    FIELD().PROTECTED().NAME(mViewer);
}
END_META;
CLASS_METHODS_META(Editor::DefaultActorComponentViewer)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().SIGNATURE(void, SetTargetComponents, const Vector<Ref<Component>>&);
    FUNCTION().PUBLIC().SIGNATURE(const Type*, GetComponentType);
    FUNCTION().PUBLIC().SIGNATURE(void, Refresh);
    FUNCTION().PROTECTED().SIGNATURE(void, OnPropertyChanged, const String&, const Vector<DataDocument>&, const Vector<DataDocument>&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnPropertiesEnabled);
    FUNCTION().PROTECTED().SIGNATURE(void, OnPropertiesDisabled);
}
END_META;

META_TEMPLATES(typename _component_type)
CLASS_BASES_META(Editor::TActorComponentViewer<_component_type>)
{
    BASE_CLASS(Editor::DefaultActorComponentViewer);
}
END_META;
META_TEMPLATES(typename _component_type)
CLASS_FIELDS_META(Editor::TActorComponentViewer<_component_type>)
{
    FIELD().PROTECTED().NAME(mTargetComponents);
}
END_META;
META_TEMPLATES(typename _component_type)
CLASS_METHODS_META(Editor::TActorComponentViewer<_component_type>)
{

    FUNCTION().PUBLIC().SIGNATURE(void, SetTargetComponents, const Vector<Ref<Component>>&);
    FUNCTION().PUBLIC().SIGNATURE(const Type*, GetComponentType);
}
END_META;
// --- END META ---
