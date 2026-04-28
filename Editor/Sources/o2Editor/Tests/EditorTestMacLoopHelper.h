#pragma once

#if defined(PLATFORM_WINDOWS) || defined(PLATFORM_MAC) || defined(PLATFORM_LINUX)

namespace Editor::Tests
{
    // EditorApplication::UpdateTaskManager only runs while playing, so we cannot
    // hook test progression through TaskManager. Instead, install a platform-level
    // 60 fps timer that invokes the supplied callback every tick.
    //
    // The callback is called with dt in seconds.
    using TickCallback = void (*)(float);

    // macOS: schedules an NSTimer on the main run loop.
    // Windows/Linux: stub (will need a Win32 timer / glib equivalent later).
    void StartTestTickTimer(TickCallback cb);
}

#endif
