#pragma once

#include <optional>

#include "o2Editor/Pipeline/UiDoc/UiDocDevice.h"
#include "o2Editor/Pipeline/UiDoc/UiTextStyle.h"

// ------------------------------------------------------------------------------------------------
// UI kit: a data type of its own (port type `uiKit`), port of AssetsLine shared/uiKit.ts. The UI kit node
// gathers the graphics of one interface style - buttons, panels, frames, icons, bars - each with its meta
// (element type, asset type, nine-slice borders, name, where it is used), plus the text styles connected
// to it. On the wire a kit is one JSON document with every picture inside it (base64 PNG)
// ------------------------------------------------------------------------------------------------
namespace Editor::UiDoc
{
    // What an element is for: background, panel, frame, button, icon, image, progressBar, progressFill, toggle,
    // slider, sliderHandle, input, tab, badge, divider, header, slot, other
    const Vector<String>& UiElementTypes();

    // One line per element type: what the model and the menus say about it ("" for an unknown type)
    String UiElementHint(const String& type);

    // How the picture is drawn into a box (o2's sprite mode): simple, sliced, tiled
    const Vector<String>& UiAssetTypes();

    // ---------------------------------------------
    // Nine-slice borders in pixels of a picture
    // ---------------------------------------------
    struct UiNine
    {
        double left = 0, top = 0, right = 0, bottom = 0;

        bool operator==(const UiNine& other) const
        {
            return left == other.left && top == other.top && right == other.right && bottom == other.bottom;
        }
    };

    // The borders as JSON {left, top, right, bottom}
    Json NineToJson(const UiNine& nine);

    // A width x height pair (inches, pixels)
    struct UiPair
    {
        double w = 0, h = 0;

        bool operator==(const UiPair& other) const { return w == other.w && h == other.h; }
    };

    // ----------------------------------------------------------------------------------------------
    // One graphic of a kit. JSON key order (KitElementToJson) is the UI kit node's: name, type, assetType,
    // width, height, source, port, dpi, dpiFrom, inches, sizeFrom, share, soft, texture, variant, purpose,
    // description, nineSlice, png
    // ----------------------------------------------------------------------------------------------
    struct UiKitElement
    {
        String                name;
        String                type = "other";
        String                assetType = "simple";
        std::optional<String> variant;     // primary, secondary, disabled, selected... - free text the layout can ask for
        std::optional<String> purpose;     // Where to use it ("background of popups"): the layout agent picks graphics by it
        std::optional<String> description;
        double                width = 1, height = 1;
        std::optional<UiNine> nineSlice;
        std::optional<double> dpi;         // The density the picture is drawn for; none = the base
        std::optional<String> dpiFrom;     // item | kit (older kits: input | base): where dpi came from
        std::optional<UiPair> inches;      // The real size on the kit's device (the whole picture, before a centre collapse)
        std::optional<String> sizeFrom;    // item | source | tag | pixels: how the size was found
        std::optional<double> share;       // source: the element's share of the screen's width, 0..1
        bool                  soft = false; // The picture had fewer pixels than its size needs: kept, at a lower dpi
        std::optional<UiPair> texture;     // The pixels the picture had before it was resampled
        String                png;         // PNG, base64 (no data: prefix); empty for an own graphic drawn from its upload
        std::optional<String> port;        // The UI kit node's input it came from
        bool                  own = false; // Made by an AI UI layout node for itself (UiDocOwn.h)
        String                uploadId;    // An own graphic's upload (not written to JSON)
        Json                  source;      // Where the picture sits in the node's input ({w, h, trim, collapsed?, detected?}); null = none
    };

    // -----------------------------------------
    // A kit: graphics and text styles, version 1
    // -----------------------------------------
    struct UiKit
    {
        String                  name = "kit";
        std::optional<UiTarget> device;     // The device the kit is made for; none in a kit made before devices
        Vector<UiKitElement>    elements;
        Vector<TextStyle>       textStyles;
    };

    // An element as JSON, keys in the UI kit node's order
    Json KitElementToJson(const UiKitElement& element);

    // The kit as JSON: {version: 1, name, device?, elements, textStyles}
    Json KitToJson(const UiKit& kit);

    // A kit document read back (from a cache file, an upstream value or the model): cleaned; an empty kit for anything
    // that is not one
    UiKit ParseKit(const String& json);

    // A kit name: letters, digits, _ and - with no leading or trailing - / _ (it becomes a file name and a layout
    // reference); fallback when nothing is left
    String CleanKitName(const Json* value, const String& fallback);

    // Default element type for an element named like this (before anyone set it)
    String GuessElementType(const String& name);

    // Types that stretch by default (sliced), the rest keep their aspect (simple)
    String DefaultAssetType(const String& type);

    // Borders of any object {left, top, right, bottom} (rounded, not negative); none when they are all 0 or it is no object
    std::optional<UiNine> NormalizeNine(const Json* value);

    // An element's purpose as it is stored: one line, at most 200 characters; "" for none
    String CleanPurpose(const Json* value);

    // --------------------------------------------------------------------------------------
    // The UI kit node's per-element settings, config.items[portId]; borders are in INPUT pixels
    // --------------------------------------------------------------------------------------
    struct UiKitItem
    {
        Json                  raw;           // The settings as stored
        String                name;          // "" = the port's name
        String                type;          // "" = guessed from the name
        String                assetType;     // "" = the type's default
        String                variant;
        String                description;
        Json                  nineSlice;     // Borders in input pixels; null = detect (NormalizeNine reads it)
        String                bg;            // keep | key | ai ("" = keep)
        bool                  trim = true;   // Drop transparent margins (default on)
        bool                  optimize = true; // Shrink a uniform stretchable centre (default on, sliced only)
        String                preparedUploadId; // The picture after the AI background removal, an upload
        String                preparedFrom;  // The InputHash of the input it was made from
        bool                  ai = false;    // Filled in by the AI
        String                nineFrom;      // "ai" when the borders came from the AI's measurement
        String                purpose;       // Where to use the element (typed or written by the AI describe)
        String                purposeFrom;   // "ai" when the AI describe wrote the purpose
        std::optional<double> dpi;           // The density of the element's texture set by hand (cleaned); none = the kit's device
        std::optional<UiPair> size;          // The element's real size set by hand, inches; none = found from where it came from
    };

    // The per-element settings of a UI kit node's config, by input port id, in config order
    Vector<Pair<String, UiKitItem>> KitItems(const Json& config);

    // The per-element settings of a UI kit node's o2 config
    Vector<Pair<String, UiKitItem>> KitItems(const DataValue& config);

    // Config keys of the UI kit node that never change what it renders
    const Vector<String>& KitConfigUiOnly();

    // The kit's the AI button AI defaults: the vision model and the image model that removes complex backgrounds
    extern const char* const UI_KIT_AI_DEFAULT_MODEL;
    extern const char* const UI_KIT_AI_DEFAULT_IMAGE_MODEL;

    // The kit for the layout agent: graphics with their sizes in the layout's design pixels at layoutDpi (none = the
    // base), nine-slices, descriptions, and the text styles
    String KitSummary(const UiKit& kit, std::optional<double> layoutDpi = std::nullopt);

    // A kit element by name, case-insensitively; null when there is none
    const UiKitElement* KitElement(const UiKit* kit, const String& name);

    // A kit text style by name, case-insensitively; null when there is none
    const TextStyle* KitStyle(const UiKit* kit, const String& name);

    // How well a graphic's name (2 per word) and purpose + description (1 per word) fit a block id: shared meaningful
    // words, plural and stem forms counted (coins ~ coin); generic words (icon, btn, panel...) never count
    int NameScore(const String& id, const String& elementName, const String& elementDescription = "");
    int NameScore(const String& id, const UiKitElement& element);

    // The first element of the first of the types the kit has, preferring the variant, then the name that fits the hint
    // (a node id like coin_icon) best, then one without a variant (or primary / normal / default); null when none
    const UiKitElement* KitElementOfType(const UiKit* kit, const Vector<String>& types, const String& variant = "",
                                         const String& hint = "");

    // The PNG file name an element has in exports: <name>.png
    String KitFileName(const UiKitElement& element);
}
