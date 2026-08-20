#pragma once

#include "TestBase.h"

namespace game
{
#if IS_EDITOR
    // Editor-only class
    class EditorOnly: public o2::IObject
    {
    public:
        int value = 0;

        IOBJECT(EditorOnly);
    };
#endif

    class Partial: public o2::IObject
    {
    public:
        int always = 1;

#if IS_EDITOR
        int editorField = 2;

        void EditorMethod();
#endif

#ifdef DEBUG_MODE
        int debugField = 3;
#else
        int releaseField = 4;
#endif

        IOBJECT(Partial);
    };
}
