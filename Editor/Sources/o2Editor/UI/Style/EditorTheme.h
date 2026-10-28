#pragma once

#include "o2/Render/VectorGraphics/VectorImage.h"
#include "o2/Utils/Function/Function.h"
#include "o2/Utils/Math/Color.h"
#include "o2/Utils/Serialization/Serializable.h"
#include "o2/Utils/System/Time/TimeStamp.h"
#include "o2/Utils/Types/String.h"

using namespace o2;

namespace Editor
{
    // Look of the whole editor: the light o2 look or the dark AssetsLine one
    enum class EditorThemeKind { Light, Dark };

    // -----------------------------------------------------------------------------------------------------------------
    // Colours the editor code draws with, outside the styles: the light ones are the o2 look as it was, the dark ones the
    // AssetsLine tokens
    // -----------------------------------------------------------------------------------------------------------------
    struct EditorPalette
    {
        bool dark = false;

        Color4 text;          // Labels and captions made by window code
        Color4 textOnHeader;  // Glyphs over the component heads
        Color4 ink;           // Dark ink: curves, the timeline scale, icons drawn by code
        Color4 inkSoft;       // Quiet handles
        Color4 accent;        // Accent of code-drawn marks: transitions, function icons, the scene selection outline
        Color4 viewBack;      // Background of the scene view, the state graph, the curve and spline editors
        Color4 viewGrid;      // Their grid
        Color4 scaleLine;     // Timeline's big scale lines
        Color4 scaleLineSoft; // Timeline's small scale lines
        Color4 frame;         // Selection frames: the scene frame tool, the curve and the spline editors
        Color4 guide;         // Guide lines from curve and spline points to their handles
        Color4 selectionOutline; // Outline of the objects selected in the scene
        Color4 frameObject;   // Its object marks
        Color4 frameParent;   // Its parent marks
        Color4 warning;       // Log warning mark
        Color4 error;         // Log error mark
        Color4 checkerA;      // Transparency checker squares
        Color4 checkerB;
        Color4 prototypes[5]; // Names of prototype instances in the scene tree, one per depth of linking

        float iconOff = 0.3f; // Opacity of the icon of a toggle that is off: the scene tools, the pause
    };

    // ------------------------------------------------------------------------------------------------------------------
    // The editor theme. The light theme is the o2 look as it has always been. The dark one restyles the vector art when it
    // loads (colours by role and the AssetsLine gradients), recolours the built styles by their light colours, and gives the
    // editor code the dark palette. A switch reloads the vector art and the editor rebuilds its styles and its windows
    // (EditorApplication::SetTheme)
    // ------------------------------------------------------------------------------------------------------------------
    namespace EditorTheme
    {
        // Returns the current theme
        EditorThemeKind GetKind();

        // Returns is the dark theme on
        bool IsDark();

        // Sets the theme: the palette, the restyling of the vector art loaded from now on, and the loaded art reloaded
        void SetKind(EditorThemeKind kind);

        // Returns the palette of the current theme
        const EditorPalette& Get();

        // Returns the palette of a theme
        const EditorPalette& Of(EditorThemeKind kind);

        // Returns the version: changes on every switch of the theme
        int GetVersion();

        // Returns the colour a style colour of the light look has in the current theme; the same colour in the light one
        Color4 StyleColor(const Color4& lightColor);

        // Recolours every built style sample to the current theme: each goes through its serialized form, so the colours of
        // its states and animations change too. Nothing in the light theme
        void RecolorStyles();

        // Reloads the vector images already loaded, so they take the restyling of the current theme
        void ReloadVectorArt();

        // Returns the folder of the saved styles of the current theme
        String StylesFolder();

        // Returns when the rules of the dark style colours were edited last: a saved dark style is valid while it is newer
        TimeStamp RulesDate();

        // Returns the theme kind by its name ("light", "dark"), light for anything else
        EditorThemeKind KindByName(const String& name);

        // Returns the name of the theme kind
        String NameOf(EditorThemeKind kind);
    }

    // ----------------------------------------------------------------------
    // Editor preferences, shown in the properties window by Edit/Preferences
    // ----------------------------------------------------------------------
    class EditorPreferences : public ISerializable
    {
    public:
        EditorThemeKind theme = EditorThemeKind::Light; // Look of the editor @SERIALIZABLE

        SERIALIZABLE(EditorPreferences);
    };

    // -----------------------------------------------------------------------------------------------------------------
    // The theme the editor starts with next time. A change takes effect after a restart, so it is confirmed in a dialog;
    // a declined one puts the preferences back
    // -----------------------------------------------------------------------------------------------------------------
    class EditorThemeChoice
    {
    public:
        EditorPreferences preferences; // Shown by Edit/Preferences; a theme picked there is asked about on the update

        Function<void(EditorThemeKind)> onConfirmed; // Called with a confirmed theme, to save it

    public:
        // Sets the theme saved for the next start
        void Reset(EditorThemeKind next);

        // Asks to start in the theme next time
        void Request(EditorThemeKind kind);

        // Asks about a theme picked in the preferences
        void Update();

        // Returns the theme saved for the next start
        EditorThemeKind GetNext() const;

        // Returns is the question shown
        bool IsAsking() const;

    protected:
        EditorThemeKind mNext = EditorThemeKind::Light; // Theme saved for the next start
        bool            mAsking = false;                // True while the question is shown
    };
}
// --- META ---

PRE_ENUM_META(Editor::EditorThemeKind);

CLASS_BASES_META(Editor::EditorPreferences)
{
    BASE_CLASS(o2::ISerializable);
}
END_META;
CLASS_FIELDS_META(Editor::EditorPreferences)
{
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(EditorThemeKind::Light).NAME(theme);
}
END_META;
CLASS_METHODS_META(Editor::EditorPreferences)
{
}
END_META;
// --- END META ---
