#pragma once

#include "o2/Render/VectorGraphics/VectorImage.h"
#include "o2/Utils/Math/Color.h"
#include "o2/Utils/Types/String.h"

using namespace o2;

namespace Editor
{
    // ------------------------------------------------------------------------------------------------------------------
    // Colours of the dark theme: the AssetsLine tokens (frontend styles.css :root) put on the roles of the editor elements
    // ------------------------------------------------------------------------------------------------------------------
    namespace EditorThemeDark
    {
        inline Color4 Hex(UInt32 rgb, int a = 255) { return Color4((rgb >> 16) & 255, (rgb >> 8) & 255, rgb & 255, a); }

        inline const Color4 backdrop = Hex(0x0B0C10);         // Behind the docks
        inline const Color4 chromeTop = Hex(0x17181F);        // Menu bar and tools panel, a glass-like fall from the top
        inline const Color4 chromeBottom = Hex(0x121318);
        inline const Color4 tabStrip = Hex(0x111217);         // Strip behind a dock's tabs
        inline const Color4 tabInactive = Hex(0x16171E);      // Tab of a window in the background
        inline const Color4 panel = Hex(0x1B1D26);            // --bg-2: window bodies; the shown tab is the body's colour
        inline const Color4 popupTop = Hex(0x262935);         // --surface-solid with a lighter top: menus, lists, dialogs
        inline const Color4 popupBottom = Hex(0x1F2129);
        inline const Color4 raisedTop = Hex(0x2C2F3B);        // --bg-3 with a lighter top: buttons, component heads
        inline const Color4 raisedBottom = Hex(0x23262F);
        inline const Color4 raisedHoverTop = Hex(0x383C4A);   // A hovered raised thing is lighter
        inline const Color4 raisedHoverBottom = Hex(0x2D303C);
        inline const Color4 pressed = Hex(0x17181F);          // A pressed one sinks below the panel
        inline const Color4 field = Hex(0x1E2029);            // --field over the panel: edit boxes, check boxes
        inline const Color4 fieldHover = Hex(0x272A35);
        inline const Color4 fieldPressed = Hex(0x18191F);
        inline const Color4 edgeTop = Hex(0x484B57);          // Edge of raised things: white 22% at the top...
        inline const Color4 edgeBottom = Hex(0x2A2C35);       // ...to 6% at the bottom, the AssetsLine card stroke
        inline const Color4 edgeHoverTop = Hex(0x606472);   // Hovered edge, a step lighter
        inline const Color4 edgeHoverBottom = Hex(0x383B46);
        inline const Color4 border = Hex(0x30323B);           // --border: flat edge of fields, separators in menus
        inline const Color4 borderStrong = Hex(0x4C505D);     // Hovered edge of fields
        inline const Color4 line = Hex(0x25272F);             // --line: rules of the frames and the tabs
        inline const Color4 track = Hex(0x22242C);            // Scroll bar track
        inline const Color4 handle = Hex(0x4A4D5A);           // Scroll bar handle
        inline const Color4 handleHover = Hex(0x626674);
        inline const Color4 knob = Hex(0x3A3E4B);             // Pills and toggled squares standing on the panel
        inline const Color4 knobHover = Hex(0x4D5263);
        inline const Color4 disc = Hex(0x575C6C);             // Disc of a round button: clear on every header and panel
        inline const Color4 discHover = Hex(0x6E7486);
        inline const Color4 text = Hex(0xE3E5EB);             // Every text: one colour, as AssetsLine's body text
        inline const Color4 textStrong = Hex(0xF4F5F8);       // --fg
        inline const Color4 glyph = Hex(0xC3C8D2);            // Icons and marks: bright enough on every surface
        inline const Color4 glyphHover = Hex(0xFFFFFF);       // A hovered icon lights up
        inline const Color4 glyphDim = Hex(0x6C7280);         // --fg-3: an inactive icon
        inline const Color4 ink = Hex(0x9AA0AE);              // --fg-dim: secondary marks
        inline const Color4 accent = Hex(0xB79CFF);           // --accent: focus, check marks, a pressed icon
        inline const Color4 accentDeep = Hex(0x5B48A6);       // Deep violet fill: selection, pressed and switched on things
        inline const Color4 accentDeepHover = Hex(0x6A56BD);
        inline const Color4 accentDeepPressed = Hex(0x473889);
        inline const Color4 accentInk = Hex(0x1A1030);        // --accent-ink: glyph over the accent
        inline const Color4 red = Hex(0xFF7A7A), green = Hex(0x62D59A), blue = Hex(0x7FB2FF), yellow = Hex(0xF1BE5B);
        inline const Color4 viewBack = Hex(0x15161C);         // Scene view, state graph, curve editor
        inline const Color4 viewGrid = Hex(0x262833);

        // Restyles an image of the editor art loaded for the dark theme; the file path tells the role of the image
        void RestyleImage(VectorImage& image, const String& path);
    }
}
