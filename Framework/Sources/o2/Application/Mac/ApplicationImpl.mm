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

    void ApplicationPlatformWrapper::OnWindowResized(const Vec2I& resolution)
    {
        ApplicationPlatformWrapper::resolution = resolution;
        
        if (Application::IsSingletonInitialzed())
            o2Application.OnResized(resolution);
    }
    
    void ApplicationPlatformWrapper::Deinitialize()
    {
        if (Application::IsSingletonInitialzed())
            o2Application.Deinitialize();
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
        ApplicationPlatformWrapper::view.device = MTLCreateSystemDefaultDevice();
        
        if(!ApplicationPlatformWrapper::view.device)
        {
            o2Debug.LogError("Metal is not supported on this device");
            return;
        }

        ApplicationPlatformWrapper::renderer = [[RendererView alloc] initWithMetalKitView:ApplicationPlatformWrapper::view];
        [ApplicationPlatformWrapper::renderer mtkView:ApplicationPlatformWrapper::view drawableSizeWillChange:ApplicationPlatformWrapper::view.drawableSize];
        ApplicationPlatformWrapper::view.delegate = ApplicationPlatformWrapper::renderer;
        
        [ApplicationPlatformWrapper::window setContentView:ApplicationPlatformWrapper::view];
        [ApplicationPlatformWrapper::window setInitialFirstResponder:ApplicationPlatformWrapper::view];
        
        mGraphicsScale = [[ApplicationPlatformWrapper::view layer] contentsScale];
        
        [ApplicationPlatformWrapper::window makeFirstResponder:ApplicationPlatformWrapper::view];
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
        
        [ApplicationPlatformWrapper::view initializeMouseTracking];
        
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
    {
        [ApplicationPlatformWrapper::window setResizable:resizible];
    }

    bool Application::IsResizible() const
    {
        return [ApplicationPlatformWrapper::window isResizable];
    }

    void Application::SetWindowSizePlatform(const Vec2I& size)
    {
        [ApplicationPlatformWrapper::window setContentSize:NSMakeSize(size.x, size.y)];
    }

    Vec2I Application::GetWindowSize() const
    {
        NSRect contentRect = [ApplicationPlatformWrapper::window contentRectForFrameRect:[ApplicationPlatformWrapper::window frame]];
        return Vec2I(contentRect.size.width, contentRect.size.height);
    }

    void Application::SetWindowPosition(const Vec2I& position)
    {
        [ApplicationPlatformWrapper::window setFrameOrigin:NSMakePoint(position.x, position.y)];
        [ApplicationPlatformWrapper::window makeKeyAndOrderFront:nil];
    }

    Vec2I Application::GetWindowPosition() const
    {
        NSRect frame = [ApplicationPlatformWrapper::window frame];
        return Vec2I(frame.origin.x, frame.origin.y);
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
        NSScreen* mainScreen = [NSScreen mainScreen];
        CGSize screenSize = [mainScreen frame].size;
        return Vec2I(screenSize.width, screenSize.height);
    }

    void Application::SetCursor(CursorType type)
    {
        NSCursor* cursor = nil;
        
        switch (type)
        {
            case CursorType::AppStarting:
                cursor = [NSCursor disappearingItemCursor];
                break;
                
            case CursorType::Arrow:
                cursor = [NSCursor arrowCursor];
                break;
                
            case CursorType::Cross:
                cursor = [NSCursor crosshairCursor];
                break;
                
            case CursorType::Hand:
                cursor = [NSCursor pointingHandCursor];
                break;
                
            case CursorType::Help:
                cursor = [NSCursor contextualMenuCursor];
                break;
                
            case CursorType::IBeam:
                cursor = [NSCursor IBeamCursor];
                break;
                
            case CursorType::Icon:
                cursor = [NSCursor arrowCursor];
                break;
                
            case CursorType::No:
                cursor = [NSCursor operationNotAllowedCursor];
                break;
                
            case CursorType::SizeAll:
                cursor = [NSCursor crosshairCursor];
                break;
                
            case CursorType::SizeNeSw:
                cursor = [[NSCursor class] performSelector:@selector(_windowResizeNorthEastSouthWestCursor)];
                break;
                
            case CursorType::SizeNS:
                cursor = [[NSCursor class] performSelector:@selector(_windowResizeNorthSouthCursor)];
                break;
                
            case CursorType::SizeNwSe:
                cursor = [[NSCursor class] performSelector:@selector(_windowResizeNorthWestSouthEastCursor)];
                break;
                
            case CursorType::SizeWE:
                cursor = [[NSCursor class] performSelector:@selector(_windowResizeEastWestCursor)];
                break;
                
            case CursorType::UpArrow:
                cursor = [NSCursor arrowCursor];
                break;
                
            case CursorType::Wait:
                cursor = [NSCursor disappearingItemCursor];
                break;
                
            default:
                cursor = [NSCursor arrowCursor];
                break;
        }
        
        if (cursor)
        {
            [cursor set];
            [[ApplicationPlatformWrapper::view window] invalidateCursorRectsForView:ApplicationPlatformWrapper::view];
            [ApplicationPlatformWrapper::view addCursorRect:[ApplicationPlatformWrapper::view bounds] cursor:cursor];
        }
    }

    void Application::SetCursorPosition(const Vec2F& position)
    {
        CGPoint point = CGPointMake(position.x, position.y);
        CGWarpMouseCursorPosition(point);
    }

    String Application::GetBinPath() const
    {
        return "";
    }
}

#endif // PLATFORM_MAC
