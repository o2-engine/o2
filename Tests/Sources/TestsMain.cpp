#include "o2/stdafx.h"
#include "o2/O2.h"
#include "o2/Application/Application.h"
#include <gtest/gtest.h>

using namespace o2;

int main(int argc, char** argv)
{
    INITIALIZE_O2;

    auto app = mmake<Application>();
    app->Initialize();

    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
