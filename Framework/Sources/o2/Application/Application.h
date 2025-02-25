#pragma once

#include "o2/Events/CursorAreaEventsListenersLayer.h"
#include "o2/Utils/Function/Function.h"
#include "o2/Utils/Math/Vector2.h"
#include "o2/Utils/Property.h"
#include "o2/Utils/Singleton.h"
#include "o2/Utils/System/Time/Timer.h"
#include "o2/Utils/Types/String.h"

#if defined PLATFORM_WINDOWS
#include "o2/Application/Windows/ApplicationBase.h"
#elif defined PLATFORM_ANDROID
#include "o2/Application/Android/ApplicationBase.h"
#include <jni.h>
#include <android/asset_manager.h>
#elif defined PLATFORM_MAC
#include "o2/Application/Mac/ApplicationBase.h"
#elif defined PLATFORM_IOS
#include "o2/Application/iOS/ApplicationBase.h"
#elif defined PLATFORM_LINUX
#include "o2/Application/Linux/ApplicationBase.h"
#endif

// Application access macros
#define o2Application o2::Application::Instance()

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

    // -----------
    // Application
    // -----------
    class Application: public Singleton<Application>, public IObject, public ApplicationBase
    {
    public:
        PROPERTIES(Application);
        PROPERTY(bool, fullscreen, SetFullscreen, IsFullScreen);               // Full screen/window changing property
        PROPERTY(bool, resizible, SetResizible, IsResizible);                  // Resizible window property
        PROPERTY(Vec2I, windowSize, SetWindowSize, GetWindowSize);             // Window frame size property
        PROPERTY(Vec2I, windowContentSize, SetContentSize, GetContentSize);    // Window content frame size property
        PROPERTY(Vec2I, windowPosition, SetWindowPosition, GetWindowPosition); // Window position on screen property
        PROPERTY(String, windowCaption, SetWindowCaption, GetWindowCaption);   // Window caption property

    public:
        Function<void()> onActivated;   // On Activated event callbacks
        Function<void()> onDeactivated; // On deactivated event callbacks
        Function<void()> onStarted;     // On started event callbacks
        Function<void()> onClosing;     // On closing event callbacks
        Function<void()> onResizing;    // On resized app window callbacks. Ignoring on mobiles/tables
        Function<void()> onMoving;      // On moving app window callbacks. Ignoring on mobiles/tables

    public:
        int maxFPS = 600;  // Maximum frames per second
        int fixedFPS = 60; // Fixed frames per second

    public:
        // Default constructor
        Application(RefCounter* refCounter);

        // Destructor 
        virtual ~Application();
        
        // Platform-specific initializations
        void InitializePlatform();

        // Returns pointer to log object
        virtual const Ref<LogStream>& GetLog() const;

        // Shutting down application
        virtual void Shutdown();

        // Makes application fullscreen. On mobiles/tablets has no effect, just ignoring
        virtual void SetFullscreen(bool fullscreen = true);

        // Return true, if application is fullscreen On mobiles/tables always true
        virtual bool IsFullScreen() const;

        // Maximize application frame. Available only on PC
        virtual void Maximize();

        // Returns is frame maximized. Available only on PC
        virtual bool IsMaximized() const;

        // Sets application window as resizible. On mobiles/tablets has no effect, just ignoring
        virtual void SetResizible(bool resizible);

        // Returns true, if application is resizible. On mobiles/tablets always returns false
        virtual bool IsResizible() const;

        // Sets application window size. On mobiles/tablets has no effect, just ignoring
        virtual void SetWindowSize(const Vec2I& size);

        // Returns application window size. On mobiles/tablets returns content size
        virtual Vec2I GetWindowSize() const;

        // Sets application window position. On mobiles/tablets has no effect, just ignoring
        virtual void SetWindowPosition(const Vec2I& position);

        // Returns application window position. On mobiles/tablets return zero vector
        virtual Vec2I GetWindowPosition() const;

        // Sets application window caption. On mobiles/tablets has no effect, just ignoring
        virtual void SetWindowCaption(const String& caption);

        // Returns application window caption. On mobiles/tablets returns empty string
        virtual String GetWindowCaption() const;

        // Sets inside content size
        virtual void SetContentSize(const Vec2I& size);

        // Returns inside content size
        virtual Vec2I GetContentSize() const;

        // Returns device screen resolution
        virtual Vec2I GetScreenResolution() const;

        // Sets cursor type
        virtual void SetCursor(CursorType type);

        // Sets cursor position
        virtual void SetCursorPosition(const Vec2F& position);

        // Sets cursor infinite moves mode
        virtual void SetCursorInfiniteMode(bool enabled);

        // Returns is cursor infinite mode enabled
        virtual bool IsCursorInfiniteModeOn() const;

        // Is application run in editor
        virtual bool IsEditor() const;

        // Returns application's path
        virtual String GetBinPath() const;
        
        // Returns graphics scale
        virtual float GetGraphicsScale() const;

        // Returns is application ready to use
        static bool IsReady();

        IOBJECT(Application);

#if defined PLATFORM_WINDOWS
        // Initializes engine application
        virtual void Initialize();

        // Launching application cycle
        virtual void Launch();

#elif defined PLATFORM_ANDROID
        // Launching application
        virtual void Initialize(JNIEnv* env, jobject activity, AAssetManager* assetManager, String dataPath,
                                const Vec2I& resolution);

        // Launching application cycle
        virtual void Launch();

        // Updates frame
        void Update();
        
#elif defined PLATFORM_MAC
        // Initializes engine application
        virtual void Initialize();
        
        // Launching application cycle
        virtual void Launch();
        
        // Updates frame
        void Update();
        
#elif defined PLATFORM_IOS
        // Initializes engine and runs it
        virtual void Run(int argc, char * argv[]);
        
        // Updates frame
        void Update();
        
        // Launching application
        virtual void Launch();

#elif defined PLATFORM_LINUX
        // Initializes engine application
        virtual void Initialize();

        // Launching application cycle
        virtual void Launch();

#endif

    protected:
        bool mReady = false; // Is all systems is ready

        Ref<Assets>        mAssets;        // Assets
        Ref<EventSystem>   mEventSystem;   // Events processing system
        Ref<FileSystem>    mFileSystem;    // File system
        Ref<Input>         mInput;         // While application user input message
        Ref<LogStream>     mLog;           // Log stream with id "app", using only for application messages
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

        bool  mCursorInfiniteModeEnabled = false; // Is cursor infinite mode enabled
        Vec2F mCursorCorrectionDelta;             // Cursor corrections delta - result of infinite cursors offset

        float mAccumulatedDT = 0.0f; // Accumulated delta time for fixed FPS update
        
        Ref<CursorAreaEventListenersLayer> mMainListenersLayer; // Main listeners layer, required for processing default scaled camera
        
        float mGraphicsScale = 1.0f; // Application graphics scale. Used in mac for retina displays

        Vec2I  mWindowedSize; // Size of window

    protected:
        // Basic initialization for all platforms
        virtual void BasicInitialize();

        // It is called when application frame resized
        virtual void OnResized(const Vec2I& size);

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
        virtual void OnUpdate(float dt);

        // Calling on updating by fixed FPS
        virtual void OnFixedUpdate(float dt);
        
        // Setups default camera with scaled graphics
        virtual void SetupGraphicsScaledCamera();

        // Calling on drawing
        virtual void OnDraw();

        // Calling when application activated
        virtual void OnActivated();

        // Calling when application deactivated
        virtual void OnDeactivated();

        // Calling when application is starting
        virtual void OnStarted();

        // Calling when application is closing
        virtual void OnClosing();

        // Calling when application window resized. Ignoring on mobiles/tablets
        virtual void OnResizing();

        // Calling when application window moved. Ignoring on mobiles/tablets
        virtual void OnMoved();

        // Initializing all systems and log. Call it when creating applications
        virtual void InitalizeSystems();

        // Deinitializing systems
        virtual void DeinitializeSystems();

        // Processing frame update, drawing and input messages
        virtual void ProcessFrame();

        // Checks that cursor is near border and moves to opposite border if needs
        void CheckCursorInfiniteMode();

        friend class WndProcFunc;
    };
}
// --- META ---

CLASS_BASES_META(o2::Application)
{
    BASE_CLASS(o2::Singleton<Application>);
    BASE_CLASS(o2::IObject);
    BASE_CLASS(o2::ApplicationBase);
}
END_META;
CLASS_FIELDS_META(o2::Application)
{
    FIELD().PUBLIC().NAME(fullscreen);
    FIELD().PUBLIC().NAME(resizible);
    FIELD().PUBLIC().NAME(windowSize);
    FIELD().PUBLIC().NAME(windowContentSize);
    FIELD().PUBLIC().NAME(windowPosition);
    FIELD().PUBLIC().NAME(windowCaption);
    FIELD().PUBLIC().NAME(onActivated);
    FIELD().PUBLIC().NAME(onDeactivated);
    FIELD().PUBLIC().NAME(onStarted);
    FIELD().PUBLIC().NAME(onClosing);
    FIELD().PUBLIC().NAME(onResizing);
    FIELD().PUBLIC().NAME(onMoving);
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
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mCursorInfiniteModeEnabled);
    FIELD().PROTECTED().NAME(mCursorCorrectionDelta);
    FIELD().PROTECTED().DEFAULT_VALUE(0.0f).NAME(mAccumulatedDT);
    FIELD().PROTECTED().NAME(mMainListenersLayer);
    FIELD().PROTECTED().DEFAULT_VALUE(1.0f).NAME(mGraphicsScale);
    FIELD().PROTECTED().NAME(mWindowedSize);
}
END_META;
CLASS_METHODS_META(o2::Application)
{

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().SIGNATURE(void, InitializePlatform);
    FUNCTION().PUBLIC().SIGNATURE(const Ref<LogStream>&, GetLog);
    FUNCTION().PUBLIC().SIGNATURE(void, Shutdown);
    FUNCTION().PUBLIC().SIGNATURE(void, SetFullscreen, bool);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsFullScreen);
    FUNCTION().PUBLIC().SIGNATURE(void, Maximize);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsMaximized);
    FUNCTION().PUBLIC().SIGNATURE(void, SetResizible, bool);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsResizible);
    FUNCTION().PUBLIC().SIGNATURE(void, SetWindowSize, const Vec2I&);
    FUNCTION().PUBLIC().SIGNATURE(Vec2I, GetWindowSize);
    FUNCTION().PUBLIC().SIGNATURE(void, SetWindowPosition, const Vec2I&);
    FUNCTION().PUBLIC().SIGNATURE(Vec2I, GetWindowPosition);
    FUNCTION().PUBLIC().SIGNATURE(void, SetWindowCaption, const String&);
    FUNCTION().PUBLIC().SIGNATURE(String, GetWindowCaption);
    FUNCTION().PUBLIC().SIGNATURE(void, SetContentSize, const Vec2I&);
    FUNCTION().PUBLIC().SIGNATURE(Vec2I, GetContentSize);
    FUNCTION().PUBLIC().SIGNATURE(Vec2I, GetScreenResolution);
    FUNCTION().PUBLIC().SIGNATURE(void, SetCursor, CursorType);
    FUNCTION().PUBLIC().SIGNATURE(void, SetCursorPosition, const Vec2F&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetCursorInfiniteMode, bool);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsCursorInfiniteModeOn);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsEditor);
    FUNCTION().PUBLIC().SIGNATURE(String, GetBinPath);
    FUNCTION().PUBLIC().SIGNATURE(float, GetGraphicsScale);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(bool, IsReady);
#if  defined PLATFORM_WINDOWS
    FUNCTION().PUBLIC().SIGNATURE(void, Initialize);
    FUNCTION().PUBLIC().SIGNATURE(void, Launch);
#endif
#if  defined PLATFORM_ANDROID
    FUNCTION().PUBLIC().SIGNATURE(void, Initialize, JNIEnv*, jobject, AAssetManager*, String, const Vec2I&);
    FUNCTION().PUBLIC().SIGNATURE(void, Launch);
    FUNCTION().PUBLIC().SIGNATURE(void, Update);
#endif
#if  defined PLATFORM_MAC
    FUNCTION().PUBLIC().SIGNATURE(void, Initialize);
    FUNCTION().PUBLIC().SIGNATURE(void, Launch);
    FUNCTION().PUBLIC().SIGNATURE(void, Update);
#endif
#if  defined PLATFORM_IOS
    FUNCTION().PUBLIC().SIGNATURE(void, Run, int, char);
    FUNCTION().PUBLIC().SIGNATURE(void, Update);
    FUNCTION().PUBLIC().SIGNATURE(void, Launch);
#endif
#if  defined PLATFORM_LINUX
    FUNCTION().PUBLIC().SIGNATURE(void, Initialize);
    FUNCTION().PUBLIC().SIGNATURE(void, Launch);
#endif
    FUNCTION().PROTECTED().SIGNATURE(void, BasicInitialize);
    FUNCTION().PROTECTED().SIGNATURE(void, OnResized, const Vec2I&);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateScene, float);
    FUNCTION().PROTECTED().SIGNATURE(void, FixedUpdateScene, float);
    FUNCTION().PROTECTED().SIGNATURE(void, PreUpdatePhysics);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdatePhysics, float);
    FUNCTION().PROTECTED().SIGNATURE(void, PostUpdatePhysics);
    FUNCTION().PROTECTED().SIGNATURE(void, DrawScene);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateEventSystem);
    FUNCTION().PROTECTED().SIGNATURE(void, PostUpdateEventSystem);
    FUNCTION().PROTECTED().SIGNATURE(void, DrawUIManager);
    FUNCTION().PROTECTED().SIGNATURE(void, DrawDebug);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateDebug, float);
    FUNCTION().PROTECTED().SIGNATURE(void, OnUpdate, float);
    FUNCTION().PROTECTED().SIGNATURE(void, OnFixedUpdate, float);
    FUNCTION().PROTECTED().SIGNATURE(void, SetupGraphicsScaledCamera);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDraw);
    FUNCTION().PROTECTED().SIGNATURE(void, OnActivated);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDeactivated);
    FUNCTION().PROTECTED().SIGNATURE(void, OnStarted);
    FUNCTION().PROTECTED().SIGNATURE(void, OnClosing);
    FUNCTION().PROTECTED().SIGNATURE(void, OnResizing);
    FUNCTION().PROTECTED().SIGNATURE(void, OnMoved);
    FUNCTION().PROTECTED().SIGNATURE(void, InitalizeSystems);
    FUNCTION().PROTECTED().SIGNATURE(void, DeinitializeSystems);
    FUNCTION().PROTECTED().SIGNATURE(void, ProcessFrame);
    FUNCTION().PROTECTED().SIGNATURE(void, CheckCursorInfiniteMode);
}
END_META;
// --- END META ---
