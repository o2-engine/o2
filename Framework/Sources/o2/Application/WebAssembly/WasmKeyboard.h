#pragma once

#include "o2/Application/VKCodes.h"
#include "o2/Utils/Types/CommonTypes.h"

namespace o2
{
    // Maps a DOM KeyboardEvent.code ("KeyW", "ArrowLeft", "Space", layout independent) to an
    // engine keyboard key, 0 when the engine has no key for it. Values come from VKCodes.h,
    // so the browser build speaks the key codes of the platform it is compiled for
    KeyboardKey DomKeyCodeToKeyboardKey(const char* code);
}
