#include "o2Editor/stdafx.h"
#include "o2Editor/Dialogs/System/OpenSaveDialog.h"

#ifdef PLATFORM_MAC

namespace Editor
{
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
