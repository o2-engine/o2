#pragma once

#include "o2/Utils/Property.h"
#include "o2Editor/Core/Properties/IPropertyField.h"
#include "o2Editor/Core/Properties/PropertiesContext.h"
#include <unordered_map>

using namespace o2;

namespace o2
{
    class Spoiler;
}

namespace Editor
{
    FORWARD_CLASS_REF(IntegerProperty);

    // --------------------
    // ScriptValue property
    // --------------------
    class ScriptValueProperty : public IPropertyField
    {
    public:
        // Default constructor
        ScriptValueProperty(RefCounter* refCounter);

        // Copy constructor
        ScriptValueProperty(RefCounter* refCounter, const ScriptValueProperty& other);

        // Copy operator
        ScriptValueProperty& operator=(const ScriptValueProperty& other);

        // Sets targets proxies
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

        // Enables or disables spoiler header
        void SetHeaderEnabled(bool enabled);

        // Returns is spoiler header enabled
        bool IsHeaderEnabled() const;

        SERIALIZABLE(ScriptValueProperty);
        CLONEABLE_REF(ScriptValueProperty);

    protected:
        Ref<Spoiler> mSpoiler; // Properties spoiler. Expands forcible when viewer hasn't header

        Map<String, ScriptValue::ValueType> mPreviousBuiltTypes; // Built types of fields, used to check 
                                                                 // changed properties structure
        Map<String, Ref<IPropertyField>>    mBuiltProperties;    // Built properties by names

        bool mHeaderEnabled = false; // Is no header attribute exists
        bool mExpanded = false; // True when must be expanded after creating object viewer

        bool mNeedUpdateProxies = false; // True when targets changed and required to refresh script values proxies

        bool mIsArray = false; // Is value array

        Ref<IntegerProperty> mCountProperty; // Vector count property

        bool mCountDifferents = false; // Is targets counts of elements are different
        int  mCountOfElements = 0;     // Common count of elements

        Ref<HorizontalLayout> mHeaderContainer; // Count property and other controls container

        Ref<Button> mAddButton; // Add button, adds new element at end

        bool mIsRefreshing = false; // Is currently refreshing content. Need to prevent cycled size changing

    protected:
        typedef Vector<Pair<String, Vector<Pair<Ref<IScriptValueProperty>, Ref<IScriptValueProperty>>>>> PropertiesList;

        // Called when property puts in buffer. Releases properties
        void OnFreeProperty() override;

        // Searches controls widgets and layers and initializes them
        void InitializeControls();

        // Returns mapped common properties
        PropertiesList GetCommonProperties(const Vector<Pair<ScriptValue, ScriptValue>>& values, bool& isArray) const;

        //Adds property by type
        void AddProperty(const String& name, const Type* type, int idx);

        // Called when count property changing
        void OnCountChanged(const Ref<IPropertyField>& def);

        // Called when expanding spoiler, refreshing properties
        void OnExpand();

        // Sets new count of elements in vector
        void Resize(int newCount);

        // Removes element from vector
        void Remove(int idx);

        // Called when add button has pressed
        void OnAddPressed();

        // Sets property proxies
        template<typename _type>
        void SetFieldProxies(PropertiesList& commonProperties, const String& name, const Ref<IPropertyField>& field);

        // Sets property proxies
        template<typename _type>
        void SetFieldPtrProxies(PropertiesList& commonProperties, const String& name, const Ref<IPropertyField>& field);

        // Called when some property changed, sets value via proxy
        void OnPropertyChanged(const String& path, const Vector<DataDocument>& before,
                               const Vector<DataDocument>& after);
    };

    template<typename _type>
    void ScriptValueProperty::SetFieldProxies(PropertiesList& commonProperties, const String& name, 
                                              const Ref<IPropertyField>& field)
    {
        auto prop = commonProperties.Find([&](auto& x) { return x.first == name; });
        auto proxies = prop->second.template Convert<Pair<Ref<IAbstractValueProxy>, Ref<IAbstractValueProxy>>>(
            [](auto& x)
            {
                Pair<Ref<IAbstractValueProxy>, Ref<IAbstractValueProxy>> res;
                res.first = mmake<TypeScriptValueProxy<_type>>(x.first->Clone());
                if (x.second && x.second->Get().IsObject())
                    res.second = mmake<TypeScriptValueProxy<_type>>(x.second->Clone());

                return res;
            });

        field->SetValueAndPrototypeProxy(proxies);
    }

    template<typename _type>
    void ScriptValueProperty::SetFieldPtrProxies(PropertiesList& commonProperties, const String& name,
                                                 const Ref<IPropertyField>& field)
    {
        auto prop = commonProperties.Find([&](auto& x) { return x.first == name; });
        auto proxies = prop->second.template Convert<Pair<Ref<IAbstractValueProxy>, Ref<IAbstractValueProxy>>>(
            [](auto& x)
            {
                Pair<Ref<IAbstractValueProxy>, Ref<IAbstractValueProxy>> res;
                res.first = mmake<PtrScriptValueProxy<_type>>(x.first->Clone());
                if (x.second && x.second->Get().IsObject())
                    res.second = mmake<PtrScriptValueProxy<_type>>(x.second->Clone());

                return res;
            });

        field->SetValueAndPrototypeProxy(proxies);
    }
}
// --- META ---

CLASS_BASES_META(Editor::ScriptValueProperty)
{
    BASE_CLASS(Editor::IPropertyField);
}
END_META;
CLASS_FIELDS_META(Editor::ScriptValueProperty)
{
    FIELD().PROTECTED().NAME(mSpoiler);
    FIELD().PROTECTED().NAME(mPreviousBuiltTypes);
    FIELD().PROTECTED().NAME(mBuiltProperties);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mHeaderEnabled);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mExpanded);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mNeedUpdateProxies);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mIsArray);
    FIELD().PROTECTED().NAME(mCountProperty);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mCountDifferents);
    FIELD().PROTECTED().DEFAULT_VALUE(0).NAME(mCountOfElements);
    FIELD().PROTECTED().NAME(mHeaderContainer);
    FIELD().PROTECTED().NAME(mAddButton);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mIsRefreshing);
}
END_META;
CLASS_METHODS_META(Editor::ScriptValueProperty)
{

    typedef const Vector<Pair<ScriptValue, ScriptValue>>& _tmp1;

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*, const ScriptValueProperty&);
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
    FUNCTION().PUBLIC().SIGNATURE(void, SetHeaderEnabled, bool);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsHeaderEnabled);
    FUNCTION().PROTECTED().SIGNATURE(void, OnFreeProperty);
    FUNCTION().PROTECTED().SIGNATURE(void, InitializeControls);
    FUNCTION().PROTECTED().SIGNATURE(PropertiesList, GetCommonProperties, _tmp1, bool&);
    FUNCTION().PROTECTED().SIGNATURE(void, AddProperty, const String&, const Type*, int);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCountChanged, const Ref<IPropertyField>&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnExpand);
    FUNCTION().PROTECTED().SIGNATURE(void, Resize, int);
    FUNCTION().PROTECTED().SIGNATURE(void, Remove, int);
    FUNCTION().PROTECTED().SIGNATURE(void, OnAddPressed);
    FUNCTION().PROTECTED().SIGNATURE(void, OnPropertyChanged, const String&, const Vector<DataDocument>&, const Vector<DataDocument>&);
}
END_META;
// --- END META ---
