#pragma once

#include "o2Editor/Core/Properties/IPropertyField.h"

using namespace o2;

namespace Editor
{
    FORWARD_CLASS_REF(FloatProperty);

    // ----------------------------
    // Editor float border property
    // ----------------------------
    class BorderFProperty: public IPropertyField
    {
    public:
        // Default constructor
        BorderFProperty(RefCounter* refCounter);

        // Copy constructor
        BorderFProperty(RefCounter* refCounter, const BorderFProperty& other);

        // Copy operator
        BorderFProperty& operator=(const BorderFProperty& other);

        // Sets fields
        void SetValueAndPrototypeProxy(const TargetsVec& targets) override;

        // Updates and checks value
        void Refresh(bool forcible = false) override;

        // Sets value
        void SetValue(const BorderF& value);

        // Sets value left
        void SetValueLeft(float value);

        // Sets value right
        void SetValueRight(float value);

        // Sets value top
        void SetValueTop(float value);

        // Sets value bottom
        void SetValueBottom(float value);

        // Sets value as unknown
        void SetUnknownValue(const BorderF& defaultValue = BorderF());

        // Sets value left as unknown
        void SetLeftUnknownValue(float defaultValue = 0.0f);

        // Sets value right as unknown
        void SetRightUnknownValue(float defaultValue = 0.0f);

        // Sets value top as unknown
        void SetTopUnknownValue(float defaultValue = 0.0f);

        // Sets value bottom as unknown
        void SetBottomUnknownValue(float defaultValue = 0.0f);

        // Returns value
        BorderF GetCommonValue() const;

        // Returns is values different
        bool IsValuesDifferent() const;

        // Returns editing by this field type
        const Type* GetValueType() const override;

        // Returns editing by this field type by static function, can't be changed during runtime
        static const Type* GetValueTypeStatic();

        SERIALIZABLE(BorderFProperty);
        CLONEABLE_REF(BorderFProperty);

    protected:
        Ref<FloatProperty> mLeftProperty;   // Left value property
        Ref<FloatProperty> mRightProperty;  // Right value property
        Ref<FloatProperty> mTopProperty;    // Top value property
        Ref<FloatProperty> mBottomProperty; // Bottom value property

    protected:
        // Searches controls widgets and layers and initializes them
        void InitializeControls();

    protected:

        class LeftValueProxy : public IValueProxy<float>
        {
            Ref<IAbstractValueProxy> mProxy;

        public:
            LeftValueProxy();
            LeftValueProxy(const Ref<IAbstractValueProxy>& proxy);

            void SetValue(const float& value) override;
            float GetValue() const override;
        };

        class RightValueProxy : public IValueProxy<float>
        {
            Ref<IAbstractValueProxy> mProxy;

        public:
            RightValueProxy();
            RightValueProxy(const Ref<IAbstractValueProxy>& proxy);

            void SetValue(const float& value) override;
            float GetValue() const override;
        };

        class TopValueProxy : public IValueProxy<float>
        {
            Ref<IAbstractValueProxy> mProxy;

        public:
            TopValueProxy();
            TopValueProxy(const Ref<IAbstractValueProxy>& proxy);

            void SetValue(const float& value) override;
            float GetValue() const override;
        };

        class BottomValueProxy : public IValueProxy<float>
        {
            Ref<IAbstractValueProxy> mProxy;

        public:
            BottomValueProxy();
            BottomValueProxy(const Ref<IAbstractValueProxy>& proxy);

            void SetValue(const float& value) override;
            float GetValue() const override;
        };
    };
}
// --- META ---

CLASS_BASES_META(Editor::BorderFProperty)
{
    BASE_CLASS(Editor::IPropertyField);
}
END_META;
CLASS_FIELDS_META(Editor::BorderFProperty)
{
    FIELD().PROTECTED().NAME(mLeftProperty);
    FIELD().PROTECTED().NAME(mRightProperty);
    FIELD().PROTECTED().NAME(mTopProperty);
    FIELD().PROTECTED().NAME(mBottomProperty);
}
END_META;
CLASS_METHODS_META(Editor::BorderFProperty)
{

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*, const BorderFProperty&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetValueAndPrototypeProxy, const TargetsVec&);
    FUNCTION().PUBLIC().SIGNATURE(void, Refresh, bool);
    FUNCTION().PUBLIC().SIGNATURE(void, SetValue, const BorderF&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetValueLeft, float);
    FUNCTION().PUBLIC().SIGNATURE(void, SetValueRight, float);
    FUNCTION().PUBLIC().SIGNATURE(void, SetValueTop, float);
    FUNCTION().PUBLIC().SIGNATURE(void, SetValueBottom, float);
    FUNCTION().PUBLIC().SIGNATURE(void, SetUnknownValue, const BorderF&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetLeftUnknownValue, float);
    FUNCTION().PUBLIC().SIGNATURE(void, SetRightUnknownValue, float);
    FUNCTION().PUBLIC().SIGNATURE(void, SetTopUnknownValue, float);
    FUNCTION().PUBLIC().SIGNATURE(void, SetBottomUnknownValue, float);
    FUNCTION().PUBLIC().SIGNATURE(BorderF, GetCommonValue);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsValuesDifferent);
    FUNCTION().PUBLIC().SIGNATURE(const Type*, GetValueType);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(const Type*, GetValueTypeStatic);
    FUNCTION().PROTECTED().SIGNATURE(void, InitializeControls);
}
END_META;
// --- END META ---
