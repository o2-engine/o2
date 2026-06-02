#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Scene/Actor.h"
#include "o2/Scene/UI/Widgets/Label.h" // IPropertiesViewer.h -> IPropertyField.h uses Ref<Label> without forward-declaring it
#include "o2/Utils/Function/Function.h"
#include "o2/Utils/Serialization/DataValue.h"
#include "o2Editor/Windows/PropertiesWindow/IPropertiesViewer.h"

using namespace o2;
using namespace Editor;

namespace
{
    // PropertiesWindow subscribes its handlers to the viewer's onPropertyChanged /
    // onPropertyChangeCompleted events (and unsubscribes on viewer switch). The real window
    // can't be built headless, so these tests drive that same contract on the viewer side:
    // a member-function subscriber added/removed with MakeFunction, exactly like the window's
    // THIS_FUNC(...) += / -=. The stub exposes the protected trigger methods.
    class CallbackStubViewer : public IPropertiesViewer
    {
    public:
        explicit CallbackStubViewer(RefCounter* refCounter): IPropertiesViewer(refCounter) {}

        using IPropertiesViewer::OnPropertyChanged;
        using IPropertiesViewer::OnPropertyChangeCompleted;
    };

    struct ChangeRecorder
    {
        int              calls = 0;
        Vector<IObject*> targets;
        bool             byUser = false;

        void OnChanged(const Vector<IObject*>& t, const Ref<IPropertyField>& field, bool user)
        {
            calls++;
            targets = t;
            byUser = user;
        }
    };

    struct CompleteRecorder
    {
        int                  calls = 0;
        Vector<IObject*>     targets;
        String               path;
        Vector<DataDocument> before;
        Vector<DataDocument> after;

        void OnCompleted(const Vector<IObject*>& t, const String& p,
                         const Vector<DataDocument>& b, const Vector<DataDocument>& a)
        {
            calls++;
            targets = t;
            path = p;
            before = b;
            after = a;
        }
    };

    Ref<CallbackStubViewer> MakeViewer()
    {
        return mmake<CallbackStubViewer>();
    }

    template<typename T>
    IObject* AsObject(const Ref<T>& ref)
    {
        return dynamic_cast<IObject*>(ref.Get());
    }
}

// ----------------------------------------------------------------------------
// onPropertyChanged subscription
// ----------------------------------------------------------------------------
TEST(PropertiesViewerCallbacks, PropertyChangedNotifiesSubscriberWithTargetsAndByUser)
{
    auto viewer = MakeViewer();
    auto actor = mmake<Actor>(ActorCreateMode::NotInScene);
    viewer->SetTargets({ AsObject(actor) });

    ChangeRecorder rec;
    viewer->onPropertyChanged += MakeFunction(&rec, &ChangeRecorder::OnChanged);

    viewer->OnPropertyChanged(Ref<IPropertyField>(), true);

    EXPECT_EQ(rec.calls, 1);
    ASSERT_EQ(rec.targets.Count(), 1);
    EXPECT_EQ(rec.targets[0], AsObject(actor));
    EXPECT_TRUE(rec.byUser);
}

TEST(PropertiesViewerCallbacks, PropertyChangedForwardsByUserFlag)
{
    auto viewer = MakeViewer();

    ChangeRecorder rec;
    viewer->onPropertyChanged += MakeFunction(&rec, &ChangeRecorder::OnChanged);

    viewer->OnPropertyChanged(Ref<IPropertyField>(), false);

    EXPECT_EQ(rec.calls, 1);
    EXPECT_FALSE(rec.byUser);
}

TEST(PropertiesViewerCallbacks, UnsubscribedPropertyChangedHandlerIsNotNotified)
{
    auto viewer = MakeViewer();

    ChangeRecorder rec;
    viewer->onPropertyChanged += MakeFunction(&rec, &ChangeRecorder::OnChanged);
    viewer->OnPropertyChanged(Ref<IPropertyField>(), true);
    ASSERT_EQ(rec.calls, 1);

    viewer->onPropertyChanged -= MakeFunction(&rec, &ChangeRecorder::OnChanged);
    viewer->OnPropertyChanged(Ref<IPropertyField>(), true);

    EXPECT_EQ(rec.calls, 1);
}

// ----------------------------------------------------------------------------
// onPropertyChangeCompleted subscription
// ----------------------------------------------------------------------------
TEST(PropertiesViewerCallbacks, PropertyChangeCompletedNotifiesSubscriberWithPayload)
{
    auto viewer = MakeViewer();
    auto actor = mmake<Actor>(ActorCreateMode::NotInScene);
    viewer->SetTargets({ AsObject(actor) });

    CompleteRecorder rec;
    viewer->onPropertyChangeCompleted += MakeFunction(&rec, &CompleteRecorder::OnCompleted);

    DataDocument before; before = String("old");
    DataDocument after;  after  = String("new");
    viewer->OnPropertyChangeCompleted("position", { before }, { after });

    EXPECT_EQ(rec.calls, 1);
    ASSERT_EQ(rec.targets.Count(), 1);
    EXPECT_EQ(rec.targets[0], AsObject(actor));
    EXPECT_EQ(rec.path, String("position"));
    EXPECT_EQ(rec.before.Count(), 1);
    EXPECT_EQ(rec.after.Count(), 1);
}

TEST(PropertiesViewerCallbacks, UnsubscribedPropertyChangeCompletedHandlerIsNotNotified)
{
    auto viewer = MakeViewer();

    CompleteRecorder rec;
    viewer->onPropertyChangeCompleted += MakeFunction(&rec, &CompleteRecorder::OnCompleted);
    viewer->OnPropertyChangeCompleted("p", {}, {});
    ASSERT_EQ(rec.calls, 1);

    viewer->onPropertyChangeCompleted -= MakeFunction(&rec, &CompleteRecorder::OnCompleted);
    viewer->OnPropertyChangeCompleted("p", {}, {});

    EXPECT_EQ(rec.calls, 1);
}

// ----------------------------------------------------------------------------
// Multicast: several subscribers
// ----------------------------------------------------------------------------
TEST(PropertiesViewerCallbacks, MultipleSubscribersAreAllNotified)
{
    auto viewer = MakeViewer();

    ChangeRecorder a, b;
    viewer->onPropertyChanged += MakeFunction(&a, &ChangeRecorder::OnChanged);
    viewer->onPropertyChanged += MakeFunction(&b, &ChangeRecorder::OnChanged);

    viewer->OnPropertyChanged(Ref<IPropertyField>(), true);

    EXPECT_EQ(a.calls, 1);
    EXPECT_EQ(b.calls, 1);
}

TEST(PropertiesViewerCallbacks, RemovingOneSubscriberKeepsTheOther)
{
    auto viewer = MakeViewer();

    ChangeRecorder a, b;
    viewer->onPropertyChanged += MakeFunction(&a, &ChangeRecorder::OnChanged);
    viewer->onPropertyChanged += MakeFunction(&b, &ChangeRecorder::OnChanged);

    viewer->onPropertyChanged -= MakeFunction(&a, &ChangeRecorder::OnChanged);
    viewer->OnPropertyChanged(Ref<IPropertyField>(), true);

    EXPECT_EQ(a.calls, 0);
    EXPECT_EQ(b.calls, 1);
}

TEST(PropertiesViewerCallbacks, FiringWithoutSubscribersIsNoOp)
{
    auto viewer = MakeViewer();

    viewer->OnPropertyChanged(Ref<IPropertyField>(), true);
    viewer->OnPropertyChangeCompleted("p", {}, {});

    SUCCEED();
}
