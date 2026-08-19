#pragma once

#ifdef PLATFORM_WASM

#include "o2/Utils/Types/String.h"

namespace o2
{
    // Bridge between the in-browser MEMFS and a server-side working copy of the
    // project (the web editor case: the page streams the project tree into
    // /project at startup and mirrors every mutation back to the server, so the
    // server's git working copy always matches what the editor sees).
    namespace WebFS
    {
        // True when the hosting page defined window.o2fsEndpoint. The plain game
        // page doesn't, making every call below a no-op there.
        bool IsEnabled();

        // Mirror MEMFS mutations to the server (synchronous XHR: when a call
        // returns, the server tree already matches MEMFS)
        void NotifyFileWritten(const String& path);
        void NotifyFileDeleted(const String& path);
        void NotifyFolderCreated(const String& path);
        void NotifyFolderRemoved(const String& path);
        void NotifyMoved(const String& from, const String& to);
        void NotifyFileCopied(const String& from, const String& to);
        void NotifyFolderCopied(const String& from, const String& to);
        void NotifyEditDateSet(const String& path, long long unixTimeSeconds);
    }
}

#endif // PLATFORM_WASM
