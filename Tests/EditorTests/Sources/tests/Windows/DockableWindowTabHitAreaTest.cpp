#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Scene/UI/Widget.h"
#include "o2/Scene/UI/WidgetLayer.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2Editor/UIRoot.h"
#include "o2Editor/Windows/DockWindowPlace.h"
#include "o2Editor/Windows/DockableWindow.h"
#include "support/EditorWindowsFixture.h"

using namespace o2;
using namespace Editor;
using namespace Editor::Tests;

// Tab click area must follow the tab layer when a neighbour tab changes width
class DockableWindowTabHitArea : public EditorWindowsFixture
{
protected:
    Ref<DockWindowPlace> place;
    Ref<DockableWindow> a;
    Ref<DockableWindow> b;

    void SetUp() override
    {
        EditorWindowsFixture::SetUp();

        auto root = MakeDock("main dock");
        EditorUIRoot.AddWidget(root);
        place = MakeDock("child");
        root->AddChild(place);

        a = MakeTabbed("a");
        b = MakeTabbed("b");
        a->PlaceDock(place);
        b->PlaceDock(place);
    }

    static Ref<DockableWindow> MakeTabbed(const String& name)
    {
        auto wnd = MakeDockable(name);
        wnd->SetAutoCalcuclatingTabWidth(false);
        auto tab = wnd->AddLayer("tab", nullptr);
        tab->AddChildLayer("main", nullptr, Layout::HorStretch(VerAlign::Top, 0, 0, 20));
        wnd->SetTabWidth(100);
        return wnd;
    }
};

TEST_F(DockableWindowTabHitArea, HitAreaMatchesTabLayerAfterArrange)
{
    place->ArrangeChildWindows();

    EXPECT_EQ(a->GetHeadDragAreaRect(), a->GetLayer("tab/main")->GetRect());
    EXPECT_EQ(b->GetHeadDragAreaRect(), b->GetLayer("tab/main")->GetRect());
    EXPECT_FLOAT_EQ(b->GetHeadDragAreaRect().left - a->GetHeadDragAreaRect().left, 100.0f);
}

TEST_F(DockableWindowTabHitArea, NeighbourTabWidening_ShiftsHitArea)
{
    place->ArrangeChildWindows();
    RectF before = b->GetHeadDragAreaRect();

    a->SetTabWidth(250);
    place->ArrangeChildWindows();

    EXPECT_FLOAT_EQ(a->GetHeadDragAreaRect().Width(), 250.0f);
    EXPECT_FLOAT_EQ(b->GetHeadDragAreaRect().left - before.left, 150.0f);
    EXPECT_EQ(b->GetHeadDragAreaRect(), b->GetLayer("tab/main")->GetRect());
}
