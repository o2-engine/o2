#pragma once

#include "o2Editor/Core/Properties/IPropertyField.h"
#include "o2/Events/CursorEventsArea.h"
#include "o2/Events/KeyboardEventsListener.h"

using namespace o2;

namespace Editor
{
    FORWARD_CLASS_REF(IntegerProperty);

    // ----------------------------
    // Editor int vector property
    // ----------------------------
    class Vec2IProperty : public IPropertyField
    {
    public:
        // Default constructor
        Vec2IProperty(RefCounter* refCounter);

        // Copy constructor
        Vec2IProperty(RefCounter* refCounter, const Vec2IProperty& other);

        // Copy operator
        Vec2IProperty& operator=(const Vec2IProperty& other);

        // Sets fields
        void SetValueAndPrototypeProxy(const TargetsVec& targets) override;

        // Updates and checks value
        void Refresh(bool forcible = false) override;

        // Reverts value to prototype value
        void Revert() override;

        // Sets value
        void SetValue(const Vec2I& value);

        // Sets value X
        void SetValueX(int value);

        // Sets value Y
        void SetValueY(int value);

        // Sets value as unknown
        void SetUnknownValue(const Vec2I& defaultValue = Vec2I());

        // Sets value X as unknown
        void SetXUnknownValue(int defaultValue = 0.0f);

        // Sets value Y as unknown
        void SetYUnknownValue(int defaultValue = 0.0f);

        // Returns value
        Vec2I GetCommonValue() const;

        // Returns is values different
        bool IsValuesDifferent() const;

        // Returns editing by this field type
        const Type* GetValueType() const override;

        // Returns editing by this field type by static function, can't be changed during runtime
        static const Type* GetValueTypeStatic();

        SERIALIZABLE(Vec2IProperty);
        CLONEABLE_REF(Vec2IProperty);

    protected:
        Ref<IntegerProperty> mXProperty; // X value property
        Ref<IntegerProperty> mYProperty; // Y value property

    protected:
        // Searches controls widgets and layers and initializes them
        void InitializeControls();

    protected:
        class XValueProxy : public IValueProxy<int>
        {
            Ref<IAbstractValueProxy> mProxy;

        public:
            XValueProxy();
            XValueProxy(const Ref<IAbstractValueProxy>& proxy);

            void SetValue(const int& value) override;
            int GetValue() const override;
        };

        class YValueProxy : public IValueProxy<int>
        {
            Ref<IAbstractValueProxy> mProxy;

        public:
            YValueProxy();
            YValueProxy(const Ref<IAbstractValueProxy>& proxy);

            void SetValue(const int& value) override;
            int GetValue() const override;
        };
    };
}
// --- META ---

CLASS_BASES_META(Editor::Vec2IProperty)
{
    BASE_CLASS(Editor::IPropertyField);
}
END_META;
CLASS_FIELDS_META(Editor::Vec2IProperty)
{
    FIELD().PROTECTED().NAME(mXProperty);
    FIELD().PROTECTED().NAME(mYProperty);
}
END_META;
CLASS_METHODS_META(Editor::Vec2IProperty)
{

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*, const Vec2IProperty&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetValueAndPrototypeProxy, const TargetsVec&);
    FUNCTION().PUBLIC().SIGNATURE(void, Refresh, bool);
    FUNCTION().PUBLIC().SIGNATURE(void, Revert);
    FUNCTION().PUBLIC().SIGNATURE(void, SetValue, const Vec2I&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetValueX, int);
    FUNCTION().PUBLIC().SIGNATURE(void, SetValueY, int);
    FUNCTION().PUBLIC().SIGNATURE(void, SetUnknownValue, const Vec2I&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetXUnknownValue, int);
    FUNCTION().PUBLIC().SIGNATURE(void, SetYUnknownValue, int);
    FUNCTION().PUBLIC().SIGNATURE(Vec2I, GetCommonValue);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsValuesDifferent);
    FUNCTION().PUBLIC().SIGNATURE(const Type*, GetValueType);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(const Type*, GetValueTypeStatic);
    FUNCTION().PROTECTED().SIGNATURE(void, InitializeControls);
}
END_META;
// --- END META ---
