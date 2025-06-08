#pragma once
#include "o2/Utils/Types/Containers/Map.h"
#include "o2/Utils/Types/String.h"

using namespace o2;

namespace Editor
{
    // Show open file dialog and returns name
    String GetOpenFileNameDialog(const String& title, const Map<String, String>& extensions, const String& defaultPath = "");

    // Show save file dialog and returns name
    String GetSaveFileNameDialog(const String& title, const Map<String, String>& extensions, const String& defaultPath = "");
}
