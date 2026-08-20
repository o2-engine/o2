#include "Enums.h"
// --- META ---

ENUM_META(Direction, Direction)
{
    ENUM_ENTRY(Down);
    ENUM_ENTRY(Left);
    ENUM_ENTRY(Right);
    ENUM_ENTRY(Up);
}
END_ENUM_META;

ENUM_META(game::Mode, game__Mode)
{
    ENUM_ENTRY(Active);
    ENUM_ENTRY(Idle);
    ENUM_ENTRY(Paused);
}
END_ENUM_META;

ENUM_META(game::Flags, game__Flags)
{
    ENUM_ENTRY(First);
    ENUM_ENTRY(Second);
}
END_ENUM_META;

ENUM_META(game::StateHolder::State, game__StateHolder__State)
{
    ENUM_ENTRY(Off);
    ENUM_ENTRY(On);
}
END_ENUM_META;

DECLARE_CLASS(game::StateHolder, game__StateHolder);
// --- END META ---
