#pragma once

#include "o2/Utils/Math/Border.h"
#include "o2/Utils/Math/Vector2.h"
#include "o2/Utils/Types/Containers/Vector.h"
#include "o2/Utils/Types/String.h"

using namespace o2;

namespace Editor
{
    // -------------------------------------------------------------------------------------------------------------------
    // A device the Game window emulates: the same list as the o2 portal's players and the web editor's Play face
    // (o2portal frontend/src/ui/devices.ts, o2-Template shell/preview-host.js - change them together). Sizes are in the
    // device's points (CSS pixels) in portrait: `size` is what a page gets, `pixels` the real screen, `pad` the bezels
    // [sides, top and bottom], `radius` the screen's corners, `safeTop`/`safeBottom` the safe area of a full-screen game
    // -------------------------------------------------------------------------------------------------------------------
    struct GameDevice
    {
        String id;
        String name;
        String group;    // Phones, Tablets, Desktop
        String kind;     // phone, tablet, desktop
        String look;     // island, punch, home, tablet, window, console
        Vec2F  size;     // points, portrait
        Vec2I  pixels;   // the real screen, portrait
        float  inch = 0;
        Vec2F  pad;      // bezels: sides, top and bottom
        float  radius = 0;
        float  safeTop = 0;
        float  safeBottom = 0;

        // Returns true for a desktop window or a console: no portrait
        bool IsFixedLandscape() const;

        // Returns the screen's size in points as the device stands
        Vec2F GetScreenSize(bool landscape) const;

        // Returns the screen's size in pixels as the device stands
        Vec2I GetScreenPixels(bool landscape) const;

        // Returns the caption of the devices menu: the name, the real screen and its diagonal
        String GetCaption() const;

        // Returns the safe area of a game on the whole screen, in points (left, bottom, right, top). A phone on its side
        // has turned counterclockwise: the island or the camera is at the left
        BorderF GetSafeArea(bool landscape) const;

        // Returns the body drawn around the screen as an SVG in points, the screen's rectangle in it (from the top
        // left) and the body's size
        String GetBodySvg(bool landscape, Vec2F& bodySize, Vec2F& screenOffset) const;
    };

    // Returns the emulated devices in the order of the menu
    const Vector<GameDevice>& GetGameDevices();

    // Returns the device by its id, null when there is none
    const GameDevice* FindGameDevice(const String& id);
}
