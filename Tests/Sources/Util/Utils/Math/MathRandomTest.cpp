#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Utils/Math/Math.h"

using namespace o2;

// A scoped generator makes Math::Random deterministic for its lifetime
TEST(MathRandom, ScopeIsDeterministicPerSeed)
{
    Vector<float> first, second, other;

    {
        Math::RandomScope scope(42);
        for (int i = 0; i < 5; i++)
            first.Add(Math::Random(0.0f, 1.0f));
    }
    {
        Math::RandomScope scope(42);
        for (int i = 0; i < 5; i++)
            second.Add(Math::Random(0.0f, 1.0f));
    }
    {
        Math::RandomScope scope(43);
        for (int i = 0; i < 5; i++)
            other.Add(Math::Random(0.0f, 1.0f));
    }

    EXPECT_EQ(first, second);
    EXPECT_NE(first, other);

    for (auto value : first)
    {
        EXPECT_GE(value, 0.0f);
        EXPECT_LE(value, 1.0f);
    }
}

// Scopes nest and restore the previous source; the global rand() sequence is not consumed by them
TEST(MathRandom, ScopesNestAndLeaveGlobalRandUntouched)
{
    srand(7);
    int expected = rand();

    srand(7);
    {
        Math::RandomScope outer(1);
        float a1 = Math::Random(0.0f, 1.0f);
        {
            Math::RandomScope inner(2);
            Math::Random(0.0f, 1.0f);
            Math::Random(0.0f, 1.0f);
        }
        float a2 = Math::Random(0.0f, 1.0f);

        Math::RandomScope check(1);
        EXPECT_FLOAT_EQ(Math::Random(0.0f, 1.0f), a1);
        EXPECT_FLOAT_EQ(Math::Random(0.0f, 1.0f), a2) << "the outer scope continues its own sequence after the inner one";
    }

    EXPECT_EQ(rand(), expected);
}
