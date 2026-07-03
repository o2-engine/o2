#pragma once

#include "o2/Utils/Math/Vector2.h"
#include "o2/Utils/Types/Ref.h"
#include "o2/Utils/Types/String.h"

namespace o2
{
    class Bitmap;

    // ---------------------------------------------------------------------------------
    // Drives a running (non-headless) application frame-by-frame for automated tests:
    // pumps frames, injects cursor input and captures screenshots through the render.
    // Every action advances whole frames, so the scene, event system and render see the
    // injected input exactly like a real user's. Cursor positions are in screen space
    // (origin at the window centre, y up).
    // ----------------------------------------------------------------------------------
    class AppTestDriver
    {
    public:
        // Runs the given number of full application frames (update + draw)
        static void PumpFrames(int count = 1);

        // Pumps frames until the given amount of game time passes
        static void Wait(float seconds);

        // Smoothly moves the cursor to the point over several frames
        static void MoveCursor(const Vec2F& screenPos, int steps = 8);

        // Presses the primary cursor at the point and advances one frame
        static void PressCursor(const Vec2F& screenPos);

        // Releases the primary cursor and advances one frame
        static void ReleaseCursor();

        // Press and release at the point
        static void Click(const Vec2F& screenPos);

        // Press at `from`, drag to `to` over several frames, release
        static void Drag(const Vec2F& from, const Vec2F& to, int steps = 12);

        // Captures the next rendered frame and returns its pixels
        static Ref<Bitmap> TakeScreenshot();

        // Captures the next rendered frame and saves it as PNG, creating folders as needed
        static bool SaveScreenshot(const String& pngPath);
    };
}
