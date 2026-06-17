#include "o2Editor/stdafx.h"
#include <gtest/gtest.h>

#include "o2Editor/Properties/IPropertyField.h"
#include "support/EditorTestScene.h"

using namespace o2;
using namespace Editor;
using namespace Editor::Tests;

// A drag re-fires onBeforeChange -> BeginUserChanging every frame. BeginUserChanging must keep the
// first (drag-start) snapshot until EndUserChanging, otherwise OnPropertyChangeCompleted receives a
// before-value equal to the last frame (before ~= after) and the edit can't be undone.

namespace
{
    class MockField : public IPropertyField
    {
    public:
        MockField(RefCounter* refCounter): IPropertyField(refCounter) {}

        int value = 0;

        void StoreValues(Vector<DataDocument>& data) const override
        {
            data.Clear();
            data.Add(DataDocument());
            data.Last() = value;
        }

        void Begin() { BeginUserChanging(); }
        void End()   { EndUserChanging(); }

        int BeforeCount() const { return mBeforeChangeValues.Count(); }
        int BeforeValue() const
        {
            int v = -999;
            if (!mBeforeChangeValues.IsEmpty())
                mBeforeChangeValues[0].Get(v);
            return v;
        }
    };
}

TEST(IPropertyFieldUserChanging, BeginCapturesOnlyOncePerSession)
{
    auto f = mmake<MockField>();

    f->value = 10;
    f->Begin();          // session start, before = 10
    f->value = 20;
    f->Begin();          // guarded — must not re-capture
    f->value = 30;
    f->Begin();

    ASSERT_EQ(f->BeforeCount(), 1);
    EXPECT_EQ(f->BeforeValue(), 10)
        << "BeginUserChanging must keep the drag-start snapshot, not re-capture on each change.";
}

TEST(IPropertyFieldUserChanging, EndReopensSessionForNextEdit)
{
    auto f = mmake<MockField>();

    f->value = 10;
    f->Begin();
    f->End();            // session closed
    f->value = 50;
    f->Begin();          // new session captures fresh

    EXPECT_EQ(f->BeforeValue(), 50)
        << "After EndUserChanging a new BeginUserChanging must capture the current before-value.";
}

TEST(IPropertyFieldUserChanging, CompletionReportsDragStartBefore)
{
    auto f = mmake<MockField>();

    int completed = 0, beforeVal = -1, afterVal = -1;
    f->onChangeCompleted = [&](const String&, const Vector<DataDocument>& b, const Vector<DataDocument>& a)
    {
        completed++;
        if (!b.IsEmpty()) b[0].Get(beforeVal);
        if (!a.IsEmpty()) a[0].Get(afterVal);
    };

    f->value = 10;
    f->Begin();                                                   // drag start
    for (int v = 11; v <= 15; v++) { f->value = v; f->Begin(); }  // drag frames re-fire Begin
    f->End();                                                     // completion

    EXPECT_EQ(completed, 1);
    EXPECT_EQ(beforeVal, 10) << "Completed 'before' must be the drag-start value, not the last frame.";
    EXPECT_EQ(afterVal, 15);
}
