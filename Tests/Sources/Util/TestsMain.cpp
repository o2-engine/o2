#include <gtest/gtest.h>

extern void InitializeTypeso2UtilTests();

// Utility-tier test runner. No Application, no o2 subsystems, no asset tree load.
// Tests linked here must depend only on header-only / value-type code from o2Framework
// (Math, Vec2F, Color, Pool, Vector, Map, Curve...). Anything that touches Time, Scene,
// Assets, Input, Render, etc. belongs in o2SystemTests or o2RenderTests.
int main(int argc, char** argv)
{
    InitializeTypeso2UtilTests();

    ::testing::InitGoogleTest(&argc, argv);

    return RUN_ALL_TESTS();
}
