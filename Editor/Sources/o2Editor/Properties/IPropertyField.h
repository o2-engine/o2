#pragma once

#include "o2/Scene/UI/Widgets/HorizontalLayout.h"
#include "o2/Utils/Basic/IObject.h"
#include "o2/Utils/Reflection/Reflection.h"
#include "o2/Utils/Serialization/DataValue.h"
#include "o2/Utils/Types/Containers/Vector.h"
#include "o2/Utils/ValueProxy.h"

using namespace o2;

namespace o2
{
    class Widget;
    class Button;
}

namespace Editor
{
    FORWARD_CLASS_REF(PropertiesContext);

    // -------------------------------
    // Editor property field interface
    // -------------------------------
    class IPropertyField : public HorizontalLayout
    {
    public:
        typedef Pair<Ref<IAbstractValueProxy>, Ref<IAbstractValueProxy>> TargetPair;
        typedef Vector<Pair<Ref<IAbstractValueProxy>, Ref<IAbstractValueProxy>>> TargetsVec;

        typedef Function<void(const Ref<IPropertyField>&)> OnChangedFunc;
        typedef Function<void(const String&, const Vector<DataDocument>&, const Vector<DataDocument>&)> OnChangeCompletedFunc;

    public:
        OnChangedFunc         onChanged;         // Immediate change value by user event
        OnChangeCompletedFunc onChangeCompleted; // Change completed by user event

    public:
        // Default constructor. Searches revert button and sets click action
        IPropertyField(RefCounter* refCounter);

        // Copy-constructor
        IPropertyField(RefCounter* refCounter, const IPropertyField& other);

        // Virtual destructor
        virtual ~IPropertyField();

        // Copy-operator
        IPropertyField& operator=(const IPropertyField& other);

        // Sets targets proxies
        virtual void SetValueAndPrototypeProxy(const TargetsVec& targets);

        // Returns targets proxies
        const TargetsVec& GetValueAndPrototypeProxy() const;

        // Sets targets proxies
        virtual void SetValueProxy(const Vector<Ref<IAbstractValueProxy>>& targets);

        // Sets parent context
        virtual void SetParentContext(const Ref<PropertiesContext>& context);

		// Checks common value and fill fields. Forcible flags disables value caching
        virtual void Refresh(bool forcible = false) {}

        // Reverts value to prototype value
        virtual void Revert() {}

        // Sets property caption
        virtual void SetCaption(const WString& text);

        // Returns property caption
        virtual WString GetCaption() const;

        // Returns remove button, creates if required
        virtual Ref<Button> GetRemoveButton();

        // Returns editing by this field type
        virtual const Type* GetValueType() const;

        // Returns editing by this field type by static function, can't be changed during runtime
        static const Type* GetValueTypeStatic();

        // Returns is values different
        bool IsValuesDifferent() const;

        // Sets reflection path f target values
        void SetValuePath(const String& path);

        // Returns reflection path of target values
        const String& GetValuePath() const;

        // Sets caption
        void SetCaptionLabel(const Ref<Label>& label);

        // Returns caption
        const Ref<Label>& GetCaptionLabel() const;

        // Sets is property revertable - it's able to check difference between origin value and prototype
        void SetRevertable(bool revertable);

        // Returns is property revertable - it's able to check difference between origin value and prototype
        bool IsRevertable() const;

        // Sets property enabled or disabled
        void SetPropertyEnabled(bool enabled);

        // Returns is property enabled
        bool IsPropertyEnabled() const;

        // Specializes field info
        virtual void SetFieldInfo(const FieldInfo* fieldInfo);

        // Sets targets pointers
        template<typename _type>
        void SetValuePointers(const Vector<_type*>& targets);

        // Sets targets pointers
        template<typename _property_type>
        void SetValuePropertyPointers(const Vector<_property_type*>& targets);

        // Sets targets pointers
        template<typename _type, typename _object_type>
        void SelectValuesPointers(const Vector<_object_type*>& targets, std::function<_type* (_object_type*)> getter);

        // Sets targets properties
        template<typename _object_type, typename _property_type>
        void SelectValuesProperties(const Vector<_object_type*>& targets,
                                    std::function<_property_type* (_object_type*)> getter);

        // Sets targets pointers
        template<typename _type, typename _object_type>
        void SelectValueAndPrototypePointers(const Vector<_object_type*>& targets,
                                             const Vector<_object_type*>& prototypes,
                                             std::function<_type* (_object_type*)> getter);

        // Sets targets properties
        template<typename _object_type, typename _property_type>
        void SelectValueAndPrototypeProperties(const Vector<_object_type*>& targets,
                                               const Vector<_object_type*>& prototypes,
                                               std::function<_property_type* (_object_type*)> getter);

        // Sets targets properties
        template<typename _type, typename _object_type>
        void SelectValueAndPrototypeFunctional(const Vector<_object_type*>& targets,
                                               const Vector<_object_type*>& prototypes,
                                               std::function<_type(_object_type*)> getter,
                                               std::function<void(_object_type*, _type)> setter);

        // Returns create menu category in editor
        static String GetCreateMenuCategory();

        SERIALIZABLE(IPropertyField);
        CLONEABLE_REF(IPropertyField);

    protected:
        bool mPropertyEnabled = false; // Is property enabled

        const FieldInfo*           mFieldInfo = nullptr; // Specialized field info
        WeakRef<PropertiesContext> mParentContext;       // Parent context
        bool                       mRevertable = true;   // Is property can be reverted

        Vector<Pair<Ref<IAbstractValueProxy>, Ref<IAbstractValueProxy>>> mValuesProxies; // Target values proxies

        bool mValuesDifferent = true; // Are values different

        Ref<Button> mRevertBtn; // Revert to source prototype button
        Ref<Button> mRemoveBtn; // Remove from array button
        Ref<Label>  mCaption;   // Caption label, null by default   

        String               mValuesPath;         // Reflection path of target values
        Vector<DataDocument> mBeforeChangeValues; // Serialized value data before changes started

    protected:
        // Called when type specialized during setting value proxy
        virtual void OnTypeSpecialized(const Type& type) {}

        // Called when property puts in buffer. Here you can release your shared resources
        virtual void OnFreeProperty();

        // Stores values to data
        virtual void StoreValues(Vector<DataDocument>& data) const {}

        // Checks that value was changed and calls onChangeCompleted
        virtual void CheckValueChangeCompleted();

        // Checks value for reverting to prototype and sets widget state "revert"
        virtual void CheckRevertableState();

        // Checks is value can be reverted
        virtual bool IsValueRevertable() const;

        // Called when field value changed
        virtual void OnValueChanged();

        // Enable property event function
        virtual void OnPropertyEnabled() {}

        // Disable property event function
        virtual void OnPropertyDisabled() {}

        // Frees values proxies
        void FreeValuesProxies();

        // Called when user began to change value and we need to store initial value data
        void BeginUserChanging();

        // Called when user completed changing value and we need to compare current value with previous and call onChangeCompleted
        void EndUserChanging();

        // Sets value via proxy
        template<typename T>
        void SetProxy(const Ref<IAbstractValueProxy>& proxy, const T& value) const;

        // Returns value from proxy
        template<typename T>
        T GetProxy(const Ref<IAbstractValueProxy>& proxy) const;

        friend class Properties;
    };

    template<typename _type>
    class TPropertyField : public IPropertyField
    {
    public:
        // Default constructor
        TPropertyField(RefCounter* refCounter);

        // Copy-constructor
        TPropertyField(RefCounter* refCounter, const TPropertyField& other);

        // COpy operator
        TPropertyField& operator=(const TPropertyField& other);

        // Checks common value and fill fields
        void Refresh(bool forcible = false) override;

        // Reverts value to prototype value
        void Revert() override;

        // Returns editing by this field type
        const Type* GetValueType() const override;

        // Returns editing by this field type by static function, can't be changed during runtime
        static const Type* GetValueTypeStatic();

        // Sets value
        void SetValue(const _type& value);

        // Sets value as unknown
        void SetUnknownValue(const _type& defaultValue = _type());

        // Returns value
        _type GetCommonValue() const;

        IOBJECT(TPropertyField<_type>);

    protected:
        _type mCommonValue = _type();

        const Type* mRealType = nullptr;

    protected:
        // Called when type specialized during setting value proxy
        void OnTypeSpecialized(const Type& type) override;

        // Checks is value can be reverted
        bool IsValueRevertable() const override;

        // Stores values to data
        void StoreValues(Vector<DataDocument>& data) const override;

        // Returns value from proxy
        virtual _type GetProxy(const Ref<IAbstractValueProxy>& proxy) const;

        // Sets value to proxy
        virtual void SetProxy(const Ref<IAbstractValueProxy>& proxy, const _type& value);

        // Sets common value
        virtual void SetCommonValue(const _type& value);

        // Sets value, checks value changed, calls onChangeCompleted
        void SetValueByUser(const _type& value);

        // Updates value view
        virtual void UpdateValueView() {}

        // Is required refresh view every time
        virtual bool IsAlwaysRefresh() const;
    };

    // -----------------------------
    // IPropertyField implementation
    // -----------------------------

    template<typename T>
    T IPropertyField::GetProxy(const Ref<IAbstractValueProxy>& proxy) const
    {
        T res;
        proxy->GetValuePtr(&res);
        return res;
    }

    template<typename T>
    void IPropertyField::SetProxy(const Ref<IAbstractValueProxy>& proxy, const T& value) const
    {
        proxy->SetValuePtr(&const_cast<T&>(value));
    }


    // ----------------------------------
    // SelectPropertyMixin implementation
    // ----------------------------------

    template<typename _type>
    void IPropertyField::SetValuePointers(const Vector<_type*>& targets)
    {
        SetValueAndPrototypeProxy(targets.template Convert<Pair<Ref<IAbstractValueProxy>, Ref<IAbstractValueProxy>>>(
            [](_type* target)
            {
                return Pair<Ref<IAbstractValueProxy>, Ref<IAbstractValueProxy>>(mmake<PointerValueProxy<_type>>(target), nullptr);
            }));
    }

    template<typename _property_type>
    void IPropertyField::SetValuePropertyPointers(const Vector<_property_type*>& targets)
    {
        SetValueAndPrototypeProxy(targets.template Convert<Pair<Ref<IAbstractValueProxy>, Ref<IAbstractValueProxy>>>(
            [](_property_type* target)
            {
                return Pair<Ref<IAbstractValueProxy>, Ref<IAbstractValueProxy>>(
                    mmake<PropertyValueProxy<typename _property_type::valueType, _property_type>>(target), nullptr);
            }));
    }

    template<typename _type, typename _object_type>
    void IPropertyField::SelectValuesPointers(const Vector<_object_type*>& targets, std::function<_type* (_object_type*)> getter)
    {
        SetValueAndPrototypeProxy(targets.template Convert<Pair<Ref<IAbstractValueProxy>, Ref<IAbstractValueProxy>>>(
            [&](_object_type* target)
            {
                return Pair<Ref<IAbstractValueProxy>, Ref<IAbstractValueProxy>>(
                    TypeOf(_type).GetValueProxy(getter(target)), nullptr);
            }));
    }

    template<typename _object_type, typename _property_type>
    void IPropertyField::SelectValuesProperties(const Vector<_object_type*>& targets,
                                                std::function<_property_type* (_object_type*)> getter)
    {
        SetValueAndPrototypeProxy(targets.template Convert<Pair<Ref<IAbstractValueProxy>, Ref<IAbstractValueProxy>>>(
            [&](_object_type* target)
            {
                return Pair<Ref<IAbstractValueProxy>, Ref<IAbstractValueProxy>>(
                    mmake<PropertyValueProxy<typename _property_type::valueType, _property_type>>(getter(target)), nullptr);
            }));
    }

    template<typename _type, typename _object_type>
    void IPropertyField::SelectValueAndPrototypePointers(const Vector<_object_type*>& targets,
                                                         const Vector<_object_type*>& prototypes,
                                                         std::function<_type* (_object_type*)> getter)
    {
        Vector<Pair<Ref<IAbstractValueProxy>, Ref<IAbstractValueProxy>>> targetPairs;
        targetPairs.Reserve(targets.Count());

        for (int i = 0; i < targets.Count() && i < prototypes.Count(); i++)
        {
            targetPairs.Add(Pair<Ref<IAbstractValueProxy>, Ref<IAbstractValueProxy>>(
                mmake<PointerValueProxy<_type>>(getter(targets[i])),
                prototypes[i] ? mmake<PointerValueProxy<_type>>(getter(prototypes[i])) : nullptr
            ));
        }

        SetValueAndPrototypeProxy(targetPairs);
    }

    template<typename _object_type, typename _property_type>
    void IPropertyField::SelectValueAndPrototypeProperties(const Vector<_object_type*>& targets,
                                                           const Vector<_object_type*>& prototypes,
                                                           std::function<_property_type* (_object_type*)> getter)
    {
        Vector<Pair<Ref<IAbstractValueProxy>, Ref<IAbstractValueProxy>>> targetPairs;
        targetPairs.Reserve(targets.Count());

        for (int i = 0; i < targets.Count() && i < prototypes.Count(); i++)
        {
            targetPairs.Add(Pair<Ref<IAbstractValueProxy>, Ref<IAbstractValueProxy>>(
                mmake<PropertyValueProxy<typename _property_type::valueType, _property_type>>(getter(targets[i])),
                prototypes[i] ? mmake<PropertyValueProxy<typename _property_type::valueType, _property_type>>(getter(prototypes[i])) : nullptr
            ));
        }

        SetValueAndPrototypeProxy(targetPairs);
    }

    template<typename _type, typename _object_type>
    void IPropertyField::SelectValueAndPrototypeFunctional(const Vector<_object_type*>& targets,
                                                           const Vector<_object_type*>& prototypes,
                                                           std::function<_type(_object_type*)> getter,
                                                           std::function<void(_object_type*, _type)> setter)
    {
        Vector<Pair<Ref<IAbstractValueProxy>, Ref<IAbstractValueProxy>>> targetPairs;
        targetPairs.Reserve(targets.Count());

        for (int i = 0; i < targets.Count() && i < prototypes.Count(); i++)
        {
            targetPairs.Add(Pair<Ref<IAbstractValueProxy>, Ref<IAbstractValueProxy>>(
                mmake<FunctionalValueProxy<_type>>([=](_type v) { setter(targets[i], v); }, [=]() { return getter(targets[i]); }),
                prototypes[i] ? mmake<FunctionalValueProxy<_type>>([=](_type v) { setter(prototypes[i], v); }, [=]() { return getter(prototypes[i]); }) : nullptr));
        }

        SetValueAndPrototypeProxy(targetPairs);
    }



    // -----------------------------
    // TPropertyField implementation
    // -----------------------------

    template<typename _type>
    TPropertyField<_type>::TPropertyField(RefCounter* refCounter):
        IPropertyField(refCounter)
    {}

    template<typename _type>
    TPropertyField<_type>::TPropertyField(RefCounter* refCounter, const TPropertyField& other) :
        IPropertyField(refCounter, other)
    {}

    template<typename _type>
    TPropertyField<_type>& Editor::TPropertyField<_type>::operator=(const TPropertyField& other)
    {
        IPropertyField::operator=(other);
        return *this;
    }

    template<typename _type>
    void TPropertyField<_type>::Refresh(bool forcible /*= false*/)
    {
        if (mValuesProxies.IsEmpty())
            return;

        auto lastCommonValue = mCommonValue;
        bool lastDifferent = mValuesDifferent;

        _type newCommonValue = GetProxy(mValuesProxies[0].first);

        bool newDifferent = false;

        for (int i = 1; i < mValuesProxies.Count(); i++)
        {
            _type value = GetProxy(mValuesProxies[i].first);
            if (newCommonValue != value)
            {
                newDifferent = true;
                break;
            }
        }

        if (newDifferent)
        {
            if (!lastDifferent)
                SetUnknownValue();
        }
        else if (lastCommonValue != newCommonValue || lastDifferent || IsAlwaysRefresh() || forcible)
            SetCommonValue(newCommonValue);

        CheckRevertableState();
    }

    template<typename _type>
    void TPropertyField<_type>::OnTypeSpecialized(const Type& type)
    {
        mRealType = &type;
    }

    template<typename _type>
    bool TPropertyField<_type>::IsAlwaysRefresh() const
    {
        return false;
    }

    template<typename _type>
    void TPropertyField<_type>::SetValueByUser(const _type& value)
    {
        StoreValues(mBeforeChangeValues);
        SetValue(value);
        CheckValueChangeCompleted();
    }

    template<typename _type>
    void TPropertyField<_type>::SetCommonValue(const _type& value)
    {
        mCommonValue = value;
        mValuesDifferent = false;

        UpdateValueView();
        OnValueChanged();
    }

    template<typename _type>
    void TPropertyField<_type>::StoreValues(Vector<DataDocument>& data) const
    {
        data.Clear();
        for (auto& ptr : mValuesProxies)
        {
            data.Add(DataDocument());
            data.Last() = GetProxy(ptr.first);
        }
    }

    template<typename _type>
    bool TPropertyField<_type>::IsValueRevertable() const
    {
        for (auto& ptr : mValuesProxies)
        {
            if (ptr.second && !Math::Equals(GetProxy(ptr.first), GetProxy(ptr.second)))
                return true;
        }

        return false;
    }

    template<typename _type>
    _type TPropertyField<_type>::GetCommonValue() const
    {
        return mCommonValue;
    }

    template<typename _type>
    void TPropertyField<_type>::SetUnknownValue(const _type& defaultValue /*= _type()*/)
    {
        mCommonValue = defaultValue;
        mValuesDifferent = true;

        UpdateValueView();
        OnValueChanged();
    }

    template<typename _type>
    void TPropertyField<_type>::SetValue(const _type& value)
    {
        for (auto& ptr : mValuesProxies)
            SetProxy(ptr.first, value);

        SetCommonValue(value);
    }

    template<typename _type>
    const Type* TPropertyField<_type>::GetValueType() const
    {
        return GetValueTypeStatic();
    }

    template<typename _type>
    const Type* TPropertyField<_type>::GetValueTypeStatic()
    {
        return &TypeOf(_type);
    }

    template<typename _type>
    void TPropertyField<_type>::Revert()
    {
        for (auto& ptr : mValuesProxies)
        {
            if (ptr.second)
                SetProxy(ptr.first, GetProxy(ptr.second));
        }

        Refresh();
    }

    template<typename _type>
    _type TPropertyField<_type>::GetProxy(const Ref<IAbstractValueProxy>& proxy) const
    {
        if constexpr (std::is_polymorphic<_type>::value)
        {
            if (mRealType && mRealType->GetUsage() == Type::Usage::Object && TypeOf(_type) != *mRealType)
            {
                auto objectType = dynamic_cast<const ObjectType*>(mRealType);
                void* typeSample = objectType->CreateSample();
                IObject* typeSampleObject = objectType->DynamicCastToIObject(typeSample);
                proxy->GetValuePtr(typeSample);
                _type res = *dynamic_cast<_type*>(typeSampleObject);
                delete typeSampleObject;
                return res;
            }
        }

        return IPropertyField::GetProxy<_type>(proxy);
    }

    template<typename _type>
    void TPropertyField<_type>::SetProxy(const Ref<IAbstractValueProxy>& proxy, const _type& value)
    {
        IPropertyField::SetProxy<_type>(proxy, value);
    }

}
// --- META ---

CLASS_BASES_META(Editor::IPropertyField)
{
    BASE_CLASS(o2::HorizontalLayout);
}
END_META;
CLASS_FIELDS_META(Editor::IPropertyField)
{
    FIELD().PUBLIC().NAME(onChanged);
    FIELD().PUBLIC().NAME(onChangeCompleted);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mPropertyEnabled);
    FIELD().PROTECTED().DEFAULT_VALUE(nullptr).NAME(mFieldInfo);
    FIELD().PROTECTED().NAME(mParentContext);
    FIELD().PROTECTED().DEFAULT_VALUE(true).NAME(mRevertable);
    FIELD().PROTECTED().NAME(mValuesProxies);
    FIELD().PROTECTED().DEFAULT_VALUE(true).NAME(mValuesDifferent);
    FIELD().PROTECTED().NAME(mRevertBtn);
    FIELD().PROTECTED().NAME(mRemoveBtn);
    FIELD().PROTECTED().NAME(mCaption);
    FIELD().PROTECTED().NAME(mValuesPath);
    FIELD().PROTECTED().NAME(mBeforeChangeValues);
}
END_META;
CLASS_METHODS_META(Editor::IPropertyField)
{

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*, const IPropertyField&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetValueAndPrototypeProxy, const TargetsVec&);
    FUNCTION().PUBLIC().SIGNATURE(const TargetsVec&, GetValueAndPrototypeProxy);
    FUNCTION().PUBLIC().SIGNATURE(void, SetValueProxy, const Vector<Ref<IAbstractValueProxy>>&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetParentContext, const Ref<PropertiesContext>&);
    FUNCTION().PUBLIC().SIGNATURE(void, Refresh, bool);
    FUNCTION().PUBLIC().SIGNATURE(void, Revert);
    FUNCTION().PUBLIC().SIGNATURE(void, SetCaption, const WString&);
    FUNCTION().PUBLIC().SIGNATURE(WString, GetCaption);
    FUNCTION().PUBLIC().SIGNATURE(Ref<Button>, GetRemoveButton);
    FUNCTION().PUBLIC().SIGNATURE(const Type*, GetValueType);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(const Type*, GetValueTypeStatic);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsValuesDifferent);
    FUNCTION().PUBLIC().SIGNATURE(void, SetValuePath, const String&);
    FUNCTION().PUBLIC().SIGNATURE(const String&, GetValuePath);
    FUNCTION().PUBLIC().SIGNATURE(void, SetCaptionLabel, const Ref<Label>&);
    FUNCTION().PUBLIC().SIGNATURE(const Ref<Label>&, GetCaptionLabel);
    FUNCTION().PUBLIC().SIGNATURE(void, SetRevertable, bool);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsRevertable);
    FUNCTION().PUBLIC().SIGNATURE(void, SetPropertyEnabled, bool);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsPropertyEnabled);
    FUNCTION().PUBLIC().SIGNATURE(void, SetFieldInfo, const FieldInfo*);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetCreateMenuCategory);
    FUNCTION().PROTECTED().SIGNATURE(void, OnTypeSpecialized, const Type&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnFreeProperty);
    FUNCTION().PROTECTED().SIGNATURE(void, StoreValues, Vector<DataDocument>&);
    FUNCTION().PROTECTED().SIGNATURE(void, CheckValueChangeCompleted);
    FUNCTION().PROTECTED().SIGNATURE(void, CheckRevertableState);
    FUNCTION().PROTECTED().SIGNATURE(bool, IsValueRevertable);
    FUNCTION().PROTECTED().SIGNATURE(void, OnValueChanged);
    FUNCTION().PROTECTED().SIGNATURE(void, OnPropertyEnabled);
    FUNCTION().PROTECTED().SIGNATURE(void, OnPropertyDisabled);
    FUNCTION().PROTECTED().SIGNATURE(void, FreeValuesProxies);
    FUNCTION().PROTECTED().SIGNATURE(void, BeginUserChanging);
    FUNCTION().PROTECTED().SIGNATURE(void, EndUserChanging);
}
END_META;

META_TEMPLATES(typename _type)
CLASS_BASES_META(Editor::TPropertyField<_type>)
{
    BASE_CLASS(Editor::IPropertyField);
}
END_META;
META_TEMPLATES(typename _type)
CLASS_FIELDS_META(Editor::TPropertyField<_type>)
{
    FIELD().PROTECTED().DEFAULT_VALUE(_type()).NAME(mCommonValue);
    FIELD().PROTECTED().DEFAULT_VALUE(nullptr).NAME(mRealType);
}
END_META;
META_TEMPLATES(typename _type)
CLASS_METHODS_META(Editor::TPropertyField<_type>)
{

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*, const TPropertyField&);
    FUNCTION().PUBLIC().SIGNATURE(void, Refresh, bool);
    FUNCTION().PUBLIC().SIGNATURE(void, Revert);
    FUNCTION().PUBLIC().SIGNATURE(const Type*, GetValueType);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(const Type*, GetValueTypeStatic);
    FUNCTION().PUBLIC().SIGNATURE(void, SetValue, const _type&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetUnknownValue, const _type&);
    FUNCTION().PUBLIC().SIGNATURE(_type, GetCommonValue);
    FUNCTION().PROTECTED().SIGNATURE(void, OnTypeSpecialized, const Type&);
    FUNCTION().PROTECTED().SIGNATURE(bool, IsValueRevertable);
    FUNCTION().PROTECTED().SIGNATURE(void, StoreValues, Vector<DataDocument>&);
    FUNCTION().PROTECTED().SIGNATURE(_type, GetProxy, const Ref<IAbstractValueProxy>&);
    FUNCTION().PROTECTED().SIGNATURE(void, SetProxy, const Ref<IAbstractValueProxy>&, const _type&);
    FUNCTION().PROTECTED().SIGNATURE(void, SetCommonValue, const _type&);
    FUNCTION().PROTECTED().SIGNATURE(void, SetValueByUser, const _type&);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateValueView);
    FUNCTION().PROTECTED().SIGNATURE(bool, IsAlwaysRefresh);
}
END_META;
// --- END META ---
