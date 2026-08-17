#pragma once

#include "o2/Events/CursorAreaEventsListenersLayer.h"
#include "o2/Utils/Function/Function.h"
#include "o2/Utils/Math/Vector2.h"
#include "o2/Utils/Property.h"
#include "o2/Utils/Singleton.h"
#include "o2/Utils/System/Time/Timer.h"
#include "o2/Utils/Types/String.h"

// Integration access macros
#define o2Integration o2::Integration::Instance()


namespace o2
{
    FORWARD_CLASS_REF(Assets);
    FORWARD_CLASS_REF(EventSystem);
    FORWARD_CLASS_REF(FileSystem);
    FORWARD_CLASS_REF(Input);
    FORWARD_CLASS_REF(LogStream);
    FORWARD_CLASS_REF(PhysicsWorld);
    FORWARD_CLASS_REF(PhysicsWorld3D);
    FORWARD_CLASS_REF(ProjectConfig);
    FORWARD_CLASS_REF(Render);
    FORWARD_CLASS_REF(Scene);
    FORWARD_CLASS_REF(SoundSystem);
    FORWARD_CLASS_REF(TaskManager);
    FORWARD_CLASS_REF(JobSystem);
    FORWARD_CLASS_REF(CoroutineScheduler);
    FORWARD_CLASS_REF(Time);
    FORWARD_CLASS_REF(UIManager);

#if defined(O2_PROFILER_ENABLED)
    FORWARD_CLASS_REF(ProfilerOverlay);
#endif

#if IS_SCRIPTING_SUPPORTED
    FORWARD_CLASS_REF(ScriptEngine);
#endif

    // ----------------------------------------------------------------------------------
	// o2 engine integration class. Initializes and holds main systems, updates and draws
	// ----------------------------------------------------------------------------------
    class Integration: public IObject, public Singleton<Integration>
    {
    public:
        int maxFPS = 600;  // Maximum frames per second
        int fixedFPS = 60; // Fixed frames per second

        // The real frame delta is clamped into this range before it drives the update: a stalled frame
        // must not teleport the simulation, and a zero one must not stop it
        static constexpr float minFrameDeltaTime = 0.001f;
        static constexpr float maxFrameDeltaTime = 0.05f;

    public:
        // Default constructor
        Integration(RefCounter* refCounter);

        // Destructor 
        virtual ~Integration();

        // Returns pointer to log object
        virtual const Ref<LogStream>& GetLog() const;

        // Is integration run in editor
        virtual bool IsEditor() const;

        // Returns is integration ready to use
		bool IsReady();

		// Sets the per-frame time budget (seconds) for main-thread jobs. Negative means unlimited.
		// Best-effort: a running job is never interrupted, so the budget can be overrun by one job
		void SetMainThreadJobsQuota(float seconds);

		// Returns the per-frame main-thread jobs time budget in seconds
		float GetMainThreadJobsQuota() const;

		// Enables headless mode: skips window creation, render and UI styles initialization,
		// and routes asserts to the log instead of popping modal dialogs / debugbreaking.
		// Must be called before Initialize(). Intended for unit-test runners.
		static void SetHeadless(bool headless);

		// Returns true if integration was started in headless mode @SCRIPTABLE
		static bool IsHeadless();

		// Brings the application window up without taking the keyboard focus, and keeps the app out of
		// the task switcher. Must be called before Initialize(). Intended for test runners: a window
		// that grabs focus every launch makes the machine unusable while the suites run
		static void SetBackgroundWindow(bool background);

		// Returns true if the window is brought up without taking the focus
		static bool IsBackgroundWindow();

		// Tears down all subsystems in a controlled order. Normally called automatically
		// from Launch() at the end of the main loop. Test runners that skip Launch() must
		// call it explicitly before the Application Ref<> drops, otherwise members destruct
		// in declaration order and Scene can outlive Time/Input/TaskManager — which is the
		// root cause of teardown SEGFAULTs in component destructors.
		virtual void Deinitialize();

		// Sets inside content size
        virtual void SetContentSize(const Vec2I& size) {}

		// Returns inside content size
        virtual Vec2I GetContentSize() const { return Vec2I(); }

		// Returns device screen resolution
        virtual Vec2I GetScreenResolution() const { return Vec2I(); }

		// Sets cursor type
        virtual void SetCursor(CursorType type) {}

		// Sets cursor position
        virtual void SetCursorPosition(const Vec2F& position) {}

		// Sets cursor infinite moves mode
        virtual void SetCursorInfiniteMode(bool enabled) {}

		// Returns is cursor infinite mode enabled
		virtual bool IsCursorInfiniteModeOn() const { return false; }

		// Returns graphics scale
		virtual float GetGraphicsScale() const { return 1.0f; }

		// Returns application's path
		virtual String GetBinPath() const { return String(); }

		// Returns is platform-specific initialization needed. External hosts (cocos2d
		// integration) own the window and disable o2's platform initialization
		virtual bool IsNeedPlatformInitialization() const { return true; }

		// Draws external renderers (e.g. hosted cocos2d scene in the editor)
		virtual void DrawExternal() {}

        IOBJECT(Integration);

    protected:
        bool mReady = false; // Are all systems ready

        static bool sHeadless; // Headless mode: skip window + render + UI styles init,
                               // and route asserts to the log

        static bool sBackgroundWindow; // Show the window without taking the focus

        Ref<Assets>        mAssets;        // Assets
        Ref<EventSystem>   mEventSystem;   // Events processing system
        Ref<FileSystem>    mFileSystem;    // File system
        Ref<Input>         mInput;         // Whole application user input message
        Ref<LogStream>     mLog;           // Log stream with id "app", using only for integration messages
        Ref<PhysicsWorld>   mPhysics;       // Physics
        Ref<PhysicsWorld3D> mPhysics3D;     // 3D physics
        Ref<ProjectConfig> mProjectConfig; // Project config
        Ref<Render>        mRender;        // Graphics render
        Ref<Scene>         mScene;         // Scene
        Ref<SoundSystem>   mSounds;        // Sound system
        Ref<TaskManager>   mTaskManager;   // Tasks manager
        Ref<JobSystem>     mJobSystem;     // Parallel job system with worker threads
        Ref<CoroutineScheduler> mCoroutineScheduler; // Coroutine timer/next-frame scheduler
        Ref<Time>          mTime;          // Time utilities
        Ref<UIManager>     mUIManager;     // UI manager>

#if defined(O2_PROFILER_ENABLED)
        Ref<ProfilerOverlay> mProfilerOverlay; // On-screen profiler widget, shown by F12 or a long tap
#endif

#if IS_SCRIPTING_SUPPORTED
        Ref<ScriptEngine>  mScriptingEngine; // Scripting engine
#endif

        Timer mTimer; // Timer for detecting delta time for update

        float mAccumulatedDT = 0.0f; // Accumulated delta time for fixed FPS update

        bool  mLifecycleStarted = false;      // True once the lifecycle coroutine has been started
        float mMainThreadJobsQuota = -1.0f;   // Per-frame time budget for main-thread jobs, seconds. < 0 = unlimited

        Ref<CursorAreaEventListenersLayer> mMainListenersLayer; // Main listeners layer, required for processing default scaled camera

	protected:
		// Basic initialization for all platforms
		virtual void BasicInitialize();

		// Platform-specific initializations
		virtual void InitializePlatform();

		// Initializes all systems and log. Called when creating integration
		virtual void InitalizeSystems();

		// Initializes render system
		virtual void InitiazeRender();

		// Initializes UI styles
		virtual void InitilizeUIStyles();

		// Deinitializing systems
		virtual void DeinitializeSystems();

		// It is called when integration frame resized
		virtual void OnResized(const Vec2I& size);

		// Processing frame update, drawing and input messages. Drives the lifecycle coroutine one frame
		virtual void ProcessFrame();

		// Runs one frame's worth of update and drawing. Invoked by the lifecycle coroutine every frame
		void ProcessFrameBody();

		// Starts the application lifecycle coroutine on the first frame. The lifecycle runs OnLifecycleLoad
		// once and then ProcessFrameBody every frame, yielding via co_await WaitNextFrame
		void EnsureLifecycleStarted();

		// Loading stage of the lifecycle, called once before the frame loop. Override to load content
		virtual void OnLifecycleLoad();

		// Calculates delta time and syncs to max FPS
		virtual void CalculateAndSyncFPS(float& dt, float& realDt);

		// First stage of frame update
		virtual void PreUpdateFrame(float dt, float realDt);

		// Updates scene with fixed delta time
		virtual void UpdateFrameFixed(float dt);

		// Main stage of frame update
		virtual void MainUpdateFrame(float dt);

		// Pre drawing frame
        virtual void PreDrawFrame();

		// Drawing frame
		virtual void DrawFrame();

		// Post drawing frame
		virtual void PostDrawFrame();

		// Last stage of frame update
		virtual void PostUpdateFrame(float dt);

		// Updates scene
		virtual void UpdateScene(float dt);

		// Updates scene with fixed delta time
		virtual void FixedUpdateScene(float dt);

		// Before update physics
		virtual void PreUpdatePhysics();

		// Updates physics
		virtual void UpdatePhysics(float dt);

		// After update physics
		virtual void PostUpdatePhysics();

		// Before update 3D physics
		virtual void PreUpdatePhysics3D();

		// Updates 3D physics
		virtual void UpdatePhysics3D(float dt);

		// After update 3D physics
		virtual void PostUpdatePhysics3D();

		// Updates task manager
		virtual void UpdateTaskManager(float dt);

		// Draws scene
		virtual void DrawScene();

		// Updates event system
		virtual void UpdateEventSystem();

		// Post updates event system
		virtual void PostUpdateEventSystem();

		// Draws UI manager
		virtual void DrawUIManager();

		// Draws debug
		virtual void DrawDebug();

		// Updates debug
		virtual void UpdateDebug(float dt);

		// Handles the profiler widget input and updates it
		void UpdateProfiler(float dt);

		// Draws the profiler widget above everything else
		void DrawProfiler();

		// Calling on updating
		virtual void OnUpdate(float dt) {}

		// Calling on updating by fixed FPS
		virtual void OnFixedUpdate(float dt) {}

		// Calling on drawing
		virtual void OnDraw() {}

		friend class WndProcFunc;
		friend struct ApplicationPlatformWrapper;
    };
}
// --- META ---

CLASS_BASES_META(o2::Integration)
{
    BASE_CLASS(o2::IObject);
    BASE_CLASS(o2::Singleton<Integration>);
}
END_META;
CLASS_FIELDS_META(o2::Integration)
{
    FIELD().PUBLIC().DEFAULT_VALUE(600).NAME(maxFPS);
    FIELD().PUBLIC().DEFAULT_VALUE(60).NAME(fixedFPS);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mReady);
    FIELD().PROTECTED().NAME(mAssets);
    FIELD().PROTECTED().NAME(mEventSystem);
    FIELD().PROTECTED().NAME(mFileSystem);
    FIELD().PROTECTED().NAME(mInput);
    FIELD().PROTECTED().NAME(mLog);
    FIELD().PROTECTED().NAME(mPhysics);
    FIELD().PROTECTED().NAME(mPhysics3D);
    FIELD().PROTECTED().NAME(mProjectConfig);
    FIELD().PROTECTED().NAME(mRender);
    FIELD().PROTECTED().NAME(mScene);
    FIELD().PROTECTED().NAME(mSounds);
    FIELD().PROTECTED().NAME(mTaskManager);
    FIELD().PROTECTED().NAME(mJobSystem);
    FIELD().PROTECTED().NAME(mCoroutineScheduler);
    FIELD().PROTECTED().NAME(mTime);
    FIELD().PROTECTED().NAME(mUIManager);
#if  defined(O2_PROFILER_ENABLED)
    FIELD().PROTECTED().NAME(mProfilerOverlay);
#endif
#if  IS_SCRIPTING_SUPPORTED
    FIELD().PROTECTED().NAME(mScriptingEngine);
#endif
    FIELD().PROTECTED().NAME(mTimer);
    FIELD().PROTECTED().DEFAULT_VALUE(0.0f).NAME(mAccumulatedDT);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mLifecycleStarted);
    FIELD().PROTECTED().DEFAULT_VALUE(-1.0f).NAME(mMainThreadJobsQuota);
}
END_META;
CLASS_METHODS_META(o2::Integration)
{

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().SIGNATURE(const Ref<LogStream>&, GetLog);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsEditor);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsReady);
    FUNCTION().PUBLIC().SIGNATURE(void, SetMainThreadJobsQuota, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetMainThreadJobsQuota);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(void, SetHeadless, bool);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE_STATIC(bool, IsHeadless);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(void, SetBackgroundWindow, bool);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(bool, IsBackgroundWindow);
    FUNCTION().PUBLIC().SIGNATURE(void, Deinitialize);
    FUNCTION().PUBLIC().SIGNATURE(void, SetContentSize, const Vec2I&);
    FUNCTION().PUBLIC().SIGNATURE(Vec2I, GetContentSize);
    FUNCTION().PUBLIC().SIGNATURE(Vec2I, GetScreenResolution);
    FUNCTION().PUBLIC().SIGNATURE(void, SetCursor, CursorType);
    FUNCTION().PUBLIC().SIGNATURE(void, SetCursorPosition, const Vec2F&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetCursorInfiniteMode, bool);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsCursorInfiniteModeOn);
    FUNCTION().PUBLIC().SIGNATURE(float, GetGraphicsScale);
    FUNCTION().PUBLIC().SIGNATURE(String, GetBinPath);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsNeedPlatformInitialization);
    FUNCTION().PUBLIC().SIGNATURE(void, DrawExternal);
}
END_META;
// --- END META ---
