#include "o2/stdafx.h"
#include "o2/Integration.h"

#include "o2/Application/Input.h"
#include "o2/Assets/Assets.h"
#include "o2/Config/ProjectConfig.h"
#include "o2/Events/EventSystem.h"
#include "o2/Physics/PhysicsWorld.h"
#include "o2/Render/Render.h"
#include "o2/Scene/Scene.h"
#include "o2/Scene/UI/UIManager.h"
#include "o2/Sound/SoundSystem.h"
#include "o2/Utils/Debug/Debug.h"
#include "o2/Utils/Debug/Log/ConsoleLogStream.h"
#include "o2/Utils/Debug/Log/FileLogStream.h"
#include "o2/Utils/Debug/Log/LogStream.h"
#include "o2/Utils/Debug/Profiling/SimpleProfiler.h"
#include "o2/Utils/Debug/StackTrace.h"
#include "o2/Utils/Editor/EditorScope.h"
#include "o2/Utils/FileSystem/FileSystem.h"
#include "o2/Utils/System/Time/Time.h"
#include "o2/Utils/System/Time/Timer.h"
#include "o2/Utils/Tasks/TaskManager.h"
#include "o2/Utils/Jobs/JobSystem.h"
#include "o2/Utils/Coroutines/CoroutineScheduler.h"
#include "o2/Utils/Coroutines/Coroutines.h"

#include <chrono>
#include <thread>

#ifdef PLATFORM_WINDOWS
#include <windows.h>
#include <crtdbg.h>
#include <stdlib.h>
#endif

#if IS_SCRIPTING_SUPPORTED
#include "o2/Scripts/ScriptEngine.h"
#endif

namespace o2
{
    FORWARD_REF_IMPL(Assets);
    FORWARD_REF_IMPL(EventSystem);
    FORWARD_REF_IMPL(FileSystem);
    FORWARD_REF_IMPL(Input);
    FORWARD_REF_IMPL(PhysicsWorld);
    FORWARD_REF_IMPL(ProjectConfig);
    FORWARD_REF_IMPL(Render);
    FORWARD_REF_IMPL(Scene);
    FORWARD_REF_IMPL(SoundSystem);
    FORWARD_REF_IMPL(TaskManager);
    FORWARD_REF_IMPL(JobSystem);
    FORWARD_REF_IMPL(CoroutineScheduler);
    FORWARD_REF_IMPL(Time);
    FORWARD_REF_IMPL(UIManager);

#if IS_SCRIPTING_SUPPORTED
    FORWARD_REF_IMPL(ScriptEngine);
#endif

	DECLARE_SINGLETON(Integration);

    bool Integration::sHeadless = false;

	Integration::Integration(RefCounter* refCounter):
        Singleton<Integration>(refCounter)
    {}

    Integration::~Integration()
    {}

	void Integration::BasicInitialize()
    {
        PROFILE_SAMPLE_FUNC();

        InitalizeSystems();

        if (!sHeadless)
        {
            InitializePlatform();
            InitiazeRender();
            InitilizeUIStyles();
        }

        mReady = true;
    }

    void Integration::SetHeadless(bool headless)
    {
        sHeadless = headless;

#ifdef PLATFORM_WINDOWS
        if (headless)
        {
            // Suppress OS / CRT modal dialogs that would block headless test runs
            // (GP-fault popup, Windows error reporting, abort dialog, CRT _ASSERT box).
            SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX);
            _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);

            for (int reportType : { _CRT_WARN, _CRT_ERROR, _CRT_ASSERT })
            {
                _CrtSetReportMode(reportType, _CRTDBG_MODE_FILE);
                _CrtSetReportFile(reportType, _CRTDBG_FILE_STDERR);
            }
        }
#endif
    }

    bool Integration::IsHeadless()
    {
        return sHeadless;
    }
     
	void Integration::InitializePlatform()
	{}

	void Integration::Deinitialize()
    {
        DeinitializeSystems();
    }

    void Integration::OnResized(const Vec2I& size)
    {        
        if (!mReady)
            return;
        
        mRender->OnFrameResized();
        o2Events.OnApplicationSized();
    }

    void Integration::UpdateScene(float dt)
    {
        PROFILE_SAMPLE_FUNC();
        mScene->Update(dt);
    }

    void Integration::FixedUpdateScene(float dt)
    {
        PROFILE_SAMPLE_FUNC();
        mScene->FixedUpdate(dt);
    }

    void Integration::PreUpdatePhysics()
    {
        PROFILE_SAMPLE_FUNC();
        mPhysics->PreUpdate();
    }

    void Integration::UpdatePhysics(float dt)
    {
        PROFILE_SAMPLE_FUNC();
        mPhysics->Update(dt);
    }

    void Integration::PostUpdatePhysics()
    {
        PROFILE_SAMPLE_FUNC();
        mPhysics->PostUpdate();
    }

    void Integration::UpdateTaskManager(float dt)
    {
        mTaskManager->Update(dt);
    }

    void Integration::InitalizeSystems()
    {
        PROFILE_SAMPLE_FUNC();

        srand((UInt)time(NULL));

        mTime = mmake<Time>();

        mLog = mmake<LogStream>("Application");
        o2Debug.GetLog()->BindStream(mLog);

        mProjectConfig = mmake<ProjectConfig>();

        mAssets = mmake<Assets>();

        mInput = mmake<Input>();
        mMainListenersLayer = mmake<CursorAreaEventListenersLayer>();

        mTaskManager = mmake<TaskManager>();

        mJobSystem = mmake<JobSystem>();
        mJobSystem->Initialize();

        mCoroutineScheduler = mmake<CoroutineScheduler>();
        mCoroutineScheduler->Initialize();

        mTimer.Reset();

        mEventSystem = mmake<EventSystem>();

        mUIManager = mmake<UIManager>();

        mScene = mmake<Scene>();

        mPhysics = mmake<PhysicsWorld>();

        mSounds = mmake<SoundSystem>();

#if IS_SCRIPTING_SUPPORTED
        mScriptingEngine = mmake<ScriptEngine>();
#endif

        mLog->Out("Initialized");
    }

	void Integration::InitiazeRender()
	{
		mRender = mmake<Render>();

		// Render on a parallel thread by default where the platform supports it: the main thread records
		// draw commands and the render thread submits them, synchronizing each frame
		mRender->SetMultithreadedRenderEnabled(true);

		o2Debug.InitializeFont();
	}

	void Integration::InitilizeUIStyles()
	{
		o2UI.TryLoadStyle();
	}

	void Integration::DeinitializeSystems()
    {
        // Stop and join worker threads first, before any other system is torn down, so no job can
        // touch a singleton that is going away. The coroutine scheduler's timer thread goes first,
        // as its timers reschedule work onto the job system
        mCoroutineScheduler->Shutdown();
        CoroutineScheduler::DestroySingleton(mCoroutineScheduler);

        mJobSystem->Shutdown();
        JobSystem::DestroySingleton(mJobSystem);

        mLifecycleStarted = false;

        Scene::DestroySingleton(mScene);
        Input::DestroySingleton(mInput);
        ProjectConfig::DestroySingleton(mProjectConfig);
        PhysicsWorld::DestroySingleton(mPhysics);
        TaskManager::DestroySingleton(mTaskManager);
        UIManager::DestroySingleton(mUIManager);
        EventSystem::DestroySingleton(mEventSystem);

        // In headless mode Render and the debug font were never constructed; skip them.
        // Otherwise preserve the original order: debug font + Assets first, then Render
        // (Render owns GL resources that some assets reference).
        if (mRender)
            o2Debug.DeinitializeFont();

        Assets::DestroySingleton(mAssets);

        SoundSystem::DestroySingleton(mSounds);

        if (mRender)
            Render::DestroySingleton(mRender);

        Time::DestroySingleton(mTime);

        mLog = nullptr;
    }

    void Integration::ProcessFrame()
    {
        PROFILE_SAMPLE_FUNC();

        if (!mReady)
            return;

        // The whole application lifecycle (loading + per-frame updates) is a coroutine. Each platform
        // frame advances it one step: wake the coroutine parked on WaitNextFrame, then run the queued
        // main-thread jobs (the lifecycle resume runs first at Critical priority and does the frame body)
        EnsureLifecycleStarted();
        o2Coroutines.OnNewFrame();
        o2Jobs.ExecuteMainThreadJobs(mMainThreadJobsQuota);
    }

    void Integration::ProcessFrameBody()
    {
        float dt = 0, realDt = 0;
		CalculateAndSyncFPS(dt, realDt);

		PreUpdateFrame(dt, realDt);
        MainUpdateFrame(dt);
		UpdateFrameFixed(dt);

		PreDrawFrame();
        DrawFrame();
        PostDrawFrame();

		PostUpdateFrame(dt);

        PROFILE_FRAME();
    }

    void Integration::EnsureLifecycleStarted()
    {
        if (mLifecycleStarted)
            return;

        mLifecycleStarted = true;
        PROFILE_THREAD("o2 Main Thread");

        // Captureless lambda coroutine: `self` is a parameter, copied into the coroutine frame, so it
        // stays valid across suspensions (unlike a captured `this`, which would dangle once the
        // temporary closure is destroyed). Started on the main thread at Critical priority so the frame
        // body runs before any user main-thread jobs and always completes despite the quota
        auto lifecycle = [](Integration* self) -> Coroutine<void> {
            self->OnLifecycleLoad();

            while (self->mReady)
            {
                self->ProcessFrameBody();
                co_await WaitNextFrame();
            }
        }(this);

        lifecycle.Start(JobThread::Main, JobPriority::Critical);
    }

    void Integration::OnLifecycleLoad()
    {}

    void Integration::SetMainThreadJobsQuota(float seconds)
    {
        mMainThreadJobsQuota = seconds;
    }

    float Integration::GetMainThreadJobsQuota() const
    {
        return mMainThreadJobsQuota;
    }

	void Integration::CalculateAndSyncFPS(float& dt, float& realDt)
	{
		PROFILE_SAMPLE("ProcessFrame:Begin");

		float maxFPSDeltaTime = 1.0f / (float)maxFPS;

		realDt = mTimer.GetDeltaTime();

		if (realDt < maxFPSDeltaTime)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds((int)((maxFPSDeltaTime - realDt) * 1000.0f)));
			realDt = maxFPSDeltaTime;
		}

		dt = Math::Clamp(realDt, 0.001f, 0.05f);
	}

	void Integration::PreUpdateFrame(float dt, float realDt)
	{
		mInput->PreUpdate();

		mTime->Update(realDt);
		UpdateDebug(dt);
		UpdateTaskManager(dt);
		UpdateEventSystem();
	}

	void Integration::UpdateFrameFixed(float dt)
	{
		PROFILE_SAMPLE("ProcessFrame:Fixed update loop");

		mAccumulatedDT += dt;
		float fixedDT = 1.0f / (float)fixedFPS;
		while (mAccumulatedDT > fixedDT)
		{
			OnFixedUpdate(fixedDT);
			FixedUpdateScene(fixedDT);

			PreUpdatePhysics();
			UpdatePhysics(fixedDT);
			PostUpdatePhysics();

			mAccumulatedDT -= fixedDT;
		}
	}

	void Integration::MainUpdateFrame(float dt)
	{
		OnUpdate(dt);
		UpdateScene(dt);

		PostUpdateEventSystem();
	}

	void Integration::PreDrawFrame()
	{
		mRender->Begin();

		mMainListenersLayer->OnBeginDraw();
		mRender->SetCamera(Camera());
		mMainListenersLayer->camera = o2Render.GetCamera();
	}

	void Integration::DrawFrame()
	{
		DrawScene();
		OnDraw();
		DrawUIManager();
		DrawDebug();
	}

	void Integration::PostDrawFrame()
	{
		mMainListenersLayer->OnEndDraw();
		mMainListenersLayer->OnDrawn(Camera::Default().GetBasis());

		if (o2Input.IsKeyDown(VK_F1))
			mRender->DrawCross(o2Input.cursorPos.Get(), 20, Color4::Red());

		mRender->End();
	}

	void Integration::PostUpdateFrame(float dt)
	{
		mInput->Update(dt);
		mUIManager->Update();

		mSounds->Update(dt);

		mAssets->CheckAssetsUnload();
	}

	void Integration::DrawScene()
    {
        PROFILE_SAMPLE_FUNC();
        mScene->Draw();
    }

    void Integration::UpdateEventSystem()
    {
        PROFILE_SAMPLE_FUNC();
        mEventSystem->Update();
    }

    void Integration::PostUpdateEventSystem()
    {
        PROFILE_SAMPLE_FUNC();
        mEventSystem->PostUpdate();
    }

    void Integration::DrawUIManager()
    {
        PROFILE_SAMPLE_FUNC();
        mUIManager->DrawCurrentLayerTopWidgets();
    }

    void Integration::DrawDebug()
    {
        o2Debug.Draw(false);
    }

    void Integration::UpdateDebug(float dt)
    {
        o2Debug.Update(false, dt);
    }

    bool Integration::IsReady()
    {
        return mReady;
    }

    bool Integration::IsEditor() const
    {
        return IS_EDITOR;
    }

    const Ref<LogStream>& Integration::GetLog() const
    {
        return mLog;
    }

    MemoryManager* MemoryManager::mInstance = new MemoryManager();
    CREATE_SINGLETON(Debug);
    CREATE_SINGLETON(FileSystem);
}
// --- META ---

DECLARE_CLASS(o2::Integration, o2__Integration);
// --- END META ---
