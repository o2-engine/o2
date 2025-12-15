#pragma once

#include "o2/Events/CursorAreaEventsListenersLayer.h"
#include "o2/Utils/Function/Function.h"
#include "o2/Utils/Math/Vector2.h"
#include "o2/Utils/Property.h"
#include "o2/Utils/Singleton.h"
#include "o2/Utils/System/Time/Timer.h"
#include "o2/Utils/Types/String.h"

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
    FORWARD_CLASS_REF(TaskManager);
    FORWARD_CLASS_REF(Time);
    FORWARD_CLASS_REF(UIManager);

#if IS_SCRIPTING_SUPPORTED
    FORWARD_CLASS_REF(ScriptEngine);
#endif

    // ----------------------------------------------------------------------------------
	// o2 engine integration class. Initializes and holds main systems, updates and draws
	// ----------------------------------------------------------------------------------
    class Integration: public IObject
    {
    public:
        int maxFPS = 600;  // Maximum frames per second
        int fixedFPS = 60; // Fixed frames per second

    public:
        // Default constructor
        Integration();

        // Destructor 
        virtual ~Integration();

        // Returns pointer to log object
        virtual const Ref<LogStream>& GetLog() const;

        // Is integration run in editor
        virtual bool IsEditor() const;

        // Returns is integration ready to use
        bool IsReady();

        IOBJECT(Integration);

    protected:
        bool mReady = false; // Are all systems ready

        Ref<Assets>        mAssets;        // Assets
        Ref<EventSystem>   mEventSystem;   // Events processing system
        Ref<FileSystem>    mFileSystem;    // File system
        Ref<Input>         mInput;         // Whole application user input message
        Ref<LogStream>     mLog;           // Log stream with id "app", using only for integration messages
        Ref<PhysicsWorld>  mPhysics;       // Physics
        Ref<ProjectConfig> mProjectConfig; // Project config
        Ref<Render>        mRender;        // Graphics render
        Ref<Scene>         mScene;         // Scene
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
		// Initializes integration
        virtual void Initialize();

        // Basic initialization for all platforms
        virtual void BasicInitialize();
        
        // Platform-specific initializations
		virtual void InitializePlatform();

		// Initializes all systems and log. Called when creating integration
		virtual void InitalizeSystems();
        
        // Deinitializes integration
		virtual void Deinitialize();

		// Deinitializing systems
		virtual void DeinitializeSystems();

        // It is called when integration frame resized
		virtual void OnResized(const Vec2I& size);

		// Processing frame update, drawing and input messages
		virtual void ProcessFrame();

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
    };
}
// --- META ---

CLASS_BASES_META(o2::Integration)
{
    BASE_CLASS(o2::IObject);
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

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().SIGNATURE(const Ref<LogStream>&, GetLog);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsEditor);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsReady);
    FUNCTION().PROTECTED().SIGNATURE(void, Initialize);
    FUNCTION().PROTECTED().SIGNATURE(void, BasicInitialize);
    FUNCTION().PROTECTED().SIGNATURE(void, InitializePlatform);
    FUNCTION().PROTECTED().SIGNATURE(void, InitalizeSystems);
    FUNCTION().PROTECTED().SIGNATURE(void, Deinitialize);
    FUNCTION().PROTECTED().SIGNATURE(void, DeinitializeSystems);
    FUNCTION().PROTECTED().SIGNATURE(void, OnResized, const Vec2I&);
    FUNCTION().PROTECTED().SIGNATURE(void, ProcessFrame);
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
