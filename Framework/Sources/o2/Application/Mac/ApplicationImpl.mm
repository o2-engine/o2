#include "o2/stdafx.h"

#ifdef PLATFORM_MAC
#import <Cocoa/Cocoa.h>
#import "o2/Application/Mac/RendererView.h"
#import "o2/Application/Mac/AppDelegate.h"
#include "o2/Application/Application.h"
#include "o2/Application/Mac/ApplicationPlatformWrapper.h"
#include "o2/Events/EventSystem.h"
#include "o2/Utils/Debug/Log/LogStream.h"
#include "o2/Utils/FileSystem/FileSystem.h"

namespace o2
{
    NSWindow* ApplicationPlatformWrapper::window;
    MTKView* ApplicationPlatformWrapper::view;
    RendererView* ApplicationPlatformWrapper::renderer;
    Vec2I ApplicationPlatformWrapper::resolution;

    void ApplicationPlatformWrapper::OnWindowRsized(const Vec2I& resolution)
    {
        ApplicationPlatformWrapper::resolution = resolution;
        
        if (Render::IsSingletonInitialzed())
            o2Render.OnFrameResized();
    }

    void Application::Initialize()
    {
        BasicInitialize();
    }
    
    void Application::InitializePlatform()
    {
        [NSApplication sharedApplication];
        [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
        [NSApp activateIgnoringOtherApps:YES];        
        [NSApp setDelegate:[[AppDelegate alloc] init]];
        
        ApplicationPlatformWrapper::window = [[NSWindow alloc] initWithContentRect:NSMakeRect(0, 0, 960, 640)
                                                                         styleMask:NSWindowStyleMaskClosable|NSWindowStyleMaskTitled|NSWindowStyleMaskResizable
                                                                           backing:NSBackingStoreBuffered defer:NO];
        
        [ApplicationPlatformWrapper::window setTitle: [[NSProcessInfo processInfo] processName]];
        [ApplicationPlatformWrapper::window makeKeyAndOrderFront:nil];
        
        ApplicationPlatformWrapper::view = [[ViewController alloc] init];
        ApplicationPlatformWrapper::view.colorPixelFormat = MTLPixelFormatRGBA8Unorm;
        [ApplicationPlatformWrapper::window setContentView:ApplicationPlatformWrapper::view];
        [ApplicationPlatformWrapper::window setInitialFirstResponder:ApplicationPlatformWrapper::view];
        [ApplicationPlatformWrapper::window makeFirstResponder:ApplicationPlatformWrapper::view];
        
        ApplicationPlatformWrapper::view.device = MTLCreateSystemDefaultDevice();
        
        if(!ApplicationPlatformWrapper::view.device)
        {
            o2Debug.LogError("Metal is not supported on this device");
            return;
        }
        
        ApplicationPlatformWrapper::renderer = [[RendererView alloc] initWithMetalKitView:ApplicationPlatformWrapper::view];
        [ApplicationPlatformWrapper::renderer mtkView:ApplicationPlatformWrapper::view drawableSizeWillChange:ApplicationPlatformWrapper::view.drawableSize];
        ApplicationPlatformWrapper::view.delegate = ApplicationPlatformWrapper::renderer;
        
        mGraphicsScale = [[ApplicationPlatformWrapper::view layer] contentsScale];
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
        
        [NSApp run];
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
    {
        [ApplicationPlatformWrapper::window setIsZoomed:true];
    }

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

    void Application::SetWindowSize(const Vec2I& size)
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
    {
        [ApplicationPlatformWrapper::window setTitle:[NSString stringWithUTF8String:caption.Data()]];
    }

    String Application::GetWindowCaption() const
    {
        return [[ApplicationPlatformWrapper::window title] UTF8String];
    }

    void Application::SetContentSize(const Vec2I& size)
    {
        [ApplicationPlatformWrapper::window setContentSize:NSMakeSize(size.x, size.y)];
    }

    Vec2I Application::GetContentSize() const
    {
        return ApplicationPlatformWrapper::resolution;
    }

    Vec2I Application::GetScreenResolution() const
    {
        return Vec2I(1000, 1000);
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

#endif // PLATFORM_MAC
