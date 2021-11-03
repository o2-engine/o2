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

	void ApplicationPlatformWrapper::OnWindowRsized(const Vec2I& resolution)
	{
		ApplicationPlatformWrapper::resolution = resolution;
		
		if (Render::IsSingletonInitialzed())
			o2Render.OnFrameResized();
	}

	void Application::Run(int argc, char * argv[])
	{
		InitalizeSystems();
		UIApplicationMain(argc, argv, nil, NSStringFromClass([AppDelegate class]));
	}
	
	void Application::InitializePlatform()
	{		
		mRender = mnew Render();
		
		o2Debug.InitializeFont();
		o2UI.TryLoadStyle();
		
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
	{
//		[ApplicationPlatformWrapper::window setIsZoomed:true];
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
	}

	String Application::GetWindowCaption() const
	{
		return "";
	}

	void Application::SetContentSize(const Vec2I& size)
	{
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

#endif // PLATFORM_IOS
