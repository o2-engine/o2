#include "o2/stdafx.h"
#include "AppTestDriver.h"

#include "o2/Application/Application.h"
#include "o2/Application/Input.h"
#include "o2/Render/Render.h"
#include "o2/Utils/Bitmap/Bitmap.h"
#include "o2/Utils/FileSystem/FileSystem.h"
#include "o2/Utils/Math/Math.h"
#include "o2/Utils/System/Time/Time.h"

namespace o2
{
    void AppTestDriver::PumpFrames(int count /*= 1*/)
    {
        for (int i = 0; i < count; i++)
            o2Application.ProcessFrame();
    }

    void AppTestDriver::Wait(float seconds)
    {
        float time = 0.0f;
        while (time < seconds)
        {
            PumpFrames(1);

            // o2Time keeps the real delta, the update loop applies it clamped: on a slow machine the
            // wall clock runs ahead of the simulation the test is waiting for
            time += Math::Clamp(o2Time.GetDeltaTime(), Integration::minFrameDeltaTime,
                                Integration::maxFrameDeltaTime);
        }
    }

    void AppTestDriver::MoveCursor(const Vec2F& screenPos, int steps /*= 8*/)
    {
        Vec2F start = o2Input.GetCursorPos();
        int count = Math::Max(steps, 1);
        for (int i = 1; i <= count; i++)
        {
            o2Input.OnCursorMoved(Math::Lerp(start, screenPos, (float)i / (float)count), 0);
            PumpFrames(1);
        }
    }

    void AppTestDriver::PressCursor(const Vec2F& screenPos)
    {
        o2Input.OnCursorMoved(screenPos, 0, false);
        o2Input.OnCursorPressed(screenPos);
        PumpFrames(1);
    }

    void AppTestDriver::ReleaseCursor()
    {
        o2Input.OnCursorReleased();
        PumpFrames(1);
    }

    void AppTestDriver::Click(const Vec2F& screenPos)
    {
        PressCursor(screenPos);
        ReleaseCursor();
    }

    void AppTestDriver::Drag(const Vec2F& from, const Vec2F& to, int steps /*= 12*/)
    {
        PressCursor(from);
        MoveCursor(to, steps);
        ReleaseCursor();
    }

    Ref<Bitmap> AppTestDriver::TakeScreenshot()
    {
        Ref<Bitmap> result;
        o2Render.CaptureNextFrame([&result](const Ref<Bitmap>& bitmap) { result = bitmap; });
        PumpFrames(1);
        return result;
    }

    bool AppTestDriver::SaveScreenshot(const String& pngPath)
    {
        Ref<Bitmap> bitmap = TakeScreenshot();
        if (!bitmap)
            return false;

        String folder = o2FileSystem.ExtractPathStr(pngPath);
        if (!folder.IsEmpty())
            o2FileSystem.FolderCreate(folder, true);

        return bitmap->Save(pngPath, Bitmap::ImageType::Png);
    }
}
