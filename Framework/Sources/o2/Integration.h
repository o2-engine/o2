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
    FORWARD_CLASS_REF(ProjectConfig);
    FORWARD_CLASS_REF(Render);
    FORWARD_CLASS_REF(Scene);
    FORWARD_CLASS_REF(SoundSystem);
    FORWARD_CLASS_REF(TaskManager);
    FORWARD_CLASS_REF(Time);
    FORWARD_CLASS_REF(UIManager);

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

		// Enables headless mode: skips window creation, render and UI styles initialization,
		// and routes asserts to the log instead of popping modal dialogs / debugbreaking.
		// Must be called before Initialize(). Intended for unit-test runners.
		static void SetHeadless(bool headless);

		// Returns true if integration was started in headless mode @SCRIPTABLE
		static bool IsHeadless();

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

        IOBJECT(Integration);

    protected:
        bool mReady = false; // Are all systems ready

        static bool sHeadless; // Headless mode: skip window + render + UI styles init,
                               // and route asserts to the log

        Ref<Assets>        mAssets;        // Assets
        Ref<EventSystem>   mEventSystem;   // Events processing system
        Ref<FileSystem>    mFileSystem;    // File system
        Ref<Input>         mInput;         // Whole application user input message
        Ref<LogStream>     mLog;           // Log stream with id "app", using only for integration messages
        Ref<PhysicsWorld>  mPhysics;       // Physics
        Ref<ProjectConfig> mProjectConfig; // Project config
        Ref<Render>        mRender;        // Graphics render
        Ref<Scene>         mScene;         // Scene
        Ref<SoundSystem>   mSounds;        // Sound system
        Ref<TaskManager>   mTaskManager;   // Tasks manager
        Ref<Time>          mTime;          // Time utilities
        Ref<UIManager>     mUIManager;     // UI manager>

#if IS_SCRIPTING_SUPPORTED
        Ref<ScriptEngine>  mScriptingEngine; // Scripting engine
#endif

        Timer mTimer; // Timer for detecting delta time for update

        float mAccumulatedDT = 0.0f; // Accumulated delta time for fixed FPS update
        
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

		// Processing frame update, drawing and input messages
		virtual void ProcessFrame();

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
    FIELD().PROTECTED().NAME(mProjectConfig);
    FIELD().PROTECTED().NAME(mRender);
    FIELD().PROTECTED().NAME(mScene);
    FIELD().PROTECTED().NAME(mSounds);
    FIELD().PROTECTED().NAME(mTaskManager);
    FIELD().PROTECTED().NAME(mTime);
    FIELD().PROTECTED().NAME(mUIManager);
#if  IS_SCRIPTING_SUPPORTED
    FIELD().PROTECTED().NAME(mScriptingEngine);
#endif
    FIELD().PROTECTED().NAME(mTimer);
    FIELD().PROTECTED().DEFAULT_VALUE(0.0f).NAME(mAccumulatedDT);
    FIELD().PROTECTED().NAME(mMainListenersLayer);
}
END_META;
CLASS_METHODS_META(o2::Integration)
{

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().SIGNATURE(const Ref<LogStream>&, GetLog);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsEditor);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsReady);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(void, SetHeadless, bool);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE_STATIC(bool, IsHeadless);
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
    FUNCTION().PROTECTED().SIGNATURE(void, BasicInitialize);
    FUNCTION().PROTECTED().SIGNATURE(void, InitializePlatform);
    FUNCTION().PROTECTED().SIGNATURE(void, InitalizeSystems);
    FUNCTION().PROTECTED().SIGNATURE(void, InitiazeRender);
    FUNCTION().PROTECTED().SIGNATURE(void, InitilizeUIStyles);
    FUNCTION().PROTECTED().SIGNATURE(void, DeinitializeSystems);
    FUNCTION().PROTECTED().SIGNATURE(void, OnResized, const Vec2I&);
    FUNCTION().PROTECTED().SIGNATURE(void, ProcessFrame);
    FUNCTION().PROTECTED().SIGNATURE(void, CalculateAndSyncFPS, float&, float&);
    FUNCTION().PROTECTED().SIGNATURE(void, PreUpdateFrame, float, float);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateFrameFixed, float);
    FUNCTION().PROTECTED().SIGNATURE(void, MainUpdateFrame, float);
    FUNCTION().PROTECTED().SIGNATURE(void, PreDrawFrame);
    FUNCTION().PROTECTED().SIGNATURE(void, DrawFrame);
    FUNCTION().PROTECTED().SIGNATURE(void, PostDrawFrame);
    FUNCTION().PROTECTED().SIGNATURE(void, PostUpdateFrame, float);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateScene, float);
    FUNCTION().PROTECTED().SIGNATURE(void, FixedUpdateScene, float);
    FUNCTION().PROTECTED().SIGNATURE(void, PreUpdatePhysics);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdatePhysics, float);
    FUNCTION().PROTECTED().SIGNATURE(void, PostUpdatePhysics);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateTaskManager, float);
    FUNCTION().PROTECTED().SIGNATURE(void, DrawScene);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateEventSystem);
    FUNCTION().PROTECTED().SIGNATURE(void, PostUpdateEventSystem);
    FUNCTION().PROTECTED().SIGNATURE(void, DrawUIManager);
    FUNCTION().PROTECTED().SIGNATURE(void, DrawDebug);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateDebug, float);
    FUNCTION().PROTECTED().SIGNATURE(void, OnUpdate, float);
    FUNCTION().PROTECTED().SIGNATURE(void, OnFixedUpdate, float);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDraw);
}
END_META;
// --- END META ---
