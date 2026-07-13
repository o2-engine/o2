#include "o2/stdafx.h"
#include "ConsoleLogStream.h"

#include <iostream>

#ifdef PLATFORM_WINDOWS
#include <io.h>
#elif defined PLATFORM_ANDROID
#include <android/log.h>
#endif

namespace o2
{
    ConsoleLogStream::ConsoleLogStream():
        LogStream()
    {
        InitConsole();
    }

    ConsoleLogStream::ConsoleLogStream(const WString& id):
        LogStream(id)
    {
        InitConsole();
    }

    ConsoleLogStream::~ConsoleLogStream()
    {
        //FreeConsole();
    }

    void ConsoleLogStream::OutStrEx(const WString& str)
    {
#if defined PLATFORM_WINDOWS
        puts(((String)str).Data());
#elif defined PLATFORM_ANDROID
        __android_log_print(ANDROID_LOG_INFO, "o2: ", "%s", ((String)str).Data());
#elif defined PLATFORM_MAC || defined PLATFORM_IOS || defined PLATFORM_LINUX || defined PLATFORM_WASM
        std::cout << ((String)str).Data() << std::endl;
#endif
    }

    void ConsoleLogStream::InitConsole()
    {
        /*if (AllocConsole())
        {
            int hCrt = _open_osfhandle((long)GetStdHandle(STD_OUTPUT_HANDLE), _O_TEXT);
            *stdout = *(::_fdopen(hCrt, "w"));
            ::setvbuf(stdout, NULL, _IONBF, 0);
            *stderr = *(::_fdopen(hCrt, "w"));
            ::setvbuf(stderr, NULL, _IONBF, 0);
        }*/
    }
}
