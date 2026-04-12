#include "o2/stdafx.h"

#ifdef PLATFORM_IOS
#import <UIKit/UIKit.h>
#import "o2/Application/iOS/RendererView.h"
#import "o2/Application/iOS/AppDelegate.h"
#include "o2/Application/Application.h"
#include "o2/Application/iOS/ApplicationPlatformWrapper.h"
#include "o2/Events/EventSystem.h"
#include "o2/Utils/Debug/Log/LogStream.h"
#include "o2/Utils/FileSystem/FileSystem.h"
#include "o2/Scene/UI/UIManager.h"

namespace o2
{
    RenderView* ApplicationPlatformWrapper::view;
    RenderViewController* ApplicationPlatformWrapper::viewController;
    RendererViewDelegate* ApplicationPlatformWrapper::renderer;
    Vec2I ApplicationPlatformWrapper::resolution;

    void ApplicationPlatformWrapper::OnWindowResized(const Vec2I& resolution)
    {
        ApplicationPlatformWrapper::resolution = resolution;

        if (Application::IsSingletonInitialzed())
            o2Application.OnResized(resolution);
    }

    void ApplicationPlatformWrapper::InitializePlatform()
    {
        o2Application.Application::InitializePlatform();
    }

    void ApplicationPlatformWrapper::LaunchApplication()
    {
        o2Application.Launch();
    }

    void ApplicationPlatformWrapper::CallInitializePlatform()
    {
        if (Application::IsSingletonInitialzed())
            o2Application.InitializePlatform();
    }

    void ApplicationPlatformWrapper::CallUpdate()
    {
        if (Application::IsSingletonInitialzed())
            o2Application.Update();
    }

    void Application::Run(int argc, char** argv)
    {
        // Set working directory to the app bundle's resource path so relative asset paths work
        NSString* resourcePath = [[NSBundle mainBundle] resourcePath];
        chdir([resourcePath UTF8String]);

        InitalizeSystems();
        UIApplicationMain(argc, argv, nil, NSStringFromClass([AppDelegate class]));
    }

    void Application::InitializePlatform()
    {
        mGraphicsScale = [[ApplicationPlatformWrapper::view layer] contentsScale];

        InitiazeRender();
        InitilizeUIStyles();

        mReady = true;
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
        return false;
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
        return ApplicationPlatformWrapper::resolution;
    }

    Vec2I Application::GetScreenResolution() const
    {
        CGRect bounds = [[UIScreen mainScreen] nativeBounds];
        return Vec2I((int)bounds.size.width, (int)bounds.size.height);
    }

    void Application::SetCursor(CursorType type)
    {}

    void Application::SetCursorPosition(const Vec2F& position)
    {}

    String Application::GetBinPath() const
    {
        NSString* resourcePath = [[NSBundle mainBundle] resourcePath];
        return String([resourcePath UTF8String]) + "/";
    }
}

#endif // PLATFORM_IOS
