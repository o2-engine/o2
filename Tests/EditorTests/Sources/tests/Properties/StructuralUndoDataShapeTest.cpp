#include "o2Editor/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Scene/Actor.h"
#include "o2/Scene/Components/EditorTestComponent.h"
#include "o2/Utils/Reflection/Reflection.h"
#include "o2/Utils/Reflection/Type.h"
#include "o2Editor/Actions/PropertyChange.h"
#include "support/EditorTestScene.h"

using namespace o2;
using namespace Editor;
using namespace Editor::Tests;

// Undo data-shape for structural property edits: full-field serialization round-trips via the same
// GetFieldPtr + FieldInfo::(De)serialize path PropertyChangeAction uses.

namespace
{
    void* ResolveField(const Ref<EditorTestComponent>& comp, const String& path, const FieldInfo*& fi)
    {
        fi = nullptr;
        auto objectType = dynamic_cast<const ObjectType*>(&comp->GetType());
        if (!objectType)
            return nullptr;

        void* realTypeObject = objectType->DynamicCastFromIObject(dynamic_cast<IObject*>(comp.Get()));
        return objectType->GetFieldPtr(realTypeObject, path, fi);
    }
}

// element count after undoing "add first element" (empty -> 1)
static int CountAfterUndoOfFirstAdd(const Ref<EditorTestComponent>& comp, const String& field)
{
    const FieldInfo* fi = nullptr;
    void* ptr = ResolveField(comp, field, fi);
    if (!fi)
        return -1;
    auto vectorType = dynamic_cast<const VectorType*>(fi->GetType());
    if (!vectorType)
        return -1;

    vectorType->SetObjectVectorSize(ptr, 0);                  // empty
    DataDocument before; vectorType->Serialize(ptr, before);

    vectorType->SetObjectVectorSize(ptr, 1);                  // add first default element (Resize(1))
    DataDocument after; vectorType->Serialize(ptr, after);
    EXPECT_TRUE(before != after) << "[" << field << "] add-first must produce a serialization diff";

    fi->Deserialize(ptr, before);                            // Undo
    return vectorType->GetObjectVectorSize(ptr);
}

TEST(StructuralUndoDataShape, VectorAddFirstElement_UndoClears_Int)      { EXPECT_EQ(CountAfterUndoOfFirstAdd(mmake<EditorTestComponent>(), "mIntVector"), 0); }
TEST(StructuralUndoDataShape, VectorAddFirstElement_UndoClears_Object)   { EXPECT_EQ(CountAfterUndoOfFirstAdd(mmake<EditorTestComponent>(), "mTestInsideVector"), 0); }
TEST(StructuralUndoDataShape, VectorAddFirstElement_UndoClears_ActorRef) { EXPECT_EQ(CountAfterUndoOfFirstAdd(mmake<EditorTestComponent>(), "mActorVector"), 0); }
TEST(StructuralUndoDataShape, VectorAddFirstElement_UndoClears_Asset)    { EXPECT_EQ(CountAfterUndoOfFirstAdd(mmake<EditorTestComponent>(), "mAssetsVector"), 0); }

TEST(StructuralUndoDataShape, VectorResize_FullSerializeRoundTrips)
{
    auto comp = mmake<EditorTestComponent>();
    comp->mIntVector = { 1, 2, 3 };

    const FieldInfo* fi = nullptr;
    void* ptr = ResolveField(comp, "mIntVector", fi);
    ASSERT_NE(fi, nullptr);
    auto vectorType = dynamic_cast<const VectorType*>(fi->GetType());

    DataDocument before; vectorType->Serialize(ptr, before);  // size 3
    vectorType->SetObjectVectorSize(ptr, 1);
    DataDocument after; vectorType->Serialize(ptr, after);    // size 1

    fi->Deserialize(ptr, before);                             // Undo
    EXPECT_EQ(comp->mIntVector.Count(), 3);
    EXPECT_EQ(comp->mIntVector[2], 3);

    fi->Deserialize(ptr, after);                              // Redo
    EXPECT_EQ(comp->mIntVector.Count(), 1);
}

// end-to-end through a real PropertyChangeAction on a component vector via the "component/<type>/<field>" path
TEST(StructuralUndoDataShape, VectorResizeFirstElement_RealActionOnComponent)
{
    SceneCleanGuard guard;

    auto actor = MakeActor();
    Ref<EditorTestComponent> comp = actor->AddComponent<EditorTestComponent>();
    TickScene();

    auto compType = dynamic_cast<const ObjectType*>(&comp->GetType());
    ASSERT_NE(compType, nullptr);
    const FieldInfo* fi = nullptr;
    void* realComp = compType->DynamicCastFromIObject(dynamic_cast<IObject*>(comp.Get()));
    void* vecPtr = compType->GetFieldPtr(realComp, "mIntVector", fi);
    ASSERT_NE(fi, nullptr);

    comp->mIntVector = {};
    DataDocument before; fi->Serialize(vecPtr, before);
    comp->mIntVector.Add(0);
    DataDocument after; fi->Serialize(vecPtr, after);
    ASSERT_EQ(comp->mIntVector.Count(), 1);

    String path = "component/" + comp->GetType().GetName() + "/mIntVector";

    auto action = mmake<PropertyChangeAction>(AsEditable({ actor }), path,
                                              Vector<DataDocument>{ before },
                                              Vector<DataDocument>{ after });

    action->Undo();
    EXPECT_EQ(comp->mIntVector.Count(), 0) << "Undo of the first added element must clear the vector (path: " << path << ")";

    action->Redo();
    EXPECT_EQ(comp->mIntVector.Count(), 1);
}

TEST(StructuralUndoDataShape, ColorGradientKeyChange_FullSerializeRoundTrips)
{
    auto comp = mmake<EditorTestComponent>();
    comp->mGradient = mmake<ColorGradient>();
    comp->mGradient->InsertKey(0.0f, Color4::Red());
    comp->mGradient->InsertKey(1.0f, Color4::Blue());

    const FieldInfo* fi = nullptr;
    void* ptr = ResolveField(comp, "mGradient", fi);
    ASSERT_NE(fi, nullptr);
    ASSERT_NE(ptr, nullptr);

    DataDocument before; fi->Serialize(ptr, before);
    int keysBefore = comp->mGradient->GetKeys().Count();

    comp->mGradient->InsertKey(0.5f, Color4::Green());   // emulate AddNewKey
    DataDocument after; fi->Serialize(ptr, after);

    EXPECT_TRUE(before != after);
    EXPECT_EQ(comp->mGradient->GetKeys().Count(), keysBefore + 1);

    fi->Deserialize(ptr, before);                        // Undo
    EXPECT_EQ(comp->mGradient->GetKeys().Count(), keysBefore);

    fi->Deserialize(ptr, after);                         // Redo
    EXPECT_EQ(comp->mGradient->GetKeys().Count(), keysBefore + 1);
}
