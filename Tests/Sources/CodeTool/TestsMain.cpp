#include <gtest/gtest.h>

// CodeTool test runner. Standalone: no o2Framework, no reflection registry.
int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);

    return RUN_ALL_TESTS();
}
