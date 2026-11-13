#pragma once

#include <optional>

#include "o2/Utils/Coroutines/Coroutine.h"
#include "o2Editor/Pipeline/PipelineZip.h"
#include "o2Editor/Pipeline/UiDoc/UiUnityExport.h"
#include "o2Editor/Pipeline/UiLayout/UiLayoutExport.h"

namespace Editor::UiDoc
{
    // ---------------------------------------------------------------------------------------------------------------
    // The AI UI layout's Download (the card's and the workspace's; port of AssetsLine's downloadLayout in
    // frontend/src/nodeEditor/uiLayout/uiLayoutState.ts) for the node's exportFor target, from the layout as
    // ToUiStructure writes it:
    //   unity - a .unitypackage: the prefab (the Canvas Scaler in Expand mode; a turned block's RectTransform turns about
    //           a centred pivot), its sprites (nine-slice borders in their metas, pixels-per-unit multiplier = the sprite's
    //           density) and every font the text styles use with its licence;
    //   o2    - the prototype written into the project's Assets the way the AI image to UI node writes its own
    //           (UiLayout::ExportPrototype), sprites resampled to the layout's density, a turned block's widget turned about
    //           a centred pivot (the web's o2 export leaves it unturned with a warning); o2 draws non-Roboto styles with
    //           Roboto (a warning says so);
    //   json  - a zip of ui_structure.json, layout.uiml, layout.json, the sprites tagged with their DPI, kit.json, the
    //           fonts used with their licences and the rendered picture
    // Guids and uids come from the node id, so a re-export keeps them
    // ---------------------------------------------------------------------------------------------------------------

    // The export targets, the node's exportFor values
    extern const char* const UI_EXPORT_UNITY; // "unity", the default
    extern const char* const UI_EXPORT_O2;    // "o2"
    extern const char* const UI_EXPORT_JSON;  // "json"

    // The node's export target (exportFor): unity, o2 or json; unity for anything else
    String ExportTargetOf(const Json& config);

    // The file a target is saved as: <name>.unitypackage, <name>_o2.zip, <name>.zip
    String ExportFileName(const UiDoc& doc, const String& target);

    // What an export needs: editor-agnostic
    struct UiLayoutExportRequest
    {
        UiDoc                doc;
        std::optional<UiKit> kit;        // The kit wired in; placeholders without one
        String               target;     // unity, o2 or json (ExportTargetOf)
        String               nodeId;     // Seeds guids and uids
        String               assetPath;  // The node's assetPath: the folder inside Assets, Generated/UI when empty
        String               assetsPath; // o2: the project's assets folder (the .proto is written only when it is the editor's)
        String               outputPath; // unity, json: the file to write
        String               picture;    // json: the rendered layout as PNG (preview.png), none when empty
    };

    // What an export wrote
    struct UiLayoutExportResult
    {
        bool           ok = false;
        String         error;
        Vector<String> written;   // The file for unity / json; the Assets paths for o2
        Vector<String> warnings;  // What the target does not reproduce: the structure's, the font notes, the exporter's
        String         prototype; // o2: Assets path of the .proto, empty when it was not written
    };

    // Exports the layout for the request's target. Main thread for o2 (it waits for the assets rebuild)
    Coroutine<UiLayoutExportResult> ExportLayout(UiLayoutExportRequest request);

    // ----- the pieces, for tests --------------------------------------------------------------------------------------

    // The fonts the Unity export packs: the Roboto faces with their licence, then every other font the structure names
    // and their licences, read from the bundled fonts; a file that cannot be read goes into missing
    Vector<UiExportFont> UnityExportFonts(const UiStructureResult& structure, Vector<String>* missing = nullptr);

    // The Unity export's input: name, folder (assetPath cleaned, Generated/UI when empty), layout, sprites, fonts, seed; the
    // Canvas Scaler in Expand mode (canvasFit)
    UiExportInput UnityExportInput(const UiDoc& doc, const UiStructureResult& structure, const String& assetPath,
                                   const String& seed, const Vector<UiExportFont>& fonts);

    // The entries of the JSON export's zip, in the web's order; fonts read from the bundled ones
    Vector<PipelineZip::Entry> JsonExportEntries(const UiDoc& doc, const UiStructureResult& structure, const String& picture,
                                                 Vector<String>* missing = nullptr);

    // What the o2 export hands to ExportPrototype: sprites resampled to one pixel per layout pixel with their borders, and
    // the layout with the borders a node gives such a sprite scaled the same way
    UiLayout::ExportRequest O2ExportRequest(const UiDoc& doc, const UiStructureResult& structure, const String& assetPath,
                                            const String& assetsPath, const String& seed);

    // The notes of the o2 export for the fonts it draws with Roboto
    Vector<String> O2FontNotes(const UiStructureResult& structure);
}
