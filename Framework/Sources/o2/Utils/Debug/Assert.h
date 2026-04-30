#pragma once

namespace o2
{
// Outs assert with description, when true_condition is false.
// ErrorMessage handles the platform-specific reaction (modal box + debugbreak,
// or in headless mode just logs to stderr and continues).
#define Assert(true_condition, desc)                    \
    {                                                   \
        if (!(true_condition))                          \
            o2::ErrorMessage(desc, __FILE__, __LINE__); \
    }

    void ErrorMessage(const char* desc, const char* file, long line);
}
