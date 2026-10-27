#pragma once

#include "o2/Utils/Math/Color.h"
#include "o2/Utils/Types/String.h"
#include "o2Editor/Pipeline/PipelineNodeType.h"

using namespace o2;

namespace Editor
{
    // Look of the Pipeline window: dark is the AssetsLine web editor, light the o2 editor's own colours
    enum class PipelineThemeKind { Dark, Light };

    // ---------------------------------------------------------------------------------
    // Colours of one theme. Names follow the AssetsLine CSS tokens, so the three editors
    // (web, o2, Unity) read the same table; geometry never depends on the theme
    // ---------------------------------------------------------------------------------
    struct PipelinePalette
    {
        bool dark = true;

        Color4 canvasInner;     // Canvas gradient centre
        Color4 canvasMid;       // Canvas gradient middle stop
        Color4 canvasOuter;     // Canvas gradient outer stop and flat canvas
        Color4 gridDot;         // Dot at every grid point
        Color4 gridDotMajor;    // Brighter dot at every tenth grid point

        Color4 card;            // --surface: card body
        Color4 cardSolid;       // --surface-solid: menus, popups, marker rims
        Color4 cardStrokeTop;   // Card outline at the top edge
        Color4 cardStrokeMid;   // Card outline at 45% of the height
        Color4 cardStrokeBottom;// Card outline at the bottom edge
        Color4 cardHighlight;   // Inner highlight line under the top edge
        Color4 headerBand;      // Header band fill, transparent in the dark theme
        Color4 headerLine;      // Line under the header band
        Color4 cardShadow;      // Shadow colour with its strength in alpha
        float  shadowOffset;    // Shadow vertical offset, px
        float  shadowBlur;      // Shadow blur radius, px
        Color4 pictureShadow;   // Shadow of the picture cards inside a node
        float  pictureShadowOffset;
        float  pictureShadowBlur;

        Color4 bg;              // --bg: window chrome
        Color4 bg2;             // --bg-2: raised chrome
        Color4 bg3;             // --bg-3: pressed, an on toggle
        Color4 fg;              // --fg: text and titles
        Color4 fgDim;           // --fg-dim: captions, hints, icons
        Color4 fg3;             // --fg-3: quietest text, placeholders, idle marks
        Color4 border;          // --border: control borders
        Color4 line;            // --line: separators, field border at rest
        Color4 field;           // --field: field and button fill
        Color4 active;          // --active: hover / pressed wash
        Color4 surface;         // --surface: floating panels and menus

        Color4 accent;          // --accent
        Color4 accentInk;       // --accent-ink: glyphs on the accent
        Color4 accentHover;     // --accent-hover
        Color4 accentSoft;      // --accent-soft: selected row wash, focus ring

        Color4 portText;        // --type-text
        Color4 portImage;       // --accent-2
        Color4 portVideo;       // --accent-3
        Color4 portAudio;       // --accent-4

        Color4 green;           // Fresh result
        Color4 yellow;          // Stale result, retry
        Color4 red;             // Errors, stop

        Color4 checkerA;        // Transparency checker, first square
        Color4 checkerB;        // Transparency checker, second square

        // Returns the colour of a port type
        Color4 PortColor(PipelinePortType type) const;
    };

    // ---------------------------------------------------------------------------------
    // Current theme of the Pipeline window: a per-user view setting kept in the pipeline
    // work folder's ViewPrefs.json, never in a .pipeline file
    // ---------------------------------------------------------------------------------
    namespace PipelineTheme
    {
        // Returns the palette of the current theme
        const PipelinePalette& Get();

        // Returns the palette of a theme
        const PipelinePalette& Of(PipelineThemeKind kind);

        // Returns the current theme
        PipelineThemeKind GetKind();

        // Switches the theme, stores it and tells the open editors to restyle
        void SetKind(PipelineThemeKind kind);

        // Returns a number that changes whenever the theme changes
        int GetVersion();

        constexpr int fontResolution = 36; // Resolution of the pipeline fonts: a text height unit is half a pixel

        // Returns the UI font asset path of a weight: 400, 500 or 600
        String Font(int weight = 400);

        // Returns the text height of a size in pixels for the pipeline fonts
        int Size(float pixels);

        // Returns the monospace font asset path, for numbers and metadata
        String MonoFont();

        // Returns the colour with its alpha multiplied
        Color4 Alpha(const Color4& color, float alpha);
    }
}
// --- META ---

PRE_ENUM_META(Editor::PipelineThemeKind);
// --- END META ---
