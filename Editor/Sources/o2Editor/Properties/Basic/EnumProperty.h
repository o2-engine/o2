#pragma once

#include "o2Editor/Properties/IPropertyField.h"

namespace o2
{
    class DropDown;
}

namespace Editor
{
    // -----------------------------
    // Editor enum property dropdown
    // -----------------------------
    class EnumProperty: public TPropertyField<int>
    {
    public:
        // Default constructor
        EnumProperty(RefCounter* refCounter);

        // Copy constructor
        EnumProperty(RefCounter* refCounter, const EnumProperty& other);

        // Copy operator
        EnumProperty& operator=(const EnumProperty& other);

        // Returns editing by this field type
        const Type* GetValueType() const override;

        // Specializes field type
        void SpecializeType(const Type* type);

        // Returns editing by this field type by static function, can't be changed during runtime
        static const Type* GetValueTypeStatic();

        SERIALIZABLE(EnumProperty);
        CLONEABLE_REF(EnumProperty);

    protected:                                                     
        const EnumType*  mEnumType = nullptr; // Type of enumeration                                                                  
        Map<int, String> mEntries;            // Enum entries

        Ref<DropDown> mDropDown;              // Layer name dropdown
        bool          mUpdatingValue = false; // Is dropdown value updating and we don't we don't check selection

    protected:
        // Updates value view
        void UpdateValueView() override;

        // Searches controls widgets and layers and initializes them
        void InitializeControls();

        // Selects item
        void OnSelectedItem(const WString& name);
    };
}
// --- META ---

CLASS_BASES_META(Editor::EnumProperty)
{
    BASE_CLASS(Editor::TPropertyField<int>);
}
END_META;
CLASS_FIELDS_META(Editor::EnumProperty)
{
    FIELD().PROTECTED().DEFAULT_VALUE(nullptr).NAME(mEnumType);
    FIELD().PROTECTED().NAME(mEntries);
    FIELD().PROTECTED().NAME(mDropDown);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mUpdatingValue);
}
END_META;
CLASS_METHODS_META(Editor::EnumProperty)
{

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*, const EnumProperty&);
    FUNCTION().PUBLIC().SIGNATURE(const Type*, GetValueType);
    FUNCTION().PUBLIC().SIGNATURE(void, SpecializeType, const Type*);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(const Type*, GetValueTypeStatic);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateValueView);
    FUNCTION().PROTECTED().SIGNATURE(void, InitializeControls);
    FUNCTION().PROTECTED().SIGNATURE(void, OnSelectedItem, const WString&);
}
END_META;
// --- END META ---
