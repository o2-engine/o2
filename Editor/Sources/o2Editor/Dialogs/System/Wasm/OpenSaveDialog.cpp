#include "o2Editor/stdafx.h"
#include "o2Editor/Dialogs/System/OpenSaveDialog.h"

#ifdef PLATFORM_WASM

namespace Editor
{
    // The page has no native file dialogs to open: the browser only offers its own file picker,
    // which cannot be driven synchronously from wasm
    String GetOpenFileNameDialog(const String& title, const Map<String, String>& extensions, const String& defaultPath /*= ""*/)
    {
        return "";
    }

    String GetSaveFileNameDialog(const String& title, const Map<String, String>& extensions, const String& defaultPath /*= ""*/)
    {
        return "";
    }
}

#endif
