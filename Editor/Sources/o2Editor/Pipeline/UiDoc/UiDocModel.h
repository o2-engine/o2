#pragma once

#include <array>
#include <functional>
#include <optional>

#include "o2/Utils/Types/Containers/Map.h"
#include "o2Editor/Pipeline/UiDoc/UiDocDevice.h"

// ------------------------------------------------------------------------------------------------
// The AI UI layout's document (port of AssetsLine shared/uiDoc/model.ts): a screen as a tree of typed
// blocks, adaptive the way o2's widget layouts and HTML flex boxes are - every block is either placed
// by its parent's layout (row / column / grid) or anchored inside its parent (a corner, an edge, the
// centre, or stretched along an axis with margins). Nothing in it says how a block looks: a UI kit skins
// it by kit=<name> or by the block's type.
//
// Optional keys are std::optional: absent and present differ in UIML, in edits (key=- removes) and in JSON.
// `keys` keeps the order the optional keys were set in - the order JSON.stringify writes a node in the web
// (a key set by an edit goes after the others, as it does on a JavaScript object)
// ------------------------------------------------------------------------------------------------
namespace Editor::UiDoc
{
    // Four sides in CSS order: top, right, bottom, left
    using Box4 = std::array<double, 4>;

    // One edit of a layout, kept as the JSON object a node's config stores it as ({op: "update", id, set, unset}...): the
    // values of `set` stay raw and are cleaned when applied, so stored steps round-trip unchanged. Builders and
    // readers: UiDocOps.h
    using UiOp = Json;

    // Block types: screen (the root), group, panel, background, image, icon, button, text, progress, toggle, slider,
    // input, tab, badge, divider, slot
    const Vector<String>& UiNodeTypes();

    // The keys a node may carry, in the order UIML writes them (text last)
    const Vector<String>& NodeKeys();

    // -------------------------------------------------------------------------------------------
    // One block of a layout. Values are as the web keeps them: numbers are doubles (cols too), texts
    // are UTF-8. Hidden = visible == false
    // -------------------------------------------------------------------------------------------
    struct UiNode
    {
        String id;
        String type = "group";

        // --- placement inside the parent (when the parent has no layout) ---
        std::optional<String> anchor;           // Canonical preset: top-left, bottom-center, top-stretch, center, stretch...
        std::optional<double> x, y;             // Offset from the anchor: inwards from the anchored edge, or from the centre
        std::optional<double> w, h;
        std::optional<Box4>   margin;           // Margins to the parent's edges along stretched axes
        std::optional<double> grow;             // Inside a row / column: share of the free space along the main axis
        std::optional<double> rotation;         // Turned about the centre of its box, degrees clockwise (UiDocRotate.h): drawn
                                                // turned, its children with it; the layout of the boxes does not change

        // --- layout of the children ---
        std::optional<String> layout;           // row | column | grid
        std::optional<double> gap, gapY;        // gapY: the grid's vertical gap
        std::optional<Box4>   pad;
        std::optional<String> align;            // start | center | end | stretch: cross axis (grid: the grid horizontally)
        std::optional<String> justify;          // start | center | end | between | around | evenly (grid: vertically)
        std::optional<double> cols, cellW, cellH;

        // --- look ---
        std::optional<String> kit;
        std::optional<String> variant;
        std::optional<String> style;
        std::optional<String> color;            // Tint of the kit picture, or the fill in line art
        std::optional<double> opacity;
        std::optional<bool>   visible;
        std::optional<String> fit;              // contain | cover | stretch

        // --- content ---
        std::optional<String> text;
        std::optional<String> textAlign;
        std::optional<double> fontSize;
        std::optional<String> textColor;
        std::optional<double> value;            // progress / slider 0..1
        std::optional<bool>   on;               // toggle
        std::optional<String> placeholder;
        std::optional<String> note;             // What the block is for (the model reads it; nothing draws it)

        std::vector<UiNode> children;
        bool                hasChildren = false; // The children key is present (an empty list differs from none in JSON)

        Vector<String> keys; // Optional keys (and "children") in the order they were set: the JSON key order

    public:
        // True when the optional key is set (any of NodeKeys, or "children")
        bool Has(const String& key) const;

        // The value of an optional key as JSON (a Box4 as an array), null when unset
        Json Get(const String& key) const;

        // Sets an optional key from a cleaned value (CleanProp's output), keeping its place in the key order or appending it
        void Set(const String& key, const Json& cleaned);

        // Removes an optional key ("children" drops the children)
        void Unset(const String& key);

        // Replaces the children (the key is appended when it was not there)
        void SetChildren(const std::vector<UiNode>& kids);

        // Drops the children and the key
        void ClearChildren();

        // False when visible == false
        bool IsShown() const { return !(visible && !*visible); }
    };

    // Types that hold other blocks without drawing anything themselves (group, screen)
    inline bool IsGroup(const UiNode& n) { return n.type == "group" || n.type == "screen"; }

    // --------------------------------------------------------------------------------------------
    // A screen: name, design size, density and the root block. JSON key order: name, width, height, dpi,
    // root - dpi after root when an edit added it later (dpiAfterRoot)
    // --------------------------------------------------------------------------------------------
    struct UiDoc
    {
        String                name = "Screen";   // The prototype's name when saved to the project
        double                width = 1080;      // Design resolution
        double                height = 1920;
        std::optional<double> dpi;               // Density of the design pixels; none = the base
        UiNode                root;
        bool                  dpiAfterRoot = false;

        UiDoc() { root.id = "root"; root.type = "screen"; }
    };

    // The default design size
    constexpr double DEFAULT_DOC_WIDTH = 1080;
    constexpr double DEFAULT_DOC_HEIGHT = 1920;

    // An empty screen (its root has an empty children list, as the web's)
    UiDoc EmptyDoc(const String& name = "Screen", double width = DEFAULT_DOC_WIDTH, double height = DEFAULT_DOC_HEIGHT,
                   std::optional<double> dpi = std::nullopt);

    // The layout's density: its own or the base
    inline double DocDpi(const UiDoc& doc) { return DocDpi(doc.dpi); }

    // Size a block of the type takes when its box does not say (unknown types: a group's)
    UiSize DefaultSize(const String& type);

    // --- anchors -----------------------------------------------------------------------------------

    // ------------------------------------------------------------------------------
    // An anchor's two axes: h = left | center | right | stretch, v = top | middle | bottom | stretch
    // ------------------------------------------------------------------------------
    struct Anchor
    {
        String h = "left";
        String v = "top";

        bool operator==(const Anchor& other) const { return h == other.h && v == other.v; }
    };

    // Any spelling of an anchor (top-left, left top, center, bottom, top-stretch, stretch, fill, h-stretch...) -> the axes
    Anchor ParseAnchor(const String& raw);

    // The anchor of a node (its anchor key, top-left when unset)
    Anchor ParseAnchor(const std::optional<String>& raw);

    // The canonical name: <v>-<h>, center or stretch
    String AnchorName(const Anchor& a);

    // The 16 presets in the picker's order
    const Vector<String>& AnchorPresets();

    // --- cleaning ------------------------------------------------------------------------------------

    // A type word or alias -> the canonical type and the layout an alias implies ("" = none); false for no type
    bool ResolveType(const String& word, String& type, String& layout);

    // #RGB / #RRGGBB / RRGGBB -> #RRGGBB upper case; false for anything else
    bool CleanColor(const Json* value, String& out);

    // letters, digits, _ and - (spaces become _), at most 64
    String CleanId(const Json* value);
    String CleanId(const String& value);

    // 1, 2 or 4 numbers (an array, a number, a "T,R,B,L" text, an object {top,right,bottom,left}) -> four sides; none
    // when they are all 0 or it is not that
    std::optional<Box4> ParseBox4(const Json* value);

    // One property cleaned as the web cleans it; false = drop it
    bool CleanProp(const String& key, const Json* value, Json& out);

    // ----------------------------------------------------
    // Ids taken in a document and the counters of auto ids
    // ----------------------------------------------------
    struct UiIdBook
    {
        Map<String, bool> used;
        Map<String, int>  counters;

        // True when the id is taken
        bool Has(const String& id) const { return used.find(id) != used.end(); }
    };

    // Any object (stored, parsed, written by the model) -> a clean node; ids made unique against the book. Unknown keys
    // are dropped
    UiNode NormalizeNode(const Json& raw, UiIdBook& book, bool isRoot = false);
    UiNode NormalizeNode(const Json& raw);

    // Any object -> a clean document
    UiDoc NormalizeDoc(const Json& raw);

    // --- JSON ----------------------------------------------------------------------------------------

    // The node as JSON in its key order (id, type, then `keys`); withChildren false leaves the children out
    Json NodeToJson(const UiNode& node, bool withChildren = true);

    // The document as JSON (JSON.stringify(doc) of the web's object)
    Json DocToJson(const UiDoc& doc);

    // A node object read as it is (no cleaning, ids kept; numbers from numeric text): for showing stored edits
    UiNode NodeFromJson(const Json& raw);

    // --- tree helpers --------------------------------------------------------------------------------

    // Calls fn for the node and every block under it, depth first: (node, parent or null, depth)
    void Walk(const UiNode& node, const std::function<void(const UiNode&, const UiNode*, int)>& fn, const UiNode* parent = nullptr,
              int depth = 0);

    // The first block with the id, depth first; null when none
    UiNode* FindNode(UiDoc& doc, const String& id);
    const UiNode* FindNode(const UiDoc& doc, const String& id);

    // The parent of the first block with the id; null for the root or when none
    UiNode* FindParent(UiDoc& doc, const String& id);
    const UiNode* FindParent(const UiDoc& doc, const String& id);

    // How many blocks the document has (the root not counted)
    int CountNodes(const UiDoc& doc);

    // Every id of the document
    Vector<String> AllIds(const UiDoc& doc);
}
