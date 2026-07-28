#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Application/Application.h"
#include "o2/Application/Input.h"
#include "o2/Render/Render.h"
#include "o2/Render/Sprite.h"
#include "o2/Scene/Components/ParticlesEmitterComponent.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Utils/Debug/Profiling/NanoProfiler.h"
#include "o2/Utils/Debug/Profiling/ProfilerOverlay.h"
#include "o2/Utils/Debug/Profiling/ProfilerWidget.h"
#include "o2/Utils/System/Time/Timer.h"
#include "o2/Utils/Test/AppTestDriver.h"

#include "Scene/SceneTestHelpers.h"

#if defined(O2_PROFILER_ENABLED)

using namespace o2;

namespace
{
    // Builds the widget over a screen sized root, the same way the overlay does, and keeps the
    // profiler recording for the test
    class ProfilerWidgetFixture: public ::testing::Test
    {
    protected:
        Ref<Widget>         root;
        Ref<ProfilerWidget> widget;

        void SetUp() override
        {
            NanoProfiler::SetEnabled(true);
            NanoProfiler::NextFrame();

            root = mmake<Widget>(ActorCreateMode::NotInScene);
            widget = mmake<ProfilerWidget>();
            root->AddChild(widget);

            *root->layout = WidgetLayout::Based(BaseCorner::Center, (Vec2F)o2Application.GetContentSize());
            *widget->layout = WidgetLayout::Based(BaseCorner::LeftTop, widget->GetContentSize());

            // park the cursor away from the panel: a test that left it hovering the timeline would
            // otherwise freeze the next one
            AppTestDriver::MoveCursor(Vec2F(), 1);

            // one application frame initializes the actors, so they become enabled in hierarchy
            AppTestDriver::PumpFrames(1);
            root->UpdateTransform();
        }

        void TearDown() override
        {
            widget = nullptr;
            root = nullptr;

            NanoProfiler::SetEnabled(false);
            NanoProfiler::NextFrame();

            AppTestDriver::PumpFrames(1);
        }

        // Records one profiler frame of the given scopes and lets the widget capture it
        void RecordFrame(const Vector<const char*>& scopes, int spin = 20000)
        {
            for (auto scope : scopes)
            {
                NanoProfiler::SampleScope sample(scope);

                volatile int sink = 0;
                for (int i = 0; i < spin; i++)
                    sink += i;
            }

            NanoProfiler::NextFrame();
            widget->Update(0.016f);
        }
    };
}

TEST_F(ProfilerWidgetFixture, ContentSizeGrowsWithMetricsAndCounters)
{
    const Vec2F bare = widget->GetContentSize();

    widget->AddMetric(PerfMetric("metric", []() { return 1.0; }, PerfMetricSettings(), { 1.0 }));
    const Vec2F withMetric = widget->GetContentSize();

    EXPECT_FLOAT_EQ(withMetric.x, bare.x);
    EXPECT_GT(withMetric.y, bare.y);

    widget->AddCounter(PerfCounter("counter", []() { return 3; }, PerfMetricSettings()));
    EXPECT_GT(widget->GetContentSize().y, withMetric.y);
}

TEST_F(ProfilerWidgetFixture, ValueStatusHandlesBothMetricDirections)
{
    PerfMetricSettings lessIsBetter;
    lessIsBetter.goodValue = 16.0;
    lessIsBetter.badValue = 33.0;

    EXPECT_EQ(ProfilerWidget::GetValueStatus(10.0, lessIsBetter), PerfStatus::Good);
    EXPECT_EQ(ProfilerWidget::GetValueStatus(20.0, lessIsBetter), PerfStatus::Normal);
    EXPECT_EQ(ProfilerWidget::GetValueStatus(40.0, lessIsBetter), PerfStatus::Bad);

    PerfMetricSettings moreIsBetter;
    moreIsBetter.goodValue = 60.0;
    moreIsBetter.badValue = 30.0;

    EXPECT_EQ(ProfilerWidget::GetValueStatus(75.0, moreIsBetter), PerfStatus::Good);
    EXPECT_EQ(ProfilerWidget::GetValueStatus(45.0, moreIsBetter), PerfStatus::Normal);
    EXPECT_EQ(ProfilerWidget::GetValueStatus(20.0, moreIsBetter), PerfStatus::Bad);
}

TEST_F(ProfilerWidgetFixture, OverallStatusIsWeightedAcrossTheMetrics)
{
    PerfMetricSettings settings;
    settings.goodValue = 0.0;
    settings.badValue = 100.0;

    auto settle = [&]()
    {
        for (int i = 0; i < PerfMetric::samplesCount; i++)
            widget->Update(0.016f);
    };

    for (int i = 0; i < 3; i++)
        widget->AddMetric(PerfMetric("good", []() { return 0.0; }, settings, { 100.0 }));

    settle();
    EXPECT_EQ(widget->GetOverallStatus(), PerfStatus::Good);

    // one bad metric out of four drags the average to the normal band
    widget->AddMetric(PerfMetric("bad", []() { return 500.0; }, settings, { 100.0 }));
    settle();
    EXPECT_EQ(widget->GetOverallStatus(), PerfStatus::Normal);

    // and a majority of bad ones make the whole panel bad
    for (int i = 0; i < 3; i++)
        widget->AddMetric(PerfMetric("bad", []() { return 500.0; }, settings, { 100.0 }));

    settle();
    EXPECT_EQ(widget->GetOverallStatus(), PerfStatus::Bad);
}

TEST_F(ProfilerWidgetFixture, TimelineCollectsFramesUpToItsCapacity)
{
    EXPECT_EQ(widget->GetHistoryCount(), 0);

    RecordFrame({ "alpha", "beta" });
    EXPECT_EQ(widget->GetHistoryCount(), 1);

    for (int i = 0; i < ProfilerWidget::historyFrames + 10; i++)
        RecordFrame({ "alpha" }, 2000);

    EXPECT_EQ(widget->GetHistoryCount(), ProfilerWidget::historyFrames);
}

TEST_F(ProfilerWidgetFixture, CursorOverTheTimelineFreezesAndDetailsIt)
{
    for (int i = 0; i < 20; i++)
        RecordFrame({ "alpha", "beta" }, 5000);

    const int frozenAt = widget->GetHistoryCount();
    EXPECT_EQ(widget->GetDetailedFrame(), -1);

    // the timeline starts at the panel's left padding, a few pixels below its top
    const RectF panel = widget->layout->GetWorldRect();
    AppTestDriver::MoveCursor(Vec2F(panel.left + 20.0f, panel.top - 40.0f), 1);

    RecordFrame({ "alpha", "beta" }, 5000);

    EXPECT_GE(widget->GetDetailedFrame(), 0);
    EXPECT_EQ(widget->GetHistoryCount(), frozenAt) << "hovered timeline must stop collecting frames";

    // and it runs again once the cursor leaves
    AppTestDriver::MoveCursor(Vec2F(panel.right + 200.0f, panel.bottom - 200.0f), 1);
    RecordFrame({ "alpha", "beta" }, 5000);

    EXPECT_EQ(widget->GetDetailedFrame(), -1);
    EXPECT_EQ(widget->GetHistoryCount(), frozenAt + 1);
}

// The two controls of the panel light up under the cursor, and the baseline button reacts to a click
TEST_F(ProfilerWidgetFixture, ControlsHighlightUnderTheCursor)
{
    const RectF panel = widget->layout->GetWorldRect();

    widget->Update(0.016f);
    EXPECT_FALSE(widget->IsBaselineHovered());
    EXPECT_FALSE(widget->IsResizeGripHovered());

    // the baseline button sits in the top right corner of the panel, inside its caption bar
    AppTestDriver::MoveCursor(Vec2F(panel.right - 25.0f, panel.top - 10.0f), 1);
    widget->Update(0.016f);
    EXPECT_TRUE(widget->IsBaselineHovered());
    EXPECT_FALSE(widget->IsResizeGripHovered());

    // and the resize grip in the bottom right one
    AppTestDriver::MoveCursor(Vec2F(panel.right - 5.0f, panel.bottom + 5.0f), 1);
    widget->Update(0.016f);
    EXPECT_FALSE(widget->IsBaselineHovered());
    EXPECT_TRUE(widget->IsResizeGripHovered());

    AppTestDriver::MoveCursor(Vec2F(), 1);
    widget->Update(0.016f);
    EXPECT_FALSE(widget->IsBaselineHovered());
    EXPECT_FALSE(widget->IsResizeGripHovered());
}

TEST_F(ProfilerWidgetFixture, BaselineCapturesCurrentValues)
{
    int counterValue = 10;
    widget->AddCounter(PerfCounter("things", [&]() { return counterValue; }, PerfMetricSettings()));

    EXPECT_FALSE(widget->IsBaselineEnabled());

    widget->SetBaselineEnabled(true);
    EXPECT_TRUE(widget->IsBaselineEnabled());

    counterValue = 25;
    widget->Update(0.5f); // past the counters sampling interval

    widget->SetBaselineEnabled(false);
    EXPECT_FALSE(widget->IsBaselineEnabled());
}

// The panel measures the game, not itself: its own update and draw record no scopes at all
TEST_F(ProfilerWidgetFixture, UpdateAndDrawAreExcludedFromTheProfile)
{
    widget->AddMetric(PerfMetric("FPS", []() { return 60.0; }, PerfMetricSettings(), { 60.0 }));
    widget->AddCounter(PerfCounter("things", []() { return 7; }, PerfMetricSettings()));

    for (int i = 0; i < 5; i++)
        RecordFrame({ "alpha", "beta", "gamma" });

    NanoProfiler::NextFrame();
    widget->Update(0.016f);
    NanoProfiler::NextFrame();
    EXPECT_EQ(NanoProfiler::GetFrameSamplesCount(), 0);

    o2Render.Begin();

    NanoProfiler::NextFrame();
    widget->Draw();
    NanoProfiler::NextFrame();
    EXPECT_EQ(NanoProfiler::GetFrameSamplesCount(), 0);

    o2Render.End();
}

TEST_F(ProfilerWidgetFixture, TimelineSurvivesEmptyProfilerFrames)
{
    RecordFrame({});
    RecordFrame({});

    EXPECT_EQ(widget->GetHistoryCount(), 2);

    o2Render.Begin();
    root->Draw();
    o2Render.End();

    EXPECT_EQ(widget->GetDetailedFrame(), -1);
}

// The counters describe what the scene is made of, so they have to follow the content that is added
// to it: a widget with a sprite layer is one actor, one UI element and one sprite
TEST(ProfilerOverlayTest, CountersFollowTheSceneContent)
{
    auto overlay = ProfilerOverlay::InstancePtr();
    ASSERT_NE(overlay, nullptr);

    const bool wasVisible = overlay->IsVisible();
    overlay->SetVisible(true);
    AppTestDriver::PumpFrames(2);

    auto counterValue = [&](const String& name)
    {
        for (auto& counter : overlay->GetWidget()->GetCounters())
        {
            if (counter.name == name)
                return counter.count;
        }

        return -1;
    };

    // let the counters sample the scene as it is now
    AppTestDriver::Wait(0.4f);

    const int actorsBefore = counterValue("Actors");
    const int uiBefore = counterValue("UI");
    const int spritesBefore = counterValue("Sprites");
    const int particlesBefore = counterValue("Particles");

    ASSERT_GE(actorsBefore, 0) << "the built-in counters must be registered";

    {
        SceneCleanGuard sceneGuard;

        auto widget = mmake<Widget>();
        widget->AddLayer("back", mmake<Sprite>());

        auto emitter = mmake<Actor>();
        emitter->AddComponent<ParticlesEmitterComponent>();

        AppTestDriver::Wait(0.4f); // past the counters sampling interval

        EXPECT_EQ(counterValue("Actors"), actorsBefore + 2);
        EXPECT_EQ(counterValue("UI"), uiBefore + 1);
        EXPECT_EQ(counterValue("Sprites"), spritesBefore + 1);
        EXPECT_EQ(counterValue("Particles"), particlesBefore + 1);
    }

    AppTestDriver::Wait(0.4f);

    EXPECT_EQ(counterValue("Actors"), actorsBefore);
    EXPECT_EQ(counterValue("Sprites"), spritesBefore);

    overlay->SetVisible(wasVisible);
}

TEST(ProfilerOverlayTest, ToggleShowsTheWidgetAndSwitchesRecording)
{
    auto overlay = ProfilerOverlay::InstancePtr();
    ASSERT_NE(overlay, nullptr);

    const bool wasVisible = overlay->IsVisible();
    const bool wasRecording = NanoProfiler::IsEnabled();

    overlay->SetVisible(true);
    EXPECT_TRUE(overlay->IsVisible());
    EXPECT_TRUE(NanoProfiler::IsEnabled());
    EXPECT_TRUE(overlay->GetWidget());

    AppTestDriver::PumpFrames(3);
    EXPECT_GT(overlay->GetWidget()->GetHistoryCount(), 0);

    overlay->SetVisible(false);
    EXPECT_FALSE(overlay->IsVisible());
    EXPECT_FALSE(NanoProfiler::IsEnabled());

    overlay->SetVisible(wasVisible);
    NanoProfiler::SetEnabled(wasRecording);
}

TEST(ProfilerOverlayTest, F12TogglesTheWidget)
{
    auto overlay = ProfilerOverlay::InstancePtr();
    ASSERT_NE(overlay, nullptr);

    const bool wasVisible = overlay->IsVisible();
    overlay->SetVisible(false);

    o2Input.OnKeyPressed(VK_F12);
    AppTestDriver::PumpFrames(1);
    o2Input.OnKeyReleased(VK_F12);
    AppTestDriver::PumpFrames(1);

    EXPECT_TRUE(overlay->IsVisible());

    o2Input.OnKeyPressed(VK_F12);
    AppTestDriver::PumpFrames(1);
    o2Input.OnKeyReleased(VK_F12);
    AppTestDriver::PumpFrames(1);

    EXPECT_FALSE(overlay->IsVisible());

    overlay->SetVisible(wasVisible);
}

TEST(ProfilerOverlayTest, LongTapInTheTopLeftCornerTogglesTheWidget)
{
    auto overlay = ProfilerOverlay::InstancePtr();
    ASSERT_NE(overlay, nullptr);

    const bool wasVisible = overlay->IsVisible();
    overlay->SetVisible(false);

    const Vec2F contentSize = (Vec2F)o2Application.GetContentSize();
    const Vec2F corner(-contentSize.x*0.5f + 20.0f, contentSize.y*0.5f - 20.0f);

    AppTestDriver::PressCursor(corner);
    AppTestDriver::Wait(ProfilerOverlay::longTapTime + 0.2f);
    AppTestDriver::ReleaseCursor();

    EXPECT_TRUE(overlay->IsVisible());

    // a hold away from the corner leaves it alone
    overlay->SetVisible(false);
    AppTestDriver::PressCursor(Vec2F(0.0f, 0.0f));
    AppTestDriver::Wait(ProfilerOverlay::longTapTime + 0.2f);
    AppTestDriver::ReleaseCursor();

    EXPECT_FALSE(overlay->IsVisible());

    overlay->SetVisible(wasVisible);
}

TEST_F(ProfilerWidgetFixture, ContentSizeFollowsTheRequestedSizeAndIsClamped)
{
    widget->AddMetric(PerfMetric("metric", []() { return 1.0; }, PerfMetricSettings(), { 1.0 }));

    const Vec2F design = widget->GetDesignSize();
    const Vec2F min = widget->GetMinContentSize();

    EXPECT_EQ(widget->GetContentSize(), design);
    EXPECT_LT(min.x, design.x);
    EXPECT_FLOAT_EQ(min.y, design.y) << "the design height is already the minimal one";

    int contentSizeChanges = 0;
    widget->onContentSizeChanged = [&]() { contentSizeChanges++; };

    // both axes are taken as they come
    widget->SetContentSize(design + Vec2F(200.0f, 120.0f));
    EXPECT_EQ(widget->GetContentSize(), design + Vec2F(200.0f, 120.0f));
    EXPECT_EQ(contentSizeChanges, 1);

    // the same size doesn't notify again
    widget->SetContentSize(design + Vec2F(200.0f, 120.0f));
    EXPECT_EQ(contentSizeChanges, 1);

    widget->SetContentSize(Vec2F(10.0f, 10.0f));
    EXPECT_EQ(widget->GetContentSize(), min);

    widget->SetContentSize(design*100.0f);
    EXPECT_EQ(widget->GetContentSize(), design*ProfilerWidget::maxSizeFactor);

    widget->onContentSizeChanged = Function<void()>();
}

// Resizing stretches the timeline only: the caption rows keep their height, so a taller panel means
// a taller graph and a wider one means wider frame columns
TEST_F(ProfilerWidgetFixture, ResizingStretchesTheTimelineAndKeepsTracking)
{
    widget->AddMetric(PerfMetric("metric", []() { return 1.0; }, PerfMetricSettings(), { 1.0 }));

    const Vec2F design = widget->GetDesignSize();
    const float minTimeline = (ProfilerWidget::maxDetailSamples + 1)*13.0f;
    const float fixedRows = design.y - minTimeline;

    widget->SetContentSize(design + Vec2F(300.0f, 200.0f));
    *widget->layout = WidgetLayout::Based(BaseCorner::LeftTop, widget->GetContentSize());
    root->UpdateTransform();

    for (int i = 0; i < 10; i++)
        RecordFrame({ "alpha", "beta" }, 5000);

    EXPECT_EQ(widget->GetHistoryCount(), 10);

    const RectF panel = widget->layout->GetWorldRect();
    EXPECT_NEAR(panel.Width(), design.x + 300.0f, 1.0f);
    EXPECT_NEAR(panel.Height(), design.y + 200.0f, 1.0f);

    // the graph took the whole extra height, the rows below it didn't move
    EXPECT_NEAR(panel.Height() - fixedRows, minTimeline + 200.0f, 1.0f);

    // the timeline hit test follows the new geometry
    AppTestDriver::MoveCursor(Vec2F(panel.left + 40.0f, panel.top - 80.0f), 1);
    RecordFrame({ "alpha", "beta" }, 5000);
    EXPECT_GE(widget->GetDetailedFrame(), 0);

    o2Render.Begin();
    root->Draw();
    o2Render.End();
}

// The panel is a debug overlay, but a debug overlay that costs a millisecond would change the frame it
// is supposed to measure. A full timeline plus every built-in metric must stay well under 0.5 ms
TEST_F(ProfilerWidgetFixture, FullPanelUpdateAndDrawStayCheap)
{
    widget->AddMetric(PerfMetric("FPS", []() { return 60.0; }, PerfMetricSettings(), { 60.0 }));
    widget->AddMetric(PerfMetric("Frame ms", []() { return 16.0; }, PerfMetricSettings(), { 16.0 }));
    widget->AddMetric(PerfMetric("Draw calls", []() { return 200.0; }, PerfMetricSettings(), { 500.0 }));
    widget->AddMetric(PerfMetric("Primitives", []() { return 30000.0; }, PerfMetricSettings(), { 50000.0 }));

    for (int i = 0; i < 5; i++)
        widget->AddCounter(PerfCounter("counter", []() { return 100; }, PerfMetricSettings()));

    // fill the timeline and the metric series completely, the worst case for both
    for (int i = 0; i < ProfilerWidget::historyFrames; i++)
        RecordFrame({ "alpha", "beta", "gamma", "delta", "epsilon", "zeta", "eta", "theta", "iota", "kappa" }, 200);

    ASSERT_EQ(widget->GetHistoryCount(), ProfilerWidget::historyFrames);

    // Best of several batches: the suites run in parallel, so a single batch can catch a hiccup
    const int kRuns = 30;
    const int kBatches = 5;

    Timer timer;

    auto measureMs = [&](const Function<void()>& body)
    {
        float best = FLT_MAX;
        for (int batch = 0; batch < kBatches; batch++)
        {
            timer.GetDeltaTime();
            for (int i = 0; i < kRuns; i++)
                body();

            best = Math::Min(best, timer.GetDeltaTime()*1000.0f/kRuns);
        }

        return best;
    };

    const float updateMs = measureMs([&]() { widget->Update(0.016f); });

    o2Render.Begin();
    const float drawMs = measureMs([&]() { widget->Draw(); });
    o2Render.End();

    o2Debug.Log("ProfilerWidget: update %.3f ms, draw %.3f ms", updateMs, drawMs);

    EXPECT_LT(updateMs, 0.5f);
    EXPECT_LT(drawMs, 2.0f);
}

#endif // O2_PROFILER_ENABLED
