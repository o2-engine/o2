// @CODETOOLIGNORE: the profiler classes are not reflected, and the whole feature is compiled
// out by O2_PROFILER, which generated registrations would not follow
#include "o2/stdafx.h"
#include "ProfilerOverlay.h"

#if defined(O2_PROFILER_ENABLED)

#include "o2/Application/Application.h"
#include "o2/Application/Input.h"
#include "o2/Application/VKCodes.h"
#include "o2/Render/Camera.h"
#include "o2/Render/Render.h"
#include "o2/Render/Sprite.h"
#include "o2/Render/Text.h"
#include "o2/Scene/Components/AnimationComponent.h"
#include "o2/Scene/Components/AnimationStateGraphComponent.h"
#include "o2/Scene/Components/ImageComponent.h"
#include "o2/Scene/Components/LightComponent.h"
#include "o2/Scene/Components/Mesh3DComponent.h"
#include "o2/Scene/Components/MeshComponent.h"
#include "o2/Scene/Components/MeshPrimitiveComponent.h"
#include "o2/Scene/Components/ParticlesEmitterComponent.h"
#include "o2/Scene/Components/SkinnedMeshComponent.h"
#include "o2/Scene/Components/SkinningMeshComponent.h"
#include "o2/Scene/Components/SpineComponent.h"
#include "o2/Scene/Scene.h"
#include "o2/Scene/UI/Widget.h"
#include "o2/Scene/UI/WidgetLayer.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Utils/Debug/Profiling/NanoProfiler.h"
#include "o2/Utils/Debug/Profiling/ProfilerWidget.h"
#include "o2/Utils/System/Time/Time.h"

namespace o2
{
    namespace
    {
        ProfilerOverlay* gProfilerOverlay = nullptr;
    }

    ProfilerOverlay::ProfilerOverlay()
    {
        gProfilerOverlay = this;
    }

    ProfilerOverlay::~ProfilerOverlay()
    {
        if (gProfilerOverlay == this)
            gProfilerOverlay = nullptr;
    }

    ProfilerOverlay* ProfilerOverlay::InstancePtr()
    {
        return gProfilerOverlay;
    }

    void ProfilerOverlay::Initialize()
    {
        if (mWidget)
            return;

        NanoProfiler::ExcludeScope exclude;

        mRoot = mmake<Widget>(ActorCreateMode::NotInScene);
        mRoot->name = "profiler overlay root";

        mWidget = mmake<ProfilerWidget>();
        mWidget->onContentSizeChanged = [&]() { OnApplicationSized(); };
        mWidget->onCountersUpdate = [&]() { CountSceneContent(); };
        mRoot->AddChild(mWidget);

        PerfMetricSettings fpsSettings;
        fpsSettings.goodValue = 55.0;
        fpsSettings.badValue = 30.0;
        mWidget->AddMetric(PerfMetric("FPS", []() { return (double)o2Time.GetFPS(); }, fpsSettings,
                                      { 30.0, 60.0, 90.0, 120.0, 240.0 }));

        PerfMetricSettings frameSettings;
        frameSettings.goodValue = 16.7;
        frameSettings.badValue = 33.4;
        mWidget->AddMetric(PerfMetric("Frame ms",
                                      []() { return (double)NanoProfiler::GetFrameDuration()*1e-6; },
                                      frameSettings, { 16.7, 33.4, 50.0, 100.0 }));

        PerfMetricSettings drawCallsSettings;
        drawCallsSettings.goodValue = 150.0;
        drawCallsSettings.badValue = 600.0;
        mWidget->AddMetric(PerfMetric("Draw calls", []() { return (double)o2Render.GetSceneDrawCallsCount(); },
                                      drawCallsSettings, { 100.0, 250.0, 500.0, 1000.0 }));

        PerfMetricSettings primitivesSettings;
        primitivesSettings.goodValue = 20000.0;
        primitivesSettings.badValue = 200000.0;
        mWidget->AddMetric(PerfMetric("Primitives", []() { return (double)o2Render.GetSceneDrawnPrimitives(); },
                                      primitivesSettings, { 10000.0, 50000.0, 100000.0, 250000.0 }));

        PerfMetricSettings countersSettings;
        countersSettings.goodValue = 0.0;
        countersSettings.badValue = 100000.0;
        countersSettings.goodWeight = 0.0f;
        countersSettings.normalWeight = 0.0f;
        countersSettings.badWeight = 0.0f;

        // What the scene is made of, all read off a single pass done by CountSceneContent. Counted before
        // the counters are added, so the panel shows real numbers on its very first frame
        CountSceneContent();

        mWidget->AddCounter(PerfCounter("Actors", [&]() { return mSceneContent.actors; }, countersSettings));
        mWidget->AddCounter(PerfCounter("UI", [&]() { return mSceneContent.ui; }, countersSettings));
        mWidget->AddCounter(PerfCounter("Sprites", [&]() { return mSceneContent.sprites; }, countersSettings));
        mWidget->AddCounter(PerfCounter("Texts", [&]() { return mSceneContent.texts; }, countersSettings));
        mWidget->AddCounter(PerfCounter("Animations", [&]() { return mSceneContent.animations; },
                                        countersSettings));
        mWidget->AddCounter(PerfCounter("Particles", [&]() { return mSceneContent.particles; },
                                        countersSettings));
        mWidget->AddCounter(PerfCounter("Models", [&]() { return mSceneContent.models; }, countersSettings));
        mWidget->AddCounter(PerfCounter("Spines", [&]() { return mSceneContent.spines; }, countersSettings));
        mWidget->AddCounter(PerfCounter("Lights", [&]() { return mSceneContent.lights; }, countersSettings));

        OnApplicationSized();
    }

    void ProfilerOverlay::CountSceneContent()
    {
        NanoProfiler::ExcludeScope exclude;

        mSceneContent = SceneContent();

        for (auto& kv : o2Scene.GetAllActors())
        {
            auto actor = kv.second.Lock();

            // The map holds every actor ever created, the editor's own UI included; only the scene counts
            if (!actor || !actor->IsOnScene())
                continue;

            mSceneContent.actors++;

            if (auto widget = DynamicCast<Widget>(actor))
            {
                mSceneContent.ui++;
                CountWidgetLayers(widget->GetLayers());
            }

            for (auto& component : actor->GetComponents())
                CountComponent(component.Get());
        }

    }

    void ProfilerOverlay::CountComponent(Component* component)
    {
        // Spine is an animation component, so it has to be asked about first
        if (dynamic_cast<SpineComponent*>(component))
            mSceneContent.spines++;
        else if (dynamic_cast<AnimationComponent*>(component) ||
                 dynamic_cast<AnimationStateGraphComponent*>(component))
            mSceneContent.animations++;
        else if (dynamic_cast<ImageComponent*>(component))
            mSceneContent.sprites++;
        else if (dynamic_cast<ParticlesEmitterComponent*>(component))
            mSceneContent.particles++;
        else if (dynamic_cast<LightComponent*>(component))
            mSceneContent.lights++;
        else if (dynamic_cast<Mesh3DComponent*>(component) || dynamic_cast<MeshComponent*>(component) ||
                 dynamic_cast<MeshPrimitiveComponent*>(component) ||
                 dynamic_cast<SkinnedMeshComponent*>(component) ||
                 dynamic_cast<SkinningMeshComponent*>(component))
            mSceneContent.models++;
    }

    void ProfilerOverlay::CountWidgetLayers(const Vector<Ref<WidgetLayer>>& layers)
    {
        for (auto& layer : layers)
        {
            if (auto drawable = layer->GetDrawable().Get())
            {
                if (dynamic_cast<Text*>(drawable))
                    mSceneContent.texts++;
                else if (dynamic_cast<Sprite*>(drawable))
                    mSceneContent.sprites++;
            }

            CountWidgetLayers(layer->GetChildren());
        }
    }

    void ProfilerOverlay::SetVisible(bool visible)
    {
        if (mVisible == visible)
            return;

        mVisible = visible;

        if (mVisible)
            Initialize();

        NanoProfiler::SetEnabled(mVisible);
    }

    const Ref<ProfilerWidget>& ProfilerOverlay::GetWidget()
    {
        Initialize();
        return mWidget;
    }

    void ProfilerOverlay::AddMetric(const PerfMetric& metric)
    {
        GetWidget()->AddMetric(metric);
        OnApplicationSized();
    }

    void ProfilerOverlay::AddCounter(const PerfCounter& counter)
    {
        GetWidget()->AddCounter(counter);
        OnApplicationSized();
    }

    void ProfilerOverlay::LayoutIn(const RectF& area)
    {
        if (!mRoot)
            return;

        NanoProfiler::ExcludeScope exclude;

        // The panel never spills out of its host: in the editor the Game window can be smaller than the
        // panel's natural size
        const Vec2F minSize = mWidget->GetMinContentSize();
        const Vec2F wanted = mWidget->GetContentSize();
        const Vec2F size(Math::Max(Math::Min(wanted.x, area.Width()), minSize.x),
                         Math::Max(Math::Min(wanted.y, area.Height()), minSize.y));

        // Hung on the top left corner of its host, flush with it
        *mRoot->layout = WidgetLayout::Based(BaseCorner::Center, area.Size(), area.Center());
        *mWidget->layout = WidgetLayout::Based(BaseCorner::LeftTop, size);

        mRoot->UpdateTransform();
    }

    void ProfilerOverlay::OnApplicationSized()
    {
        // The default camera puts the screen around the origin
        const Vec2F half = (Vec2F)o2Application.GetContentSize()*0.5f;
        LayoutIn(RectF(-half.x, -half.y, half.x, half.y));
    }

    bool ProfilerOverlay::CheckToggleInput(float dt)
    {
        if (o2Input.IsKeyPressed(VK_F12))
            return true;

        const Vec2F contentSize = (Vec2F)o2Application.GetContentSize();
        const RectF corner(-contentSize.x*0.5f, contentSize.y*0.5f - longTapCornerSize,
                           -contentSize.x*0.5f + longTapCornerSize, contentSize.y*0.5f);

        if (o2Input.IsCursorPressed())
        {
            mLongTapOrigin = o2Input.GetCursorPos();
            mLongTapTime = 0.0f;
            mLongTapTracking = corner.IsInside(mLongTapOrigin);

            return false;
        }

        if (!mLongTapTracking)
            return false;

        // A tap that travels is a drag, not a long tap
        if (!o2Input.IsCursorDown() || (o2Input.GetCursorPos() - mLongTapOrigin).Length() > longTapCornerSize*0.25f)
        {
            mLongTapTracking = false;
            return false;
        }

        mLongTapTime += dt;
        if (mLongTapTime < longTapTime)
            return false;

        mLongTapTracking = false;

        return true;
    }

    void ProfilerOverlay::Update(float dt)
    {
        NanoProfiler::ExcludeScope exclude;

        mDrawnThisFrame = false;

        if (CheckToggleInput(dt))
            SetVisible(!mVisible);

        if (!mVisible)
            return;

        mRoot->Update(dt);
        mRoot->UpdateChildren(dt);
    }

    void ProfilerOverlay::DrawIn(const RectF& area)
    {
        if (!mVisible)
            return;

        NanoProfiler::ExcludeScope exclude;

        mDrawnThisFrame = true;

        LayoutIn(area);
        mRoot->Draw();
    }

    void ProfilerOverlay::Draw()
    {
        if (!mVisible || mDrawnThisFrame)
            return;

        NanoProfiler::ExcludeScope exclude;

        const Camera prevCamera = o2Render.GetCamera();
        o2Render.SetCamera(Camera::Default());

        OnApplicationSized();
        mRoot->Draw();

        o2Render.SetCamera(prevCamera);
    }
}

#endif // O2_PROFILER_ENABLED
