#include "o2Editor/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Scene/UI/UIManager.h"
#include "o2Editor/Properties/Basic/VectorProperty.h"
#include "support/EditorTestScene.h"

using namespace o2;
using namespace Editor;
using namespace Editor::Tests;

namespace
{
    class TestVectorProperty : public VectorProperty
    {
    public:
        TestVectorProperty(RefCounter* refCounter): VectorProperty(refCounter) {}
        void Add() { OnAddPressed(); }
    };
}

// adding an element drives Resize through the real widget and must emit exactly one onChangeCompleted
TEST(VectorPropertyUI, AddElementEmitsOneChangeCompleted)
{
    auto field = mmake<TestVectorProperty>();
    field->SpecializeType(&TypeOf(Vector<int>));

    Vector<int> backing;
    field->SetValuePointers(Vector<Vector<int>*>{ &backing });

    int completed = 0;
    field->onChangeCompleted = [&](const String&, const Vector<DataDocument>&, const Vector<DataDocument>&) { completed++; };

    field->Add();

    EXPECT_EQ(backing.Count(), 1);
    EXPECT_EQ(completed, 1);
}
