#pragma once

#include <optional>

#include "o2/Scene/UI/Widget.h"
#include "o2/Scene/UI/Widgets/Label.h"
#include "o2/Render/Sprite.h"
#include "o2Editor/Pipeline/UiLayout/UiLayoutJson.h"
#include "o2Editor/Pipeline/UiLayout/UiLayoutSlice.h"

namespace Editor::UiLayout
{
    // Values of a ui_structure.json read the way the skill's Python reads them
    namespace PyValue
    {
        // float(x): numbers, booleans and numeric text
        bool Float(const Json* v, double& out);

        // layout._to_int: int(round(float(x))), round() to the even neighbour; def when it is not a number
        int ToInt(const Json* v, int def = 0);

        // A member as a number, def when it is not one
        double Num(const Json* parent, const char* key, double def = 0.0);

        // A member as a string, empty when it is not one
        String Str(const Json* parent, const char* key);

        // Python truthiness of a member
        bool Truthy(const Json* v);
    }

    // A node of a ui_structure.json with its box as the skill's layout.py resolves it
    struct UiResolvedNode
    {
        String path; // root/popup/close
        Box    rel;  // Top-left, y down, relative to the parent
        Box    abs;  // In canvas pixels
    };

    // Resolves every node's box the way layout.resolve_positions does: a parent's row / column / grid layout, align / vAlign
    // / offset, responsive constraints at the reference size, scroll offsets; parents first. False with the reason for a
    // structure the skill would reject (an unknown layout type, a bad grid, a non-positive responsive box)
    bool ResolveStructure(const Json& structure, Vector<UiResolvedNode>& out, String* error = nullptr);

    // The structure as the skill renders it (ui_components.snapshot): every state block's current variant applied to its
    // targets, parents first, and progress labels written from their format
    Json SnapshotStructure(const Json& structure);

    // Formats a progress label as Python's str.format does for {value}, {min}, {max}, {percent} with an optional .Nf spec;
    // false for anything else
    bool FormatProgress(const String& format, double value, double min, double max, String& out);

    // A sprite file as the plan sees it
    struct UiSpriteInfo
    {
        std::optional<NineSlice> nine; // Its .png.meta borders
        int                      w = 0, h = 0;
    };

    // The nine-slice borders a node's nineSlice asks for, as render_comparison.py reads them: true, "auto" or "meta" take the
    // sprite's .png.meta borders (else a quarter of its smaller side, 4..60), a number all four sides, an object its sides
    // (missing ones 0); none for false or nothing
    std::optional<NineSlice> NineSliceOf(const Json* value, const UiSpriteInfo* sprite);

    // Font file of a weight: Roboto-Regular.ttf, Roboto-Medium.ttf or Roboto-Bold.ttf
    String FontFileOf(const String& weight);

    // Weight of a fontFamily ("fonts/Roboto-Bold.ttf", "Roboto-Medium.ttf"): bold, medium or regular
    String WeightOfFont(const String& file);

    // o2 text height in points of a pixel font size: round(fontSize * 72 / 92), at least 1
    int HeightPoints(double fontSize);

    // How one node of the structure becomes a widget
    struct UiPlanNode
    {
        String path;       // The skill's element path: root/popup/close
        String widgetPath; // Path by widget names from the prototype root, the key of a kept actor id
        String name;
        String type;       // container, image, rect, text, button, overlay
        String widget;     // Widget, Image, Label, Button, GridLayout, HorizontalProgress, VerticalProgress
        int    parent = -1;
        Box    rel;
        Box    abs;

        Vec2F anchorMin, anchorMax, offsetMin, offsetMax; // o2's layout of the box: y up, from the parent's anchor points

        String                   asset;   // Sprite file the node draws, empty for none
        std::optional<NineSlice> nine;
        String                   color;   // #RRGGBB: fill of rect / overlay / container, tint of an image, colour of a text
        double                   opacity = 1.0;
        bool                     visible = true;

        String text;
        String fontFile;          // Roboto-*.ttf
        double fontSize = 16;
        String alignment;         // left, center, right
        String textVAlign;        // top, middle, bottom
        String strokeColor;       // Outline colour, empty for none
        double strokeWidth = 0;

        String progressOrientation; // Right, Left, Down, Up for a progress fill
        double progressValue = 1.0; // Its ratio, 0..1

        Vec2F gridCell;            // GridLayout: cell size
        float gridSpacing = 0;     // GridLayout: one spacing for both axes
        int   gridColumns = 0;
        Vec2F gridBorder;          // GridLayout: left and top border
    };

    // The widgets a structure becomes and what they cannot reproduce
    struct UiWidgetPlan
    {
        String             rootName; // canvas.name
        Vec2F              size;     // canvas size
        Vector<UiPlanNode> nodes;    // Parents first, the structure's root first
        Vector<String>     warnings;
    };

    // Plans the widgets of a ui_structure.json: the snapshot's boxes resolved like the skill does, anchors (a responsive block
    // becomes o2's anchors and offsets, otherwise the box is attached to its anchor point), sprites with their nine-slice
    // borders (sprites: by lower-case file name), texts, progress fills, native GridLayouts where o2 reproduces the grid. False
    // with the reason when the structure does not resolve
    bool PlanWidgets(const Json& structure, const Map<String, UiSpriteInfo>& sprites, UiWidgetPlan& out, String* error = nullptr);

    // ------------------------------------------------------------------------------------------------
    // Where the widget builder takes sprites and fonts from: the exported assets of the prototype
    // ------------------------------------------------------------------------------------------------
    class UiWidgetResources
    {
    public:
        virtual ~UiWidgetResources() = default;

        // Returns the sprite of a structure asset ("<name>.png"), null when there is none
        virtual Ref<Sprite> SpriteOf(const String& asset) = 0;

        // Returns a white sprite, tinted for flat colours
        virtual Ref<Sprite> WhiteSprite() = 0;

        // Sets the label's font from a font file (Roboto-Bold.ttf)
        virtual void SetFont(const Ref<Label>& label, const String& fontFile) = 0;

        // Gives the label a style drawing the glyphs in fill with an outline of the radius in stroke
        virtual void SetStroke(const Ref<Label>& label, const Color4& fill, const Color4& stroke, int radius) = 0;
    };

    // The widget tree of a plan
    struct UiWidgetTree
    {
        Ref<Widget>         root;
        Vector<Ref<Widget>> widgets;  // Every widget in drawing order
        Vector<String>      warnings; // The plan's and the builder's
    };

    // How the tree is built
    struct UiWidgetOptions
    {
        bool                  explicitDepths = false; // Drawing depths in tree order instead of inheriting them
        String                layer;                  // Scene layer of every widget, none when empty
        Map<String, SceneUID> keepIds;                // Actor ids by widget path, kept by a re-export
    };

    // Builds the native widgets of a plan outside the scene
    UiWidgetTree BuildWidgets(const UiWidgetPlan& plan, UiWidgetResources& resources, const UiWidgetOptions& options);

    // Lays out a tree that is not in the scene and settles its progress bars
    void UpdateWidgetTree(const Ref<Widget>& root);

    // Returns the path of every widget below and including the root, by widget names
    Map<String, SceneUID> CollectActorIds(const Ref<Actor>& root);
}
