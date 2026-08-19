#include "o2Editor/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Render/Render.h"
#include "o2/Scene/UI/Widget.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Utils/Debug/Profiling/NanoProfiler.h"
#include "o2/Utils/Debug/Profiling/ProfilerOverlay.h"
#include "o2/Utils/Debug/Profiling/ProfilerWidget.h"
#include "o2Editor/Windows/GameWindow/GameWindow.h"
#include "o2/Utils/Editor/EditorScope.h"
#include "support/EditorTestScene.h"

#if defined(O2_PROFILER_ENABLED)

using namespace o2;
using namespace Editor;
using namespace Editor::Tests;

namespace
{
    // Replays the editor's frame shape: EditorApplication::ProcessFrame keeps the whole frame in
    // editor scope and the game parts — the scene update and the Game window rendering — leave it
    class EditorFrameGuard
    {
    public:
        EditorFrameGuard()
        {
            NanoProfiler::BindThread();
            NanoProfiler::SetEnabled(true);
            NanoProfiler::NextFrame();
        }

        ~EditorFrameGuard()
        {
            NanoProfiler::SetEnabled(false);
            NanoProfiler::NextFrame();
        }
    };

    bool FrameHasSample(const char* name)
    {
        const NanoProfiler::Sample* samples = NanoProfiler::GetFrameSamples();
        for (int i = 0; i < NanoProfiler::GetFrameSamplesCount(); i++)
        {
            if (samples[i].name == name)
                return true;
        }

        return false;
    }
}

// In the editor the panel must show the game only: everything the editor itself does happens inside
// an editor scope and is dropped, while the scene update and the Game window drawing are recorded
TEST(ProfilerEditorScope, OnlyGameWorkIsRecordedInsideAnEditorFrame)
{
    EditorFrameGuard frame;

    {
        PushEditorScopeOnStack editorFrame;

        {
            NanoProfiler::SampleScope editorUi("editor windows draw");
        }

        // EditorApplication::UpdateScene runs the game scene outside the editor scope
        {
            ForcePopEditorScopeOnStack gameScope;
            NanoProfiler::SampleScope sceneUpdate("game scene update");
        }

        {
            NanoProfiler::SampleScope editorTree("editor tree update");
        }

        // GameWindow::GameView::Draw leaves the editor scope around the scene rendering
        {
            ForcePopEditorScopeOnStack gameScope;
            NanoProfiler::SampleScope gameDraw("game window draw");
        }
    }

    NanoProfiler::NextFrame();

    EXPECT_EQ(NanoProfiler::GetFrameSamplesCount(), 2);
    EXPECT_TRUE(FrameHasSample("game scene update"));
    EXPECT_TRUE(FrameHasSample("game window draw"));
    EXPECT_FALSE(FrameHasSample("editor windows draw"));
    EXPECT_FALSE(FrameHasSample("editor tree update"));
}

// The panel itself is an editor overlay: drawing it inside the editor scope records nothing, so the
// timeline never charges the game for the profiler's own cost
TEST(ProfilerEditorScope, PanelDrawnInsideTheEditorScopeAddsNothingToTheFrame)
{
    SceneCleanGuard guard;
    EditorFrameGuard frame;

    auto root = mmake<Widget>(ActorCreateMode::NotInScene);
    auto widget = mmake<ProfilerWidget>();
    root->AddChild(widget);

    *root->layout = WidgetLayout::Based(BaseCorner::Center, Vec2F(1280.0f, 800.0f));
    *widget->layout = WidgetLayout::Based(BaseCorner::LeftTop, widget->GetContentSize());

    TickScene();
    root->UpdateTransform();

    {
        ForcePopEditorScopeOnStack gameScope;
        NanoProfiler::SampleScope gameWork("game work");
    }

    NanoProfiler::NextFrame();
    ASSERT_EQ(NanoProfiler::GetFrameSamplesCount(), 1);

    int drawnPrimitives = 0;
    {
        PushEditorScopeOnStack editorFrame;

        widget->Update(0.016f);

        o2Render.Begin();
        root->Draw();
        drawnPrimitives = o2Render.GetDrawnPrimitives();
        o2Render.End();
    }

    NanoProfiler::NextFrame();
    EXPECT_EQ(NanoProfiler::GetFrameSamplesCount(), 0);

    // the panel is drawn even though the whole frame is in an editor scope
    EXPECT_GT(drawnPrimitives, 0);

    // and the frame it captured is the game one
    EXPECT_EQ(widget->GetHistoryCount(), 1);
}

// In the editor the panel belongs to the Game window: it is drawn by the view that renders the game,
// anchored to its top left corner, so it never covers the rest of the editor
TEST(ProfilerInGameWindow, PanelIsPlacedInsideTheGameView)
{
    SceneCleanGuard guard;

    auto overlay = ProfilerOverlay::InstancePtr();
    ASSERT_NE(overlay, nullptr);

    const bool wasVisible = overlay->IsVisible();
    overlay->SetVisible(true);

    auto view = mmake<GameWindow::GameView>();
    *view->layout = WidgetLayout::Based(BaseCorner::Center, Vec2F(1000.0f, 700.0f), Vec2F(120.0f, -60.0f));
    view->UpdateTransform();
    TickScene();

    o2Render.Begin();
    {
        PushEditorScopeOnStack editorFrame;
        view->Draw();
    }
    o2Render.End();

    const RectF viewRect = view->layout->GetWorldRect();
    const RectF panel = overlay->GetWidget()->layout->GetWorldRect();

    EXPECT_NEAR(panel.left, viewRect.left, 1.0f);
    EXPECT_NEAR(panel.top, viewRect.top, 1.0f);
    EXPECT_LE(panel.right, viewRect.right);
    EXPECT_GE(panel.bottom, viewRect.bottom);

    overlay->SetVisible(wasVisible);
}

// A Game window smaller than the panel doesn't squeeze it below the size it still reads at
TEST(ProfilerInGameWindow, PanelKeepsItsMinimalSizeInATinyGameView)
{
    SceneCleanGuard guard;

    auto overlay = ProfilerOverlay::InstancePtr();
    ASSERT_NE(overlay, nullptr);

    const bool wasVisible = overlay->IsVisible();
    overlay->SetVisible(true);

    auto view = mmake<GameWindow::GameView>();
    *view->layout = WidgetLayout::Based(BaseCorner::Center, Vec2F(200.0f, 150.0f));
    view->UpdateTransform();
    TickScene();

    o2Render.Begin();
    {
        PushEditorScopeOnStack editorFrame;
        view->Draw();
    }
    o2Render.End();

    auto widget = overlay->GetWidget();
    const RectF panel = widget->layout->GetWorldRect();
    const Vec2F min = widget->GetMinContentSize();

    EXPECT_NEAR(panel.Width(), min.x, 1.0f);
    EXPECT_NEAR(panel.Height(), min.y, 1.0f);

    overlay->SetVisible(wasVisible);
}

#endif // O2_PROFILER_ENABLED
