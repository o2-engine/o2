#pragma once

#include "o2Editor/Properties/Basic/EnumProperty.h"
#include "o2Editor/UI/SegmentedIcons.h"

namespace Editor
{
    // ----------------------------------------------------------------------------------------------------------
    // An option of an enum shown as an icon: its value and the icon
    // ----------------------------------------------------------------------------------------------------------
    struct EnumIconOption
    {
        int    value = 0;
        String icon;

        bool operator==(const EnumIconOption& other) const { return value == other.value && icon == other.icon; }
    };

    // ----------------------------------------------------------------------------------------------------------
    // Enum property as a group of icons, one per option: a block slides under the icon of the value, none when the
    // targets differ. A click sets the option's value, through the same undoable change as the dropdown
    // of EnumProperty
    // ----------------------------------------------------------------------------------------------------------
    class EnumIconsProperty: public EnumProperty
    {
    public:
        // Default constructor
        explicit EnumIconsProperty(RefCounter* refCounter);

        // Copy constructor
        EnumIconsProperty(RefCounter* refCounter, const EnumIconsProperty& other);

        // Copy operator
        EnumIconsProperty& operator=(const EnumIconsProperty& other);

        // Makes the icons of the options
        void SetOptions(const Vector<EnumIconOption>& options);

        // Returns the options
        const Vector<EnumIconOption>& GetOptions() const;

        // Returns the index of the option with the value, -1 when there is none
        int GetOptionIndex(int value) const;

        // Returns the group of the icons
        const Ref<SegmentedIcons>& GetSegments() const;

        SERIALIZABLE(EnumIconsProperty);
        CLONEABLE_REF(EnumIconsProperty);

    protected:
        Ref<SegmentedIcons>    mSegments; // Icons of the options
        Vector<EnumIconOption> mOptions;  // Options of the icons

    protected:
        // Chooses the icon of the value
        void UpdateValueView() override;

        // Finds the group of the icons
        void InitializeSegments();
    };
}
// --- META ---

CLASS_BASES_META(Editor::EnumIconsProperty)
{
    BASE_CLASS(Editor::EnumProperty);
}
END_META;
CLASS_FIELDS_META(Editor::EnumIconsProperty)
{
    FIELD().PROTECTED().NAME(mSegments);
    FIELD().PROTECTED().NAME(mOptions);
}
END_META;
CLASS_METHODS_META(Editor::EnumIconsProperty)
{

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*, const EnumIconsProperty&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetOptions, const Vector<EnumIconOption>&);
    FUNCTION().PUBLIC().SIGNATURE(const Vector<EnumIconOption>&, GetOptions);
    FUNCTION().PUBLIC().SIGNATURE(int, GetOptionIndex, int);
    FUNCTION().PUBLIC().SIGNATURE(const Ref<SegmentedIcons>&, GetSegments);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateValueView);
    FUNCTION().PROTECTED().SIGNATURE(void, InitializeSegments);
}
END_META;
// --- END META ---
