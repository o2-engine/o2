#pragma once

#include "o2/Utils/Property.h"
#include "o2Editor/Core/Properties/IPropertyField.h"
#include "o2Editor/Core/Properties/PropertiesContext.h"

using namespace o2;

namespace o2
{
    class Button;
    class Spoiler;
    class Widget;
}

namespace Editor
{
    FORWARD_CLASS_REF(IntegerProperty);

    // ----------------------
    // Editor vector property
    // ----------------------
    class VectorProperty: public IPropertyField
    {
    public:
        // Default constructor
        VectorProperty(RefCounter* refCounter);

        // Copy constructor
        VectorProperty(RefCounter* refCounter, const VectorProperty& other);

        // Copy operator
        VectorProperty& operator=(const VectorProperty& other);

        // Destructor
        ~VectorProperty();

        // Sets fields
        void SetValueAndPrototypeProxy(const TargetsVec& targets) override;

        // Updates and checks value
        void Refresh(bool forcible = false) override;

        // Returns editing by this field type
        const Type* GetValueType() const override;

        // Sets property caption
        void SetCaption(const WString& text) override;

        // Returns property caption
        WString GetCaption() const override;

        // Adds remove button
        Ref<Button> GetRemoveButton() override;

        // Specializes vector type
        void SpecializeType(const Type* type);

        // Specializes field info, processing attributes
        void SetFieldInfo(const FieldInfo* fieldInfo) override;

        // Returns specialized vector type
        const Type* GetSpecializedType() const;

        // Expands property fields
        void Expand();

        // Collapses property field
        void Collapse();

        // Sets properties expanding
        void SetExpanded(bool expanded);

        // Returns is properties expanded
        bool IsExpanded() const;

        // Enables or disables spoiler header
        void SetHeaderEnabled(bool enabled);

        // Returns is spoiler header enabled
        bool IsHeaderEnabled() const;

        // Sets enabled or disabled elements indexes in caption
        void SetCaptionIndexesEnabled(bool enabled);

        // Returns is enabled or disabled elements indexes in caption
        bool IsCaptionIndexesEnabled() const;

        // Sets enabled or disabled elements count edit box
        void SetCountEditBoxEnabled(bool enabled);

        // Return enabled or disabled elements count edit box
        bool IsCountEditBoxEnabled() const;

        SERIALIZABLE(VectorProperty);
        CLONEABLE_REF(VectorProperty);

    protected:
        struct TargetObjectData
        {
            Ref<IAbstractValueProxy> proxy;

            void* data = nullptr;
            bool  isCreated = false;

            bool operator==(const TargetObjectData& other) const { return proxy == other.proxy; }

            void Refresh();
            void SetValue();
        };

    protected:
        Ref<Spoiler> mSpoiler; // Properties spoiler

        const VectorType* mVectorType = nullptr; // Vector type

        Vector<Pair<TargetObjectData, TargetObjectData>>  mTargetObjects; // Target objects
                                                     
        Vector<Ref<IPropertyField>> mValueProperties;     // Values properties
        Vector<Ref<IPropertyField>> mValuePropertiesPool; // Unused value properties pool

        Ref<IntegerProperty> mCountProperty; // Vector count property

        bool mCountDifferents = false; // Is targets counts of elements are different
        int  mCountOfElements = 0;     // Common count of elements

        Ref<HorizontalLayout> mHeaderContainer; // Count property and other controls container

        bool mHeaderEnabled = true;         // Is header of spoiler enabled
        bool mCaptionIndexesEnabled = true; // Is enabled or disabled elements indexes in caption
        bool mCountEditBoxEnabled = true;   // Is enabled or disabled elements count edit box

        Ref<Button> mAddButton; // Add button, adds new element at end

        bool mIsRefreshing = false; // Is currently refreshing content. Need to prevent cycled size changing

    protected:
        // Enable property event function
        void OnPropertyEnabled() override;

        // Disable property event function
        void OnPropertyDisabled() override;

        // Searches controls widgets and layers and initializes them
        void InitializeControls();

        // Returns value pointer from proxy when proxy is pointer proxy
        void* GetProxyValuePointer(const Ref<IAbstractValueProxy>& proxy) const;

        // Returns free element property
        Ref<IPropertyField> GetFreeValueProperty();

        // Frees element property
        void FreeValueProperty(const Ref<IPropertyField>& def);

        // Called when count property changing
        void OnCountChanged(const Ref<IPropertyField>& def);

        // Sets new count of elements in vector
        void Resize(int newCount);

        // Removes element from vector
        void Remove(int idx);

        // Called when add button has pressed
        void OnAddPressed();

        // Called when expanding spoiler, refreshing array properties
        void OnExpand();

        // Returns object target data from proxy. Creates copy of object when it is property proxy, or gets pointer from pointer proxy
        TargetObjectData GetObjectFromProxy(const Ref<IAbstractValueProxy>& proxy);

        // Returns element caption by number and name
        String GetElementCaption(int idx, const Vector<Pair<Ref<IAbstractValueProxy>, Ref<IAbstractValueProxy>>>& targets);

        // Tries to find object name by reflection
        String TryGetObjectName(void* object, const Type& type) const;

        // Called when some property changed, sets value via proxy
        void OnPropertyChanged(const String& path, const Vector<DataDocument>& before, 
                               const Vector<DataDocument>& after);
    };
}
// --- META ---

CLASS_BASES_META(Editor::VectorProperty)
{
    BASE_CLASS(Editor::IPropertyField);
}
END_META;
CLASS_FIELDS_META(Editor::VectorProperty)
{
    FIELD().PROTECTED().NAME(mSpoiler);
    FIELD().PROTECTED().DEFAULT_VALUE(nullptr).NAME(mVectorType);
    FIELD().PROTECTED().NAME(mTargetObjects);
    FIELD().PROTECTED().NAME(mValueProperties);
    FIELD().PROTECTED().NAME(mValuePropertiesPool);
    FIELD().PROTECTED().NAME(mCountProperty);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mCountDifferents);
    FIELD().PROTECTED().DEFAULT_VALUE(0).NAME(mCountOfElements);
    FIELD().PROTECTED().NAME(mHeaderContainer);
    FIELD().PROTECTED().DEFAULT_VALUE(true).NAME(mHeaderEnabled);
    FIELD().PROTECTED().DEFAULT_VALUE(true).NAME(mCaptionIndexesEnabled);
    FIELD().PROTECTED().DEFAULT_VALUE(true).NAME(mCountEditBoxEnabled);
    FIELD().PROTECTED().NAME(mAddButton);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mIsRefreshing);
}
END_META;
CLASS_METHODS_META(Editor::VectorProperty)
{

    typedef const Vector<Pair<Ref<IAbstractValueProxy>, Ref<IAbstractValueProxy>>>& _tmp1;

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*, const VectorProperty&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetValueAndPrototypeProxy, const TargetsVec&);
    FUNCTION().PUBLIC().SIGNATURE(void, Refresh, bool);
    FUNCTION().PUBLIC().SIGNATURE(const Type*, GetValueType);
    FUNCTION().PUBLIC().SIGNATURE(void, SetCaption, const WString&);
    FUNCTION().PUBLIC().SIGNATURE(WString, GetCaption);
    FUNCTION().PUBLIC().SIGNATURE(Ref<Button>, GetRemoveButton);
    FUNCTION().PUBLIC().SIGNATURE(void, SpecializeType, const Type*);
    FUNCTION().PUBLIC().SIGNATURE(void, SetFieldInfo, const FieldInfo*);
    FUNCTION().PUBLIC().SIGNATURE(const Type*, GetSpecializedType);
    FUNCTION().PUBLIC().SIGNATURE(void, Expand);
    FUNCTION().PUBLIC().SIGNATURE(void, Collapse);
    FUNCTION().PUBLIC().SIGNATURE(void, SetExpanded, bool);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsExpanded);
    FUNCTION().PUBLIC().SIGNATURE(void, SetHeaderEnabled, bool);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsHeaderEnabled);
    FUNCTION().PUBLIC().SIGNATURE(void, SetCaptionIndexesEnabled, bool);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsCaptionIndexesEnabled);
    FUNCTION().PUBLIC().SIGNATURE(void, SetCountEditBoxEnabled, bool);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsCountEditBoxEnabled);
    FUNCTION().PROTECTED().SIGNATURE(void, OnPropertyEnabled);
    FUNCTION().PROTECTED().SIGNATURE(void, OnPropertyDisabled);
    FUNCTION().PROTECTED().SIGNATURE(void, InitializeControls);
    FUNCTION().PROTECTED().SIGNATURE(void*, GetProxyValuePointer, const Ref<IAbstractValueProxy>&);
    FUNCTION().PROTECTED().SIGNATURE(Ref<IPropertyField>, GetFreeValueProperty);
    FUNCTION().PROTECTED().SIGNATURE(void, FreeValueProperty, const Ref<IPropertyField>&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCountChanged, const Ref<IPropertyField>&);
    FUNCTION().PROTECTED().SIGNATURE(void, Resize, int);
    FUNCTION().PROTECTED().SIGNATURE(void, Remove, int);
    FUNCTION().PROTECTED().SIGNATURE(void, OnAddPressed);
    FUNCTION().PROTECTED().SIGNATURE(void, OnExpand);
    FUNCTION().PROTECTED().SIGNATURE(TargetObjectData, GetObjectFromProxy, const Ref<IAbstractValueProxy>&);
    FUNCTION().PROTECTED().SIGNATURE(String, GetElementCaption, int, _tmp1);
    FUNCTION().PROTECTED().SIGNATURE(String, TryGetObjectName, void*, const Type&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnPropertyChanged, const String&, const Vector<DataDocument>&, const Vector<DataDocument>&);
}
END_META;
// --- END META ---
