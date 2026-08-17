#pragma once
#include "o2/Utils/Serialization/Serializable.h"

namespace test
{
    class Defines: public o2::ISerializable
    {
    public:
        int alwaysField = 1; // @SERIALIZABLE

#if IS_EDITOR
        int editorField = 2; // @SERIALIZABLE
#endif

#ifdef PLATFORM_WINDOWS
        int windowsField = 3; // @SERIALIZABLE
#else
        int otherPlatformField = 4; // @SERIALIZABLE
#endif

        SERIALIZABLE(Defines);
    };
}
// --- META ---

CLASS_BASES_META(test::Defines)
{
    BASE_CLASS(o2::ISerializable);
}
END_META;
CLASS_FIELDS_META(test::Defines)
{
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(1).NAME(alwaysField);
#if  IS_EDITOR
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(2).NAME(editorField);
#endif
#if defined(PLATFORM_WINDOWS)
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(3).NAME(windowsField);
#endif
#if !(defined(PLATFORM_WINDOWS))
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(4).NAME(otherPlatformField);
#endif
}
END_META;
CLASS_METHODS_META(test::Defines)
{
}
END_META;
// --- END META ---
