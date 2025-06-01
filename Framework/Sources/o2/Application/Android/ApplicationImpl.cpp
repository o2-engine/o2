#include "o2/stdafx.h"

#ifdef PLATFORM_ANDROID

#include "Application/Application.h"
#include "Events/EventSystem.h"
#include "Utils/Debug/Log/LogStream.h"
#include "Utils/FileSystem/FileSystem.h"

namespace o2
{
    JavaVM* ApplicationBase::GetJVM() const
    {
        return mJVM;
    }

    const jobject* ApplicationBase::GetActivity() const
    {
        return &mActivity;
    }

    AAssetManager* ApplicationBase::GetAssetManager() const
    {
        return mAssetManager;
    }

    String ApplicationBase::GetDataPath() const
    {
        return mDataPath;
    }

    void Application::InitializePlatform()
    {}

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

    void Application::Initialize(JNIEnv* env, jobject activity, AAssetManager* assetManager, String dataPath,
                             const Vec2I& resolution)
    {
        mResolution = resolution;

        env->GetJavaVM(&mJVM);
        mActivity = activity;
        mAssetManager = assetManager;
        mDataPath = dataPath;

        BasicInitialize();
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
        return Vec2I();
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
    {}

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