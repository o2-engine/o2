#include "Enums.h"

// --- META ---

ENUM_META(test::Color, test__Color)
{
    ENUM_ENTRY(Blue);
    ENUM_ENTRY(Green);
    ENUM_ENTRY(Red);
}
END_ENUM_META;

ENUM_META(test::Flags, test__Flags)
{
    ENUM_ENTRY(First);
    ENUM_ENTRY(None);
    ENUM_ENTRY(Second);
}
END_ENUM_META;

ENUM_META(test::WithEnums::Inner, test__WithEnums__Inner)
{
    ENUM_ENTRY(A);
    ENUM_ENTRY(B);
}
END_ENUM_META;

DECLARE_CLASS(test::WithEnums, test__WithEnums);
// --- END META ---
