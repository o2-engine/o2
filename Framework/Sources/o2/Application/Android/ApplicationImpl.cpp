#include "o2/stdafx.h"

#ifdef PLATFORM_ANDROID

#include "o2/Application/Application.h"
#include "o2/Application/Android/AndroidPlatform.h"
#include "o2/Events/EventSystem.h"
#include "o2/Utils/Debug/Log/LogStream.h"
#include "o2/Utils/FileSystem/FileSystem.h"

namespace o2
{
    namespace AndroidPlatform
    {
        static JavaVM*        gJVM = nullptr;
        static jobject        gActivity = nullptr;
        static AAssetManager* gAssetManager = nullptr;
        static String         gDataPath;
        static Vec2I          gResolution = Vec2I(1280, 720);

        void SetJVM(JavaVM* jvm)                     { gJVM = jvm; }
        void SetActivity(jobject activity)           { gActivity = activity; }
        void SetAssetManager(AAssetManager* manager) { gAssetManager = manager; }
        void SetDataPath(const String& dataPath)     { gDataPath = dataPath; }
        void SetResolution(const Vec2I& resolution)  { gResolution = resolution; }

        JavaVM*        GetJVM()          { return gJVM; }
        jobject        GetActivity()     { return gActivity; }
        AAssetManager* GetAssetManager() { return gAssetManager; }
        String         GetDataPath()     { return gDataPath; }
        Vec2I          GetResolution()   { return gResolution; }
    }

    JavaVM* ApplicationBase::GetJVM() const
    {
        return mJVM;
    }

    jobject ApplicationBase::GetActivity() const
    {
        return mActivity;
    }

    AAssetManager* ApplicationBase::GetAssetManager() const
    {
        return mAssetManager;
    }

    String ApplicationBase::GetDataPath() const
    {
        return mDataPath;
    }

    void Application::Initialize()
    {
        BasicInitialize();
    }

    void Application::InitializePlatform()
    {
        // State that was injected from the Java side through AndroidPlatform::Set*
        // before the call to Application::Initialize().
        mJVM          = AndroidPlatform::GetJVM();
        mActivity     = AndroidPlatform::GetActivity();
        mAssetManager = AndroidPlatform::GetAssetManager();
        mDataPath     = AndroidPlatform::GetDataPath();
        mResolution   = AndroidPlatform::GetResolution();
    }

    void Application::Shutdown()
    {}

    void Application::SetFullscreen(bool fullscreen /*= true*/)
    {}

    void Application::CheckCursorInfiniteMode()
    {}

    void Application::Launch()
    {
        mLog->Out("Application launched!");

        OnStarted();
        onStarted.Invoke();
        o2Events.OnApplicationStarted();
    }

    void Application::Update()
    {
        ProcessFrame();
    }

    bool Application::IsFullScreen() const
    {
        return true;
    }

    void Application::Maximize()
    {}

    bool Application::IsMaximized() const
    {
        return true;
    }

    void Application::SetResizible(bool resizible)
    {}

    bool Application::IsResizible() const
    {
        return false;
    }

    void Application::SetWindowSizePlatform(const Vec2I& size)
    {}

    Vec2I Application::GetWindowSize() const
    {
        return mResolution;
    }

    void Application::SetWindowPosition(const Vec2I& position)
    {}

    Vec2I Application::GetWindowPosition() const
    {
        return Vec2I();
    }

    void Application::SetWindowCaption(const String& caption)
    {}

    String Application::GetWindowCaption() const
    {
        return "";
    }

    void Application::SetContentSize(const Vec2I& size)
    {
        mResolution = size;
    }

    Vec2I Application::GetContentSize() const
    {
        return mResolution;
    }

    Vec2I Application::GetScreenResolution() const
    {
        return mResolution;
    }

    void Application::SetCursor(CursorType type)
    {}

    void Application::SetCursorPosition(const Vec2F& position)
    {}

    String Application::GetBinPath() const
    {
        return "";
    }
}

#endif // PLATFORM_ANDROID
