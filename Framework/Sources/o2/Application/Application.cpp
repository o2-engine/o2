#include "o2/stdafx.h"
#include "o2/Application/Application.h"

namespace o2
{
    DECLARE_SINGLETON(Application);

    Application::Application(RefCounter* refCounter):
        Singleton<Application>(refCounter)
    {}

    Application::~Application()
    {}

    void Application::OnResized(const Vec2I& size)
    {
        mWindowedSize = size;

		Integration::OnResized(size);

        if (mReady)
        {
            onResizing.Invoke();
            OnResizing();
        }
    }

    void Application::ProcessFrame()
    {
        if (!mReady)
			return;

		if (mCursorInfiniteModeEnabled)
			CheckCursorInfiniteMode();

		Integration::ProcessFrame();
    }

    void Application::SetCursorInfiniteMode(bool enabled)
    {
        mCursorInfiniteModeEnabled = enabled;
    }

    bool Application::IsCursorInfiniteModeOn() const
    {
        return mCursorInfiniteModeEnabled;
    }
    
    float Application::GetGraphicsScale() const
    {
        return mGraphicsScale;
    }

    void Application::SetWindowSize(const Vec2I& size)
    {
        SetWindowSizePlatform(size);
        OnResized(size);
	}
}
// --- META ---

DECLARE_CLASS(o2::Application, o2__Application);
// --- END META ---
