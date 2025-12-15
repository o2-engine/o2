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

#include <chrono>
#include <thread>

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
    FORWARD_REF_IMPL(TaskManager);
    FORWARD_REF_IMPL(Time);
    FORWARD_REF_IMPL(UIManager);

#if IS_SCRIPTING_SUPPORTED
    FORWARD_REF_IMPL(ScriptEngine);
#endif

	Integration::Integration()
    {}

    Integration::~Integration()
    {}

	void Integration::Initialize()
	{
        BasicInitialize();
	}

	void Integration::BasicInitialize()
    {
        PROFILE_SAMPLE_FUNC();

        InitalizeSystems();
        InitializePlatform();

        mRender = mmake<Render>();

        o2Debug.InitializeFont();
        o2UI.TryLoadStyle();

        mReady = true;
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

        mTimer.Reset();

        mEventSystem = mmake<EventSystem>();

        mUIManager = mmake<UIManager>();

        mScene = mmake<Scene>();

        mPhysics = mmake<PhysicsWorld>();

#if IS_SCRIPTING_SUPPORTED
        mScriptingEngine = mmake<ScriptEngine>();
#endif

        mLog->Out("Initialized");
    }

    void Integration::DeinitializeSystems()
    {
        Scene::DestroySingleton(mScene);
        Input::DestroySingleton(mInput);
        ProjectConfig::DestroySingleton(mProjectConfig);
        PhysicsWorld::DestroySingleton(mPhysics);
        TaskManager::DestroySingleton(mTaskManager);
        UIManager::DestroySingleton(mUIManager);
        EventSystem::DestroySingleton(mEventSystem);
		o2Debug.DeinitializeFont();
		Assets::DestroySingleton(mAssets);
        Render::DestroySingleton(mRender);
        Time::DestroySingleton(mTime);
        
        mLog = nullptr;
    }

    void Integration::ProcessFrame()
    {
        PROFILE_SAMPLE_FUNC();

        if (!mReady)
            return;

        float dt = 0, realDt = 0;

        {
            PROFILE_SAMPLE("ProcessFrame:Begin");

            float maxFPSDeltaTime = 1.0f/(float)maxFPS;

            realDt = mTimer.GetDeltaTime();

            if (realDt < maxFPSDeltaTime)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds((int)((maxFPSDeltaTime - realDt)*1000.0f)));
                realDt = maxFPSDeltaTime;
            }

            dt = Math::Clamp(realDt, 0.001f, 0.05f);
        }

        mInput->PreUpdate();

        mTime->Update(realDt);
        UpdateDebug(dt);
        UpdateTaskManager(dt);
        UpdateEventSystem();

        mRender->Begin();

        OnUpdate(dt);
        UpdateScene(dt);

        {
            PROFILE_SAMPLE("ProcessFrame:Fixed update loop");

            mAccumulatedDT += dt;
            float fixedDT = 1.0f/(float)fixedFPS;
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

        PostUpdateEventSystem();
        
        mMainListenersLayer->OnBeginDraw();
        mRender->SetCamera(Camera());
        mMainListenersLayer->camera = o2Render.GetCamera();

        DrawScene();
        OnDraw();

        DrawUIManager();
        DrawDebug();

        mMainListenersLayer->OnEndDraw();
        mMainListenersLayer->OnDrawn(Camera::Default().GetBasis());
        
        if (o2Input.IsKeyDown(VK_F1))
            mRender->DrawCross(o2Input.cursorPos.Get(), 20, Color4::Red());

        mRender->End();

        mInput->Update(dt);
        mUIManager->Update();

        mAssets->CheckAssetsUnload();

        PROFILE_FRAME();
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
