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
