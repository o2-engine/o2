#pragma once

#include "o2/Utils/Property.h"
#include "o2Editor/Core/Properties/IPropertyField.h"
#include "o2Editor/Core/Properties/PropertiesContext.h"

using namespace o2;

namespace o2
{
    class Spoiler;
}

namespace Editor
{
    FORWARD_CLASS_REF(IObjectPropertiesViewer);

    // ----------------------
    // Editor object property
    // ----------------------
    class ObjectProperty: public IPropertyField
    {
    public:
        // Default constructor
        ObjectProperty(RefCounter* refCounter);

        // Copy constructor
        ObjectProperty(RefCounter* refCounter, const ObjectProperty& other);

        // Copy operator
        ObjectProperty& operator=(const ObjectProperty& other);

        // Sets fields
        void SetValueAndPrototypeProxy(const TargetsVec& targets) override;

        // Updates and checks value
        void Refresh(bool forcible = false) override;

        // Returns editing by this field type
        const Type* GetValueType() const override;

        // Returns editing by this field type by static function, can't be changed during runtime
        static const Type* GetValueTypeStatic();

        // Specializes field info, processing attributes
        void SetFieldInfo(const FieldInfo* fieldInfo) override;

        // Sets property caption
        void SetCaption(const WString& text) override;

        // Returns property caption
        WString GetCaption() const override;

        // Adds remove button
        Ref<Button> GetRemoveButton() override;

        // Expands property fields
        void Expand();

        // Collapses property field
        void Collapse();

        // Sets properties expanding
        void SetExpanded(bool expanded);

        // Returns is properties expanded
        bool IsExpanded() const;

        SERIALIZABLE(ObjectProperty);
        CLONEABLE_REF(ObjectProperty);

    protected:
        struct TargetObjectData
        {
            Ref<IAbstractValueProxy> proxy;
            IObject* data = nullptr;
            bool isCreated = false;

            bool operator==(const TargetObjectData& other) const { return proxy == other.proxy; }

            void Refresh();
            void SetValue();
        };

    protected:                                                                     
        Vector<Pair<TargetObjectData, TargetObjectData>> mTargetObjects; // Target objects

        Ref<IObjectPropertiesViewer> mObjectViewer; // Object properties viewer

        bool mNoHeader = false;          // Is no header attribute exists
        bool mExpanded = false;          // True when must be expanded after creating object viewer

        WString mCaption; // Property caption

    protected:
        // Called when property puts in buffer. Here you can release your shared resources
        void OnFreeProperty() override;

        // Enable viewer event function
        void OnPropertyEnabled() override;

        // Disable viewer event function
        void OnPropertyDisabled() override;

        // Searches controls widgets and layers and initializes them
        void InitializeControls();

        // Checks viewer type, creates new if needed
        void CheckViewer(bool forcible = false);

        // Returns object target data from proxy. Creates copy of object when it is property proxy, or gets pointer from pointer proxy
        TargetObjectData GetObjectFromProxy(const Ref<IAbstractValueProxy>& proxy);

        // Called when some property changed, sets value via proxy
        void OnPropertyChanged(const String& path, const Vector<DataDocument>& before, 
                               const Vector<DataDocument>& after);
    };
}
// --- META ---

CLASS_BASES_META(Editor::ObjectProperty)
{
    BASE_CLASS(Editor::IPropertyField);
}
END_META;
CLASS_FIELDS_META(Editor::ObjectProperty)
{
    FIELD().PROTECTED().NAME(mTargetObjects);
    FIELD().PROTECTED().NAME(mObjectViewer);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mNoHeader);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mExpanded);
    FIELD().PROTECTED().NAME(mCaption);
}
END_META;
CLASS_METHODS_META(Editor::ObjectProperty)
{

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*, const ObjectProperty&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetValueAndPrototypeProxy, const TargetsVec&);
    FUNCTION().PUBLIC().SIGNATURE(void, Refresh, bool);
    FUNCTION().PUBLIC().SIGNATURE(const Type*, GetValueType);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(const Type*, GetValueTypeStatic);
    FUNCTION().PUBLIC().SIGNATURE(void, SetFieldInfo, const FieldInfo*);
    FUNCTION().PUBLIC().SIGNATURE(void, SetCaption, const WString&);
    FUNCTION().PUBLIC().SIGNATURE(WString, GetCaption);
    FUNCTION().PUBLIC().SIGNATURE(Ref<Button>, GetRemoveButton);
    FUNCTION().PUBLIC().SIGNATURE(void, Expand);
    FUNCTION().PUBLIC().SIGNATURE(void, Collapse);
    FUNCTION().PUBLIC().SIGNATURE(void, SetExpanded, bool);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsExpanded);
    FUNCTION().PROTECTED().SIGNATURE(void, OnFreeProperty);
    FUNCTION().PROTECTED().SIGNATURE(void, OnPropertyEnabled);
    FUNCTION().PROTECTED().SIGNATURE(void, OnPropertyDisabled);
    FUNCTION().PROTECTED().SIGNATURE(void, InitializeControls);
    FUNCTION().PROTECTED().SIGNATURE(void, CheckViewer, bool);
    FUNCTION().PROTECTED().SIGNATURE(TargetObjectData, GetObjectFromProxy, const Ref<IAbstractValueProxy>&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnPropertyChanged, const String&, const Vector<DataDocument>&, const Vector<DataDocument>&);
}
END_META;
// --- END META ---
