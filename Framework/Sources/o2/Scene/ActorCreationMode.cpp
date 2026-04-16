#include "o2/stdafx.h"
#include "ActorCreationMode.h"

#include "o2/Utils/Reflection/Type.h"
// --- META ---

ENUM_META(o2::ActorCreateMode, o2__ActorCreateMode)
{
    ENUM_ENTRY(Default);
    ENUM_ENTRY(InScene);
    ENUM_ENTRY(NotInScene);
}
END_ENUM_META;
// --- END META ---
