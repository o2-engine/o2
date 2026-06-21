#include "o2Editor/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Scene/Actor.h"
#include "o2/Utils/ValueProxy.h"
#include "o2Editor/Actions/PropertyChange.h"
#include "o2Editor/Properties/IPropertyField.h"
#include "support/EditorTestScene.h"

using namespace o2;
using namespace Editor;
using namespace Editor::Tests;

// In action mode a field must NOT write its proxy in SetValueByUserAndComplete; the value is applied
// by the PropertyChangeAction at the commit site. The proxy write path stays available as the "old
// way" (legacy mode) for composites and custom editors.

namespace
{
    template<typename T>
    class TestField : public TPropertyField<T>
    {
    public:
        TestField(RefCounter* refCounter): TPropertyField<T>(refCounter) {}

        void Commit(const T& value) { this->SetValueByUserAndComplete(value); }
        void EnableActionMode(bool enabled) { this->SetValueChangeAppliedByAction(enabled); }
    };

    template<typename T>
    Ref<IAbstractValueProxy> ProxyOf(T* backing)
    {
        return mmake<PointerValueProxy<T>>(backing);
    }
}

TEST(PropertyFieldActionApply, ActionModeDoesNotWriteProxy)
{
    int backing = 5;
    auto f = mmake<TestField<int>>();
    f->SetValueProxy({ ProxyOf(&backing) });

    int afterVal = -1;
    bool completed = false;
    f->onChangeCompleted = [&](const String&, const Vector<DataDocument>& before, const Vector<DataDocument>& after)
    {
        completed = true;
        if (!after.IsEmpty()) after[0].Get(afterVal);
    };

    f->EnableActionMode(true);
    f->Commit(42);

    EXPECT_TRUE(completed);
    EXPECT_EQ(backing, 5) << "Action mode must not write the proxy; the action applies the change.";
    EXPECT_EQ(afterVal, 42) << "Completed 'after' must carry the desired value, not the stale proxy value.";
    EXPECT_EQ(f->GetCommonValue(), 42) << "The field's common value/view must reflect the desired value.";
}

TEST(PropertyFieldActionApply, LegacyModeWritesProxy)
{
    int backing = 5;
    auto f = mmake<TestField<int>>();
    f->SetValueProxy({ ProxyOf(&backing) });
    f->onChangeCompleted = [&](const String&, const Vector<DataDocument>&, const Vector<DataDocument>&) {};

    // flag defaults to false (legacy)
    f->Commit(42);

    EXPECT_EQ(backing, 42) << "Legacy mode must write the proxy directly, as before.";
    EXPECT_EQ(f->GetCommonValue(), 42);
}

TEST(PropertyFieldActionApply, ActionModeFallsBackWhenNoConsumer)
{
    int backing = 5;
    auto f = mmake<TestField<int>>();
    f->SetValueProxy({ ProxyOf(&backing) });

    f->EnableActionMode(true);
    // onChangeCompleted intentionally not wired

    f->Commit(42);

    EXPECT_EQ(backing, 42) << "With no completion consumer, action mode must fall back to a direct proxy write.";
}

TEST(PropertyFieldActionApply, AfterValuesAlignWithBeforeAcrossMultipleTargets)
{
    int a = 1, b = 2;
    auto f = mmake<TestField<int>>();
    f->SetValueProxy({ ProxyOf(&a), ProxyOf(&b) });

    int beforeCount = -1, afterCount = -1, after0 = -1, after1 = -1;
    f->onChangeCompleted = [&](const String&, const Vector<DataDocument>& before, const Vector<DataDocument>& after)
    {
        beforeCount = before.Count();
        afterCount = after.Count();
        if (after.Count() > 0) after[0].Get(after0);
        if (after.Count() > 1) after[1].Get(after1);
    };

    f->EnableActionMode(true);
    f->Commit(7);

    EXPECT_EQ(beforeCount, 2);
    EXPECT_EQ(afterCount, 2) << "'after' must hold one document per target, aligned 1:1 with 'before'.";
    EXPECT_EQ(after0, 7);
    EXPECT_EQ(after1, 7);
    EXPECT_EQ(a, 1) << "Neither target proxy may be written in action mode.";
    EXPECT_EQ(b, 2);
}

TEST(PropertyFieldActionApply, FieldAfterValuesDriveActionApplyAndUndo)
{
    SceneCleanGuard guard;

    auto actor = MakeActor();
    TickScene();
    actor->SetName("old");

    // Drive an action-mode field over a standalone backing, capture the (before, after) it emits,
    // then feed them to a PropertyChangeAction on the actor — the same route the commit site uses.
    String backing = "old";
    auto f = mmake<TestField<String>>();
    f->SetValueProxy({ ProxyOf(&backing) });

    Vector<DataDocument> capturedBefore, capturedAfter;
    f->onChangeCompleted = [&](const String&, const Vector<DataDocument>& before, const Vector<DataDocument>& after)
    {
        capturedBefore = before;
        capturedAfter = after;
    };

    f->EnableActionMode(true);
    f->Commit("new");

    EXPECT_EQ(backing, String("old")) << "Action mode leaves the proxy untouched.";

    auto action = mmake<PropertyChangeAction>(AsEditable({ actor }), "name", capturedBefore, capturedAfter);
    action->Redo();
    EXPECT_EQ(actor->GetName(), String("new")) << "The action applies the field's 'after' value to the object.";

    action->Undo();
    EXPECT_EQ(actor->GetName(), String("old")) << "Undo restores the field's 'before' value.";

    action->Redo();
    EXPECT_EQ(actor->GetName(), String("new"));
}
