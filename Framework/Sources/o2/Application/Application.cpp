#include "o2/stdafx.h"
#include "o2/Application/Application.h"

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

    DECLARE_SINGLETON(Application);

    Application::Application(RefCounter* refCounter):
        Singleton<Application>(refCounter)
    {}

    Application::~Application()
    {
        DeinitializeSystems();
    }

    void Application::BasicInitialize()
    {
        PROFILE_SAMPLE_FUNC();

        InitalizeSystems();
        InitializePlatform();

        mRender = mmake<Render>();

        o2Debug.InitializeFont();
        o2UI.TryLoadStyle();

        mReady = true;
    }

    void Application::OnResized(const Vec2I& size)
    {
        mWindowedSize = size;
        mRender->OnFrameResized();
        onResizing.Invoke();
        OnResizing();
        o2Events.OnApplicationSized();
    }

    void Application::UpdateScene(float dt)
    {
        PROFILE_SAMPLE_FUNC();
        mScene->Update(dt);
    }

    void Application::FixedUpdateScene(float dt)
    {
        PROFILE_SAMPLE_FUNC();
        mScene->FixedUpdate(dt);
    }

    void Application::PreUpdatePhysics()
    {
        PROFILE_SAMPLE_FUNC();
        mPhysics->PreUpdate();
    }

    void Application::UpdatePhysics(float dt)
    {
        PROFILE_SAMPLE_FUNC();
        mPhysics->Update(dt);
    }

    void Application::PostUpdatePhysics()
    {
        PROFILE_SAMPLE_FUNC();
        mPhysics->PostUpdate();
    }

    void Application::InitalizeSystems()
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

    void Application::DeinitializeSystems()
    {
        mScene = nullptr;
        mInput = nullptr;
        mProjectConfig = nullptr;
        mPhysics = nullptr;
        mTaskManager = nullptr;
        mUIManager = nullptr;
        mEventSystem = nullptr;
        mRender = nullptr;
        mFileSystem = nullptr;
        mTime = nullptr;
        mLog = nullptr;

        Assets::DestroySingleton(mAssets);
    }

    void Application::ProcessFrame()
    {
        PROFILE_SAMPLE_FUNC();

        if (!mReady)
            return;

        float dt = 0, realDt = 0;

        {
            PROFILE_SAMPLE("ProcessFrame:Begin");

            if (mCursorInfiniteModeEnabled)
                CheckCursorInfiniteMode();

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
        mTaskManager->Update(dt);
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

        mRender->End();

        mInput->Update(dt);
        mUIManager->Update();

        mAssets->CheckAssetsUnload();

        PROFILE_FRAME();
    }

    void Application::DrawScene()
    {
        PROFILE_SAMPLE_FUNC();
        mScene->Draw();
    }

    void Application::UpdateEventSystem()
    {
        PROFILE_SAMPLE_FUNC();
        mEventSystem->Update();
    }

    void Application::PostUpdateEventSystem()
    {
        PROFILE_SAMPLE_FUNC();
        mEventSystem->PostUpdate();
    }

    void Application::DrawUIManager()
    {
        PROFILE_SAMPLE_FUNC();
        mUIManager->DrawCurrentLayerTopWidgets();
    }

    void Application::DrawDebug()
    {
        o2Debug.Draw(false);
    }

    void Application::UpdateDebug(float dt)
    {
        o2Debug.Update(false, dt);
    }

    void Application::OnMoved()
    {}

    void Application::OnResizing()
    {}

    void Application::OnClosing()
    {}

    void Application::OnStarted()
    {}

    void Application::OnDeactivated()
    {}

    void Application::OnActivated()
    {}

    void Application::OnUpdate(float dt)
    {}

    void Application::OnFixedUpdate(float dt)
    {}

    void Application::OnDraw()
    {}

    bool Application::IsReady()
    {
        return IsSingletonInitialzed() && Application::Instance().mReady;
    }

    void Application::SetCursorInfiniteMode(bool enabled)
    {
        mCursorInfiniteModeEnabled = enabled;
    }

    bool Application::IsCursorInfiniteModeOn() const
    {
        return mCursorInfiniteModeEnabled;
    }

    bool Application::IsEditor() const
    {
        return IS_EDITOR;
    }

    const Ref<LogStream>& Application::GetLog() const
    {
        return mInstance->mLog;
    }
    
    float Application::GetGraphicsScale() const
    {
        return mGraphicsScale;
    }

    MemoryManager* MemoryManager::mInstance = new MemoryManager();
    CREATE_SINGLETON(Debug);
    CREATE_SINGLETON(FileSystem);
}
// --- META ---

DECLARE_CLASS(o2::Application, o2__Application);
// --- END META ---
