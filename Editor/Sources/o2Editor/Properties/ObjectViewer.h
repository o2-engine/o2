#pragma once
#include "o2/Scene/UI/Widgets/VerticalLayout.h"

using namespace o2;

namespace Editor
{
    FORWARD_CLASS_REF(IObjectPropertiesViewer);
    FORWARD_CLASS_REF(PropertiesContext);
    FORWARD_CLASS_REF(IPropertyField);

    // -----------------------------------------------------------------
    // Basic automatic object viewer. Selects suitable properties viewer
    // -----------------------------------------------------------------
    class ObjectViewer: public VerticalLayout
    {
    public:
        typedef Function<void(const Ref<IPropertyField>& field, bool byUser)> OnPropertyChangedFunc;
        typedef Function<void(const String&, const Vector<DataDocument>&, const Vector<DataDocument>&)> OnPropertyChangeCompletedFunc;

        OnPropertyChangedFunc         onPropertyChanged;         // Called when property changed
        OnPropertyChangeCompletedFunc onPropertyChangeCompleted; // Called when property change completed

    public:
        // Default constructor, creates view widget as vertical layout
        ObjectViewer(RefCounter* refCounter);

        // Refreshing controls and properties by target objects with prototypes
        void Refresh(const Vector<Pair<IObject*, IObject*>>& targetObjets);

        // Refreshing controls and properties by target objects
        void Refresh(const Vector<IObject*>& targetObjets);

        // Sets parent context
        void SetParentContext(const Ref<PropertiesContext>& context);

        // Enable viewer event function
        virtual void OnPropertiesEnabled();

        // Disable viewer event function
        virtual void OnPropertiesDisabled();

        // Returns viewing objects type
        const Type* GetViewingObjectType() const;

        // Returns create menu category in editor
        static String GetCreateMenuCategory();

        SERIALIZABLE(ObjectViewer);
        CLONEABLE_REF(ObjectViewer);

    protected:
        Ref<PropertiesContext> mParentContext; // Parent properties context

        Ref<IObjectPropertiesViewer> mPropertiesViewer; // Object properties viewer

    protected:
        // Called when some property changed
        virtual void OnPropertyChanged(const Ref<IPropertyField>& field, bool byUser);

        // Called when some property change completed
        virtual void OnPropertyChangeCompleted(const String& path, const Vector<DataDocument>& before, 
                                               const Vector<DataDocument>& after);
    };
}
// --- META ---

CLASS_BASES_META(Editor::ObjectViewer)
{
    BASE_CLASS(o2::VerticalLayout);
}
END_META;
CLASS_FIELDS_META(Editor::ObjectViewer)
{
    FIELD().PUBLIC().NAME(onPropertyChanged);
    FIELD().PUBLIC().NAME(onPropertyChangeCompleted);
    FIELD().PROTECTED().NAME(mParentContext);
    FIELD().PROTECTED().NAME(mPropertiesViewer);
}
END_META;
CLASS_METHODS_META(Editor::ObjectViewer)
{

    typedef const Vector<Pair<IObject*, IObject*>>& _tmp1;

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().SIGNATURE(void, Refresh, _tmp1);
    FUNCTION().PUBLIC().SIGNATURE(void, Refresh, const Vector<IObject*>&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetParentContext, const Ref<PropertiesContext>&);
    FUNCTION().PUBLIC().SIGNATURE(void, OnPropertiesEnabled);
    FUNCTION().PUBLIC().SIGNATURE(void, OnPropertiesDisabled);
    FUNCTION().PUBLIC().SIGNATURE(const Type*, GetViewingObjectType);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetCreateMenuCategory);
    FUNCTION().PROTECTED().SIGNATURE(void, OnPropertyChanged, const Ref<IPropertyField>&, bool);
    FUNCTION().PROTECTED().SIGNATURE(void, OnPropertyChangeCompleted, const String&, const Vector<DataDocument>&, const Vector<DataDocument>&);
}
END_META;
// --- END META ---
