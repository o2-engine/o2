#pragma once

#include <optional>

#include "o2Editor/Pipeline/UiDoc/UiDocSkin.h"

// ------------------------------------------------------------------------------------------------
// Layout + kit -> ui_structure.json (XuToWei/Image-To-UI's schema; port of AssetsLine shared/uiDoc/
// structure.ts), the input the existing exporters turn into a Unity prefab or an o2 prototype. Geometry is
// taken at the design size: every block gets its box relative to its parent, its anchor, and - unless a
// layout group places it - a `responsive` block that is exactly its anchor preset (Unity's anchorMin /
// anchorMax + offsets). A row / column / grid becomes a layout group when the schema can say it (no
// growing children, symmetric padding, no cross-axis stretch); otherwise its children get their solved boxes
// ------------------------------------------------------------------------------------------------
namespace Editor::UiDoc
{
    // ----------------------------------------------------------------------------------------------
    // A sprite the structure names (the web's UiExportSprite): <file> with its PNG bytes, size, nine-slice and,
    // when it is not at the layout's density, its pixels per layout pixel and its own DPI
    // ----------------------------------------------------------------------------------------------
    struct UiStructureSprite
    {
        String                file;      // <element name>.png, as the layout's `asset` paths name it
        String                png;       // PNG bytes (decoded from the kit's base64)
        double                width = 0, height = 0;
        std::optional<UiNine> nineSlice; // Sliced elements only
        std::optional<double> density;   // Sprite pixels per layout pixel; none = 1
        std::optional<double> dpi;       // The sprite's own DPI (with density)
    };

    // -----------------------------------------------------------------
    // ui_structure.json with the sprites and fonts it uses, and warnings
    // -----------------------------------------------------------------
    struct UiStructureResult
    {
        String                    json;     // ui_structure.json: JSON.stringify(structure, null, 2)
        Json                      structure; // The same as a value
        Vector<UiStructureSprite> sprites;  // In first-use order
        Vector<String>            fonts;    // Font files the texts name, first-use order
        Vector<String>            warnings;
    };

    // The layout (with the kit, or without: placeholders) as ui_structure.json
    UiStructureResult ToUiStructure(const UiDoc& doc, const UiKit* kit);
}
