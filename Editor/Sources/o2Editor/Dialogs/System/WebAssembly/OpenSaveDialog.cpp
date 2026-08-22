#include "o2Editor/stdafx.h"
#include "o2Editor/Dialogs/System/OpenSaveDialog.h"

#ifdef PLATFORM_WASM

#include "o2/EngineSettings.h"

#include <emscripten.h>
#include <cstdlib>

// The engine expects a synchronous dialog, but a modal over the frozen main
// thread can't be interactive. So the picker UI lives in a popup window (its
// own event loop keeps it live) served by the backend at /picker, and the main
// thread blocks on a single synchronous long-poll XHR until the popup posts
// the choice to the server. Falls back to window.prompt when popups are blocked
// or no server endpoint is configured.
EM_JS(char*, o2_web_asset_dialog, (const char* mode, const char* title, const char* exts), {
    var modeStr = UTF8ToString(mode);
    var titleStr = UTF8ToString(title);
    var extsStr = UTF8ToString(exts);

    var promptFallback = function() {
        var res = window.prompt(titleStr + ' — path inside Assets/', '');
        return stringToNewUTF8(res || '');
    };

    if (typeof window === 'undefined' || !window.o2fsEndpoint)
        return promptFallback();

    var id = Math.random().toString(36).slice(2) + Date.now().toString(36);
    // The page may be served under a path prefix; the endpoint carries it
    var base = window.o2fsEndpoint.replace(/\/api$/, '');
    var url = base + '/picker?id=' + id + '&mode=' + modeStr +
              '&title=' + encodeURIComponent(titleStr) +
              '&exts=' + encodeURIComponent(extsStr);

    // noopener is essential: it puts the popup into its own renderer process,
    // so it stays interactive while this thread blocks in the sync XHR below.
    // (A same-process popup would share the frozen main thread — deadlock.)
    // noopener always returns null, so a blocked popup can't be detected here;
    // the server resolves the wait as cancel when the picker never claims it.
    window.open(url, '_blank', 'noopener,width=780,height=580');

    try {
        var xhr = new XMLHttpRequest();
        xhr.open('GET', window.o2fsEndpoint + '/dialog/wait?id=' + id, false);
        xhr.send();
        if (xhr.status < 200 || xhr.status >= 300)
            return stringToNewUTF8('');
        var res = JSON.parse(xhr.responseText);
        return stringToNewUTF8(res && res.path ? res.path : '');
    } catch (e) {
        console.error('[o2picker]', e);
        return stringToNewUTF8('');
    }
});

namespace Editor
{
    // Values in the extensions map are "*.ext" patterns; the picker wants a
    // bare comma-separated extension list
    static String ExtensionsParam(const Map<String, String>& extensions)
    {
        String exts;
        for (auto& kv : extensions)
        {
            String pattern = kv.second;
            int dotIdx = pattern.FindLast(".");
            String ext = dotIdx >= 0 ? pattern.SubStr(dotIdx + 1) : pattern;
            if (ext.IsEmpty() || ext == "*")
                continue;

            if (!exts.IsEmpty())
                exts += ",";
            exts += ext;
        }
        return exts;
    }

    static String ShowDialog(const char* mode, const String& title, const Map<String, String>& extensions)
    {
        char* raw = o2_web_asset_dialog(mode, title.Data(), ExtensionsParam(extensions).Data());
        String picked(raw);
        free(raw);

        if (picked.IsEmpty())
            return "";

        // The picker returns an Assets-relative path; callers convert back via
        // GetPathRelativeToPath(result, GetAssetsPath()), so anchor it there
        return String(GetAssetsPath()) + picked;
    }

    String GetOpenFileNameDialog(const String& title, const Map<String, String>& extensions, const String& defaultPath /*= ""*/)
    {
        return ShowDialog("open", title, extensions);
    }

    String GetSaveFileNameDialog(const String& title, const Map<String, String>& extensions, const String& defaultPath /*= ""*/)
    {
        return ShowDialog("save", title, extensions);
    }
}

#endif
