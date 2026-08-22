#pragma once

#include "TestBase.h"

// Global enum with values and trailing comma
enum class Direction { Left, Right, Up = 10, Down, };

namespace game
{
    // Namespaced enum
    enum class Mode
    {
        Idle = 0,
        Active, // most common state
        Paused = Idle
    };

    // Old-style enum
    enum Flags
    {
        First = 1,
        Second = 2
    };

    class StateHolder: public o2::IObject
    {
    public:
        enum class State { On, Off };

        IOBJECT(StateHolder);

    private:
        enum class Hidden { A, B };
    };

    // Template class - nested enum must be skipped
    template<typename T>
    class Wrapper: public o2::IObject
    {
    public:
        enum class Kind { Value, Reference };

        T value;

        IOBJECT(Wrapper);
    };
}
// --- META ---

PRE_ENUM_META(Direction);

PRE_ENUM_META(game::Mode);

PRE_ENUM_META(game::Flags);

PRE_ENUM_META(game::StateHolder::State);

CLASS_BASES_META(game::StateHolder)
{
    BASE_CLASS(o2::IObject);
}
END_META;
CLASS_FIELDS_META(game::StateHolder)
{
}
END_META;
CLASS_METHODS_META(game::StateHolder)
{
}
END_META;

META_TEMPLATES(typename T)
CLASS_BASES_META(game::Wrapper<T>)
{
    BASE_CLASS(o2::IObject);
}
END_META;
META_TEMPLATES(typename T)
CLASS_FIELDS_META(game::Wrapper<T>)
{
    FIELD().PUBLIC().NAME(value);
}
END_META;
META_TEMPLATES(typename T)
CLASS_METHODS_META(game::Wrapper<T>)
{
}
END_META;
// --- END META ---
