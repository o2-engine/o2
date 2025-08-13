#pragma once

#include "o2/Application/Application.h"
#include "o2/Events/CursorAreaEventsListenersLayer.h"
#include "o2/Render/Sprite.h"
#include "o2Editor/EditorConfig.h"
#include "o2/Assets/Types/SceneAsset.h"

using namespace o2;

namespace o2
{
    class MenuPanel;
}

// Editor application access macros
#define o2EditorApplication (*(EditorApplication*)(Application::InstancePtr()))

namespace Editor
{
    FORWARD_CLASS_REF(EditorConfig);
    FORWARD_CLASS_REF(IAction);
    FORWARD_CLASS_REF(MenuPanel);
    FORWARD_CLASS_REF(Properties);
    FORWARD_CLASS_REF(SceneWindow);
    FORWARD_CLASS_REF(ToolsPanel);
    FORWARD_CLASS_REF(UIRoot);
    FORWARD_CLASS_REF(WindowsManager);

    // ------------------
    // Editor application
    // ------------------
    class EditorApplication: public Application
    {
    public:
        bool isPaused = false;  // Is editor scene paused
        bool step = false;      // Is needed to do one update step

    public:
        // Default constructor. Initializes all editor components
        EditorApplication(RefCounter* refCounter);

        // Destructor
        ~EditorApplication();

        // Returns current scene name
        const String& GetLoadedSceneName() const;

        // Loads scene from file
        void LoadScene(const AssetRef<SceneAsset>& scene);

        // Saves current scene
        void SaveScene();

        // Saves scene from file
        void SaveSceneAs(const String& path);

        // Makes new scene
        void MakeNewScene();

        // Returns is current scene was changed
        bool IsSceneChanged() const;

        // Runs or stops scene playing. Saving scene into temporary file before playing, restoring after stopping
        void SetPlaying(bool playing);

        // Is scene playing
        bool IsPlaying() const;

        // Dynamic cast to RefCounterable via Application
        static Ref<RefCounterable> CastToRefCounterable(const Ref<EditorApplication>& ref);

        IOBJECT(EditorApplication);
        REF_COUNTERABLE_IMPL(Application);

    protected:
        Ref<Sprite> mBackground; // Background sprite
        Ref<Sprite> mBackSign;   // Background o2 signature

        Ref<UIRoot>         mUIRoot;         // Root editor UI
        Ref<WindowsManager> mWindowsManager; // Windows manager
        Ref<EditorConfig>   mConfig;         // Application configuration
        Ref<ToolsPanel>     mToolsPanel;     // Tools panel
        Ref<MenuPanel>      mMenuPanel;      // Menu panel

        Ref<Properties>  mProperties;  // Properties manager
        Ref<SceneWindow> mSceneWindow; // Scene window reference

        DataDocument mSceneDump; // Scene dump, created before playing

        bool mIsPlaying = false;      // Is editor scene playing
        bool mPlayingChanged = false; // True when need to update playing mode on update
        bool mUpdateStep = false;     // True when frame updating available on this frame

        int mDrawCalls = 0;       // Draw calls count, stored before beginning rendering
        int mDrawnPrimitives = 0; // DRaw promitives at frame

    protected:
        // Initializing all systems and log. Sets up editor settings for some systems
        void InitalizeSystems() override;
        
        // Deinitializes application
        void Deinitialize() override;

        // Check style rebuilding and loads editor UI style
        void LoadUIStyle();

        // Before update physics
        void PreUpdatePhysics() override;

        // Updates physics
        void UpdatePhysics(float dt) override;

        // After update physics
        void PostUpdatePhysics() override;

        // Updates scene
        void UpdateScene(float dt) override;

        // Updates scene with fixed delta time
        void FixedUpdateScene(float dt) override;

        // Updates task manager
        void UpdateTaskManager(float dt) override;

        // Draws scene
        void DrawScene() override;

        // Draws UI manager
        void DrawUIManager() override;

        // Draw debug empty stub
        void DrawDebug() override;

        // Updates debug inly in play mode
        void UpdateDebug(float dt) override;

        // Updates event system
        void UpdateEventSystem() override;

        // Post updates event system
        void PostUpdateEventSystem() override;

        // Calling on updating
        void OnUpdate(float dt) override;

        // Calling on drawing
        void OnDraw() override;

        // Calling when application activated
        void OnActivated() override;

        // Calling when application deactivated
        void OnDeactivated() override;

        // Calling when application is starting
        void OnStarted() override;

        // Calling when application is closing
        void OnClosing() override;

        // Calling when application window resized. Ignoring on mobiles/tablets
        void OnResizing() override;

        // Calling when application window moved. Ignoring on mobiles/tablets
        void OnMoved() override;

        // Processing frame update, drawing and input messages without scene
        void ProcessFrame() override;

        // Checks that required to switch playing mode
        void CheckPlayingSwitch();
    };
}
// --- META ---

CLASS_BASES_META(Editor::EditorApplication)
{
    BASE_CLASS(o2::Application);
}
END_META;
CLASS_FIELDS_META(Editor::EditorApplication)
{
    FIELD().PUBLIC().DEFAULT_VALUE(false).NAME(isPaused);
    FIELD().PUBLIC().DEFAULT_VALUE(false).NAME(step);
    FIELD().PROTECTED().NAME(mBackground);
    FIELD().PROTECTED().NAME(mBackSign);
    FIELD().PROTECTED().NAME(mUIRoot);
    FIELD().PROTECTED().NAME(mWindowsManager);
    FIELD().PROTECTED().NAME(mConfig);
    FIELD().PROTECTED().NAME(mToolsPanel);
    FIELD().PROTECTED().NAME(mMenuPanel);
    FIELD().PROTECTED().NAME(mProperties);
    FIELD().PROTECTED().NAME(mSceneWindow);
    FIELD().PROTECTED().NAME(mSceneDump);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mIsPlaying);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mPlayingChanged);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mUpdateStep);
    FIELD().PROTECTED().DEFAULT_VALUE(0).NAME(mDrawCalls);
    FIELD().PROTECTED().DEFAULT_VALUE(0).NAME(mDrawnPrimitives);
}
END_META;
CLASS_METHODS_META(Editor::EditorApplication)
{

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().SIGNATURE(const String&, GetLoadedSceneName);
    FUNCTION().PUBLIC().SIGNATURE(void, LoadScene, const AssetRef<SceneAsset>&);
    FUNCTION().PUBLIC().SIGNATURE(void, SaveScene);
    FUNCTION().PUBLIC().SIGNATURE(void, SaveSceneAs, const String&);
    FUNCTION().PUBLIC().SIGNATURE(void, MakeNewScene);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsSceneChanged);
    FUNCTION().PUBLIC().SIGNATURE(void, SetPlaying, bool);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsPlaying);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(Ref<RefCounterable>, CastToRefCounterable, const Ref<EditorApplication>&);
    FUNCTION().PROTECTED().SIGNATURE(void, InitalizeSystems);
    FUNCTION().PROTECTED().SIGNATURE(void, Deinitialize);
    FUNCTION().PROTECTED().SIGNATURE(void, LoadUIStyle);
    FUNCTION().PROTECTED().SIGNATURE(void, PreUpdatePhysics);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdatePhysics, float);
    FUNCTION().PROTECTED().SIGNATURE(void, PostUpdatePhysics);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateScene, float);
    FUNCTION().PROTECTED().SIGNATURE(void, FixedUpdateScene, float);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateTaskManager, float);
    FUNCTION().PROTECTED().SIGNATURE(void, DrawScene);
    FUNCTION().PROTECTED().SIGNATURE(void, DrawUIManager);
    FUNCTION().PROTECTED().SIGNATURE(void, DrawDebug);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateDebug, float);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateEventSystem);
    FUNCTION().PROTECTED().SIGNATURE(void, PostUpdateEventSystem);
    FUNCTION().PROTECTED().SIGNATURE(void, OnUpdate, float);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDraw);
    FUNCTION().PROTECTED().SIGNATURE(void, OnActivated);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDeactivated);
    FUNCTION().PROTECTED().SIGNATURE(void, OnStarted);
    FUNCTION().PROTECTED().SIGNATURE(void, OnClosing);
    FUNCTION().PROTECTED().SIGNATURE(void, OnResizing);
    FUNCTION().PROTECTED().SIGNATURE(void, OnMoved);
    FUNCTION().PROTECTED().SIGNATURE(void, ProcessFrame);
    FUNCTION().PROTECTED().SIGNATURE(void, CheckPlayingSwitch);
}
END_META;
// --- END META ---
