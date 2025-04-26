#pragma once

#include "o2/Scene/UI/Widgets/VerticalLayout.h"
#include "o2/Utils/Basic/IObject.h"
#include "o2/Utils/ValueProxy.h"
#include "o2Editor/Core/Properties/PropertiesContext.h"

using namespace o2;

namespace o2
{
    class VerticalLayout;
}

namespace Editor
{
    FORWARD_CLASS_REF(IPropertyField);

    // ----------------------------------------------------------------------------------
    // Object properties viewer interface. Used in IObjectProperty and IObjectPtrProperty
    // Override this class to create new object properties viewer
    // ----------------------------------------------------------------------------------
    class IObjectPropertiesViewer : public IObject, public RefCounterable
    {
    public:
        typedef Function<void(const Ref<IPropertyField>&)> OnChangedFunc;
        typedef Function<void(const String&, const Vector<DataDocument>&, const Vector<DataDocument>&)> OnChangeCompletedFunc;

    public:
        OnChangedFunc         onChanged;         // Immediate change value by user event
        OnChangeCompletedFunc onChangeCompleted; // Change completed by user event

        String path; // Path to viewing object fields

    public:
        // Default constructor
        IObjectPropertiesViewer();

        // Refreshing controls and properties by target objects
        void Refresh(const Vector<Pair<IObject*, IObject*>>& targetObjets);

        // Returns viewing objects base type
        virtual const Type* GetViewingObjectType() const;

        // Returns viewing objects base type by static function
        static const Type* GetViewingObjectTypeStatic();

        // Sets parent context
        void SetParentContext(const Ref<PropertiesContext>& context);

        // Returns view widget
        const Ref<Spoiler>& GetSpoiler();

		// Creates spoiler if it is not created yet
		Ref<Spoiler> CheckCreateSpoiler(const Ref<Widget>& parent);

        // Creates spoiler for properties
        virtual Ref<Spoiler> CreateSpoiler(const Ref<Widget>& parent);

        // Sets is header enabled and properties can be collapsed in spoiler
        virtual void SetHeaderEnabled(bool enabled);

        // Returns is header enabled
        bool IsHeaderEnabled() const;

        // Expands or collapses spoiler
		virtual void SetExpanded(bool expanded, bool forcible = false);

        // Returns is spoiler expanded
        bool IsExpanded() const;

        // Sets spoiler caption
        virtual void SetCaption(const WString& caption);

        // Returns caption
        const WString& GetCaption() const;

        // Returns is viewer empty
        bool IsEmpty() const;

        // ThCalled when object viewer enabled
        virtual void OnPropertiesEnabled();

        // ThCalled when object viewer disabled
        virtual void OnPropertiesDisabled();

        IOBJECT(IObjectPropertiesViewer);

    protected:
        Ref<Spoiler> mSpoiler;              // Properties spoiler. Expands forcible when viewer hasn't header
        bool         mHeaderEnabled = true; // Is header enabled and properties hiding in spoiler

        bool mPropertiesBuilt = false; // True when properties built at first refreshing

        Vector<Pair<IObject*, IObject*>> mTargetObjects; // Target objects

        const Type* mRealObjectType = &TypeOf(IObject);  // Specialized viewing object type

        Ref<PropertiesContext> mPropertiesContext; // Field properties information

        OnChangeCompletedFunc mOnChildFieldChangeCompleted; // Default field change completed callback, calls
                                                            // inChangeCompleted from this with full combined path

    protected:
        // Called when header enable changed
        virtual void OnHeaderEnableChanged(bool enabled) {}

        // Checks if properties need to be rebuilt, rebuilds if necessary; returns true when properties was rebuilt
        virtual bool CheckBuildProperties(const Vector<Pair<IObject*, IObject*>>& targetObjets);

        // Called when the viewer is refreshed, builds properties, and places them in mPropertiesContext
        virtual void RebuildProperties(const Vector<Pair<IObject*, IObject*>>& targetObjets) {}

        // Called when viewer is refreshed
        virtual void OnRefreshed(const Vector<Pair<IObject*, IObject*>>& targetObjets) {}

        // ThCalled when the viewer is freed
        virtual void OnFree() {}

        // Called when some child field were changed
        void OnFieldChangeCompleted(const String& path, const Vector<DataDocument>& before, 
                                    const Vector<DataDocument>& after);

        friend class Properties;
    };

    // --------------------------------------
    // Specialize object properties interface
    // --------------------------------------
    template<typename _object_type>
    class TObjectPropertiesViewer : public IObjectPropertiesViewer
    {
    public:
        // Returns viewing objects base type
        const Type* GetViewingObjectType() const override;

        // Returns viewing objects base type by static function
        static const Type* GetViewingObjectTypeStatic();

        IOBJECT(TObjectPropertiesViewer<_object_type>);

    protected:
        Vector<Pair<_object_type*, _object_type*>> mTypeTargetObjects; // Target objects casted to viewer type

    protected:
        // Called when viewer is refreshed
        void OnRefreshed(const Vector<Pair<IObject*, IObject*>>& targetObjets) override;
    };

    template<typename _object_type>
    const Type* TObjectPropertiesViewer<_object_type>::GetViewingObjectType() const
    {
        return GetViewingObjectTypeStatic();
    }

    template<typename _object_type>
    const Type* TObjectPropertiesViewer<_object_type>::GetViewingObjectTypeStatic()
    {
        return &TypeOf(_object_type);
    }

    template<typename _object_type>
    void TObjectPropertiesViewer<_object_type>::OnRefreshed(const Vector<Pair<IObject*, IObject*>>& targetObjets)
    {
        mTypeTargetObjects = targetObjets.Convert<Pair<_object_type*, _object_type*>>([](const auto& x) {
            return Pair<_object_type*, _object_type*>(dynamic_cast<_object_type*>(x.first), dynamic_cast<_object_type*>(x.second));
        });
    }
}
// --- META ---

CLASS_BASES_META(Editor::IObjectPropertiesViewer)
{
    BASE_CLASS(o2::IObject);
    BASE_CLASS(o2::RefCounterable);
}
END_META;
CLASS_FIELDS_META(Editor::IObjectPropertiesViewer)
{
    FIELD().PUBLIC().NAME(onChanged);
    FIELD().PUBLIC().NAME(onChangeCompleted);
    FIELD().PUBLIC().NAME(path);
    FIELD().PROTECTED().NAME(mSpoiler);
    FIELD().PROTECTED().DEFAULT_VALUE(true).NAME(mHeaderEnabled);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mPropertiesBuilt);
    FIELD().PROTECTED().NAME(mTargetObjects);
    FIELD().PROTECTED().DEFAULT_VALUE(&TypeOf(IObject)).NAME(mRealObjectType);
    FIELD().PROTECTED().NAME(mPropertiesContext);
    FIELD().PROTECTED().NAME(mOnChildFieldChangeCompleted);
}
END_META;
CLASS_METHODS_META(Editor::IObjectPropertiesViewer)
{

    typedef const Vector<Pair<IObject*, IObject*>>& _tmp1;
    typedef const Vector<Pair<IObject*, IObject*>>& _tmp2;
    typedef const Vector<Pair<IObject*, IObject*>>& _tmp3;
    typedef const Vector<Pair<IObject*, IObject*>>& _tmp4;

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().SIGNATURE(void, Refresh, _tmp1);
    FUNCTION().PUBLIC().SIGNATURE(const Type*, GetViewingObjectType);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(const Type*, GetViewingObjectTypeStatic);
    FUNCTION().PUBLIC().SIGNATURE(void, SetParentContext, const Ref<PropertiesContext>&);
    FUNCTION().PUBLIC().SIGNATURE(const Ref<Spoiler>&, GetSpoiler);
    FUNCTION().PUBLIC().SIGNATURE(Ref<Spoiler>, CheckCreateSpoiler, const Ref<Widget>&);
    FUNCTION().PUBLIC().SIGNATURE(Ref<Spoiler>, CreateSpoiler, const Ref<Widget>&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetHeaderEnabled, bool);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsHeaderEnabled);
    FUNCTION().PUBLIC().SIGNATURE(void, SetExpanded, bool, bool);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsExpanded);
    FUNCTION().PUBLIC().SIGNATURE(void, SetCaption, const WString&);
    FUNCTION().PUBLIC().SIGNATURE(const WString&, GetCaption);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsEmpty);
    FUNCTION().PUBLIC().SIGNATURE(void, OnPropertiesEnabled);
    FUNCTION().PUBLIC().SIGNATURE(void, OnPropertiesDisabled);
    FUNCTION().PROTECTED().SIGNATURE(void, OnHeaderEnableChanged, bool);
    FUNCTION().PROTECTED().SIGNATURE(bool, CheckBuildProperties, _tmp2);
    FUNCTION().PROTECTED().SIGNATURE(void, RebuildProperties, _tmp3);
    FUNCTION().PROTECTED().SIGNATURE(void, OnRefreshed, _tmp4);
    FUNCTION().PROTECTED().SIGNATURE(void, OnFree);
    FUNCTION().PROTECTED().SIGNATURE(void, OnFieldChangeCompleted, const String&, const Vector<DataDocument>&, const Vector<DataDocument>&);
}
END_META;

META_TEMPLATES(typename _object_type)
CLASS_BASES_META(Editor::TObjectPropertiesViewer<_object_type>)
{
    BASE_CLASS(Editor::IObjectPropertiesViewer);
}
END_META;
META_TEMPLATES(typename _object_type)
CLASS_FIELDS_META(Editor::TObjectPropertiesViewer<_object_type>)
{
    FIELD().PROTECTED().NAME(mTypeTargetObjects);
}
END_META;
META_TEMPLATES(typename _object_type)
CLASS_METHODS_META(Editor::TObjectPropertiesViewer<_object_type>)
{

    typedef const Vector<Pair<IObject*, IObject*>>& _tmp1;

    FUNCTION().PUBLIC().SIGNATURE(const Type*, GetViewingObjectType);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(const Type*, GetViewingObjectTypeStatic);
    FUNCTION().PROTECTED().SIGNATURE(void, OnRefreshed, _tmp1);
}
END_META;
// --- END META ---
