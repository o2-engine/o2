#pragma once

#include "o2/Integration.h"

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
#define o2Application dynamic_cast<o2::Application&>(o2Integration)

namespace o2
{
    // -------------------------------------------------------------------------
	// Application. Exdends integration interface, used to handle engine systems
	// -------------------------------------------------------------------------
    class Application: public ApplicationBase, public Integration
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
        Function<void()> onResizing;    // On resized app window callbacks. Ignoring on mobiles/tablets
        Function<void()> onMoving;      // On moving app window callbacks. Ignoring on mobiles/tablets

    public:
        // Default constructor
        Application(RefCounter* refCounter);

        // Destructor 
        virtual ~Application();

        // Shutting down application
        virtual void Shutdown();

        // Makes application fullscreen. On mobiles/tablets has no effect, just ignoring
        virtual void SetFullscreen(bool fullscreen = true);

        // Returns true if application is fullscreen. On mobiles/tablets always true
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

        // Returns application window position. On mobiles/tablets returns zero vector
        virtual Vec2I GetWindowPosition() const;

        // Sets application window caption. On mobiles/tablets has no effect, just ignoring
        virtual void SetWindowCaption(const String& caption);

        // Returns application window caption. On mobiles/tablets returns empty string
        virtual String GetWindowCaption() const;

        // Sets inside content size
        void SetContentSize(const Vec2I& size) override;

        // Returns inside content size
        Vec2I GetContentSize() const override;

        // Returns device screen resolution
        Vec2I GetScreenResolution() const override;

        // Sets cursor type
        void SetCursor(CursorType type) override;

        // Sets cursor position
        void SetCursorPosition(const Vec2F& position) override;

        // Sets cursor infinite moves mode
        void SetCursorInfiniteMode(bool enabled) override;

        // Returns is cursor infinite mode enabled
        bool IsCursorInfiniteModeOn() const override;
        
        // Returns graphics scale
		float GetGraphicsScale() const override;

		// Returns application's path
		String GetBinPath() const override;

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
        bool  mCursorInfiniteModeEnabled = false; // Is cursor infinite mode enabled
        Vec2F mCursorCorrectionDelta;             // Cursor corrections delta - result of infinite cursors offset
        
        float mGraphicsScale = 1.0f; // Application graphics scale. Used in mac for retina displays

        Vec2I  mWindowedSize; // Size of window

    protected:        
        // Platform-specific initializations
        void InitializePlatform() override;

        // Checks that cursor is near border and moves to opposite border if needs
        void CheckCursorInfiniteMode();

        // Platform-specific window size setting implementation
		void SetWindowSizePlatform(const Vec2I& size);

		// Processing frame update, drawing and input messages
		void ProcessFrame() override;

		// It is called when application frame resized
		void OnResized(const Vec2I& size) override;

		// Calling when application activated
        virtual void OnActivated() {}

		// Calling when application deactivated
        virtual void OnDeactivated() {}

		// Calling when application is starting
        virtual void OnStarted() {}

		// Calling when application is closing
        virtual void OnClosing() {}

		// Calling when application window resized. Ignoring on mobiles/tablets
        virtual void OnResizing() {}

		// Calling when application window moved. Ignoring on mobiles/tablets
        virtual void OnMoved() {}

        friend class WndProcFunc;
        friend struct ApplicationPlatformWrapper;
    };
}
// --- META ---

CLASS_BASES_META(o2::Application)
{
    BASE_CLASS(o2::ApplicationBase);
    BASE_CLASS(o2::Integration);
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
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mCursorInfiniteModeEnabled);
    FIELD().PROTECTED().NAME(mCursorCorrectionDelta);
    FIELD().PROTECTED().DEFAULT_VALUE(1.0f).NAME(mGraphicsScale);
    FIELD().PROTECTED().NAME(mWindowedSize);
}
END_META;
CLASS_METHODS_META(o2::Application)
{

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
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
    FUNCTION().PUBLIC().SIGNATURE(float, GetGraphicsScale);
    FUNCTION().PUBLIC().SIGNATURE(String, GetBinPath);
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
    FUNCTION().PROTECTED().SIGNATURE(void, InitializePlatform);
    FUNCTION().PROTECTED().SIGNATURE(void, CheckCursorInfiniteMode);
    FUNCTION().PROTECTED().SIGNATURE(void, SetWindowSizePlatform, const Vec2I&);
    FUNCTION().PROTECTED().SIGNATURE(void, ProcessFrame);
    FUNCTION().PROTECTED().SIGNATURE(void, OnResized, const Vec2I&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnActivated);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDeactivated);
    FUNCTION().PROTECTED().SIGNATURE(void, OnStarted);
    FUNCTION().PROTECTED().SIGNATURE(void, OnClosing);
    FUNCTION().PROTECTED().SIGNATURE(void, OnResizing);
    FUNCTION().PROTECTED().SIGNATURE(void, OnMoved);
}
END_META;
// --- END META ---
