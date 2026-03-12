#include "o2/stdafx.h"
#include "o2/O2.h"
#include "o2/Application/Application.h"
#include <gtest/gtest.h>

extern void InitializeTypeso2Tests();

using namespace o2;

int main(int argc, char** argv)
{
    InitializeTypeso2Tests();
    INITIALIZE_O2;

    auto app = mmake<Application>();
    app->Initialize();

    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
