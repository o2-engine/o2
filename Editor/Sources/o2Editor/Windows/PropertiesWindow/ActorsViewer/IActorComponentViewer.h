#pragma once

#include "o2/Utils/Basic/IObject.h"
#include "o2/Utils/Reflection/Reflection.h"
#include "o2/Utils/Reflection/Type.h"
#include "o2/Utils/Types/Containers/Vector.h"
#include "o2Editor/Properties/PropertiesContext.h"

using namespace o2;

namespace o2
{
    class Actor;
    class Button;
    class Component;
    class ContextMenu;
    class Widget;
}

namespace Editor
{
    FORWARD_CLASS_REF(SpoilerWithHead);

    // ---------------------------------------
    // Editor actor component viewer interface
    // ---------------------------------------
    class IActorComponentViewer: public IObject, virtual public RefCounterable
    {
    public:
        typedef Function<void(const Ref<IPropertyField>& field, bool byUser)> OnPropertyChangedFunc;
        typedef Function<void(const String& path, const Vector<DataDocument>& before, 
                               const Vector<DataDocument>& after)> OnPropertyChangeCompletedFunc;

        OnPropertyChangedFunc         onPropertyChanged;         // Called when property changed
        OnPropertyChangeCompletedFunc onPropertyChangeCompleted; // Called when property change completed

    public:
        // Default constructor. Initializes data widget
        IActorComponentViewer();

        // Virtual destructor
        virtual ~IActorComponentViewer();

        // Sets target actors
        virtual void SetTargetComponents(const Vector<Ref<Component>>& components);

        // Returns viewing component type 
        virtual const Type* GetComponentType() const { return nullptr; }

        // Returns data widget
        virtual Ref<Widget> GetWidget() const;

        // Expands view
        void Expand();

        // Collapse view
        void Collapse();

        // Updates all component values
        virtual void Refresh();

        // Sets viewer enabled
        void SetPropertiesEnabled(bool enabled);

        // Returns is viewer enabled
        bool IsPropertiesEnabled() const;

        IOBJECT(IActorComponentViewer);

    protected:
        Vector<Ref<Component>> mTargetComponents; // Target components

        Ref<SpoilerWithHead> mSpoiler;       // Component's spoiler
        Ref<Button>          mRemoveButton;  // Remove component button
        Ref<Button>          mOptionsButton; // Options button with dropdown menu
        Ref<ContextMenu>     mOptionsMenu;   // Options dropdown menu

        bool mPropertiesEnabled = false; // Is viewer enabled 

    protected:
        // Removes target components
        void RemoveTargetComponents();

        // Copies current component to clipboard
        void CopyComponent();

        // Cuts current component (copy + remove)
        void CutComponent();

        // Pastes component from clipboard to same actor
        void PasteComponent();

        // Enable viewer event function
        virtual void OnPropertiesEnabled() {}

        // Disable viewer event function
        virtual void OnPropertiesDisabled() {}

        // Called when some property changed
        virtual void OnPropertyChanged(const Ref<IPropertyField>& field, bool byUser);

        // Called when some property change completed
        virtual void OnPropertyChangeCompleted(const String& path, const Vector<DataDocument>& before, 
                                       const Vector<DataDocument>& after);

        friend class ActorViewer;
    };
}
// --- META ---

CLASS_BASES_META(Editor::IActorComponentViewer)
{
    BASE_CLASS(o2::IObject);
    BASE_CLASS(o2::RefCounterable);
}
END_META;
CLASS_FIELDS_META(Editor::IActorComponentViewer)
{
    FIELD().PUBLIC().NAME(onPropertyChanged);
    FIELD().PUBLIC().NAME(onPropertyChangeCompleted);
    FIELD().PROTECTED().NAME(mTargetComponents);
    FIELD().PROTECTED().NAME(mSpoiler);
    FIELD().PROTECTED().NAME(mRemoveButton);
    FIELD().PROTECTED().NAME(mOptionsButton);
    FIELD().PROTECTED().NAME(mOptionsMenu);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mPropertiesEnabled);
}
END_META;
CLASS_METHODS_META(Editor::IActorComponentViewer)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().SIGNATURE(void, SetTargetComponents, const Vector<Ref<Component>>&);
    FUNCTION().PUBLIC().SIGNATURE(const Type*, GetComponentType);
    FUNCTION().PUBLIC().SIGNATURE(Ref<Widget>, GetWidget);
    FUNCTION().PUBLIC().SIGNATURE(void, Expand);
    FUNCTION().PUBLIC().SIGNATURE(void, Collapse);
    FUNCTION().PUBLIC().SIGNATURE(void, Refresh);
    FUNCTION().PUBLIC().SIGNATURE(void, SetPropertiesEnabled, bool);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsPropertiesEnabled);
    FUNCTION().PROTECTED().SIGNATURE(void, RemoveTargetComponents);
    FUNCTION().PROTECTED().SIGNATURE(void, CopyComponent);
    FUNCTION().PROTECTED().SIGNATURE(void, CutComponent);
    FUNCTION().PROTECTED().SIGNATURE(void, PasteComponent);
    FUNCTION().PROTECTED().SIGNATURE(void, OnPropertiesEnabled);
    FUNCTION().PROTECTED().SIGNATURE(void, OnPropertiesDisabled);
    FUNCTION().PROTECTED().SIGNATURE(void, OnPropertyChanged, const Ref<IPropertyField>&, bool);
    FUNCTION().PROTECTED().SIGNATURE(void, OnPropertyChangeCompleted, const String&, const Vector<DataDocument>&, const Vector<DataDocument>&);
}
END_META;
// --- END META ---
