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
