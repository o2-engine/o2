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
// --- META ---

#if  IS_EDITOR
CLASS_BASES_META(game::EditorOnly)
{
    BASE_CLASS(o2::IObject);
}
END_META;
CLASS_FIELDS_META(game::EditorOnly)
{
#if  IS_EDITOR
    FIELD().PUBLIC().DEFAULT_VALUE(0).NAME(value);
#endif
}
END_META;
CLASS_METHODS_META(game::EditorOnly)
{
}
END_META;
#endif

CLASS_BASES_META(game::Partial)
{
    BASE_CLASS(o2::IObject);
}
END_META;
CLASS_FIELDS_META(game::Partial)
{
    FIELD().PUBLIC().DEFAULT_VALUE(1).NAME(always);
#if  IS_EDITOR
    FIELD().PUBLIC().DEFAULT_VALUE(2).NAME(editorField);
#endif
#if defined  DEBUG_MODE
    FIELD().PUBLIC().DEFAULT_VALUE(3).NAME(debugField);
#endif
#if !(defined  DEBUG_MODE)
    FIELD().PUBLIC().DEFAULT_VALUE(4).NAME(releaseField);
#endif
}
END_META;
CLASS_METHODS_META(game::Partial)
{

#if  IS_EDITOR
    FUNCTION().PUBLIC().SIGNATURE(void, EditorMethod);
#endif
}
END_META;
// --- END META ---
