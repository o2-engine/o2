#include "o2Editor/stdafx.h"
#include <gtest/gtest.h>
#include <thread>
#include "o2/Render/Camera.h"
#include "o2/Render/Render.h"
#include "o2/Render/Text.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Scene/UI/Widgets/LongList.h"
#include "o2/Utils/Debug/Debug.h"
#include "o2/Utils/Editor/EditorScope.h"
#include "o2/Utils/Test/AppTestDriver.h"
#include "o2Editor/UIRoot.h"
#include "o2Editor/Windows/DockableWindow.h"
#include "o2Editor/Windows/LogWindow/LogWindow.h"

using namespace o2;
using namespace Editor;

// The log list follows new messages while it is scrolled to the end, stays put when scrolled
// up, and takes messages logged from other threads

namespace
{
    struct LogWindowUiFixture : ::testing::Test
    {
        Ref<LogWindow> window;
        Ref<LongList>  list;

        void SetUp() override
        {
            if (!UIRoot::IsSingletonInitialzed())
            {
                PushEditorScopeOnStack scope;
                mmake<UIRoot>();
            }

            PushEditorScopeOnStack scope;
            window = mmake<LogWindow>();
            auto wnd = window->GetWindow();
            *wnd->layout = WidgetLayout::Based(BaseCorner::Center, Vec2F(600, 300));
            EditorUIRoot.AddWidget(wnd);
            wnd->Show(true);
            list = wnd->FindChildByType<LongList>();
            Tick(3);
        }

        void TearDown() override
        {
            list = nullptr;
            window = nullptr;
            if (UIRoot::IsSingletonInitialzed())
                EditorUIRoot.RemoveAllWidgets();
        }

        void Tick(int frames = 1)
        {
            for (int i = 0; i < frames; i++)
            {
                PushEditorScopeOnStack scope;
                window->Update(1.0f / 60.0f);
                o2Render.Begin();
                o2Render.SetCamera(Camera());
                auto root = EditorUIRoot.GetRootWidget();
                root->Update(1.0f / 60.0f);
                root->UpdateChildren(1.0f / 60.0f);
                root->UpdateChildrenTransforms();
                root->Draw();
                o2Render.End();
                AppTestDriver::PumpFrames(1);
            }
        }

        // Captions of the list rows on screen, top to bottom
        Vector<String> Shown()
        {
            Vector<String> res;
            for (auto& item : list->GetChildWidgets())
                res.Add((String)item->GetLayerDrawable<Text>("caption")->GetText());
            return res;
        }
    };
}

TEST_F(LogWindowUiFixture, NewMessagesStayInViewAtTheEnd)
{
    for (int i = 0; i < 100; i++)
        o2Debug.Log("line " + (String)i);
    Tick(2);
    ASSERT_FALSE(Shown().IsEmpty());
    EXPECT_EQ(Shown().Last(), "line 99");

    for (int i = 100; i < 105; i++)
        o2Debug.Log("line " + (String)i);
    Tick(2);
    EXPECT_EQ(Shown().Last(), "line 104");
    EXPECT_NEAR(list->GetScroll().y, list->GetScrollRange().top, 1.0f);
}

TEST_F(LogWindowUiFixture, AListScrolledUpStaysWhereItIs)
{
    for (int i = 0; i < 100; i++)
        o2Debug.Log("line " + (String)i);
    Tick(2);

    list->SetScrollForcible(Vec2F(0, 0));
    Tick(2);
    String firstShown = Shown().First();

    for (int i = 100; i < 120; i++)
        o2Debug.Log("line " + (String)i);
    Tick(2);
    EXPECT_NEAR(list->GetScroll().y, 0.0f, 1.0f);
    EXPECT_EQ(Shown().First(), firstShown);
}

TEST_F(LogWindowUiFixture, MessagesFromAnotherThreadArrive)
{
    std::thread worker([]()
    {
        for (int i = 0; i < 200; i++)
            o2Debug.Log("worker " + (String)i);
    });
    worker.join();
    Tick(2);
    ASSERT_FALSE(Shown().IsEmpty());
    EXPECT_EQ(Shown().Last(), "worker 199");
}
