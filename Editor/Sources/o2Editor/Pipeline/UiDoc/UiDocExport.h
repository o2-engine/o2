#pragma once

#include <optional>

#include "o2/Utils/Coroutines/Coroutine.h"
#include "o2Editor/Pipeline/UiDoc/UiDocStructure.h"
#include "o2Editor/Pipeline/UiLayout/UiLayoutExport.h"

namespace Editor::UiDoc
{
    // ---------------------------------------------------------------------------------------------------------------
    // The AI UI layout's Save to project (the card's and the workspace's): the layout as ToUiStructure writes it becomes
    // an o2 prototype in the project's Assets the way the AI image to UI node writes its own (UiLayout::ExportPrototype):
    // the .proto, its sprites resampled to the layout's density with their slice borders, every bundled font the text
    // styles use with its licence, a font style per text look (fill or gradient, outlines, shadows and glows), a turned
    // block's widget turned about a centred pivot. Uids come from the node id, so a second save keeps them. The node's
    // exportFor key (the web's download target) is ignored and left as it is
    // ---------------------------------------------------------------------------------------------------------------

    // What a save needs: editor-agnostic
    struct UiLayoutExportRequest
    {
        UiDoc                doc;
        std::optional<UiKit> kit;        // The kit wired in; placeholders without one
        String               nodeId;     // Seeds the uids
        String               assetPath;  // The node's assetPath: the folder inside Assets, Generated/UI when empty
        String               assetsPath; // The project's assets folder (the .proto is written only when it is the editor's)
    };

    // What a save wrote
    struct UiLayoutExportResult
    {
        bool           ok = false;
        String         error;
        Vector<String> written;   // Assets paths
        Vector<String> warnings;  // What the prototype does not reproduce
        String         prototype; // Assets path of the .proto, empty when it was not written
    };

    // The folder inside Assets a layout node saves into: its assetPath trimmed, without leading or trailing slashes,
    // Generated/UI when empty
    String ExportFolderOf(const String& assetPath);

    // The Assets path of the .proto a layout node saves: <folder>/<screen name>.proto
    String ExportPrototypePathOf(const UiDoc& doc, const String& assetPath);

    // Saves the layout as an o2 prototype. Main thread (it waits for the assets rebuild)
    Coroutine<UiLayoutExportResult> ExportLayout(UiLayoutExportRequest request);

    // What the save hands to ExportPrototype: sprites resampled to one pixel per layout pixel with their borders, and the
    // layout with the borders a node gives such a sprite scaled the same way and each styled text's `textStyle`
    UiLayout::ExportRequest PrototypeRequest(const UiDoc& doc, const UiStructureResult& structure, const String& assetPath,
                                             const String& assetsPath, const String& seed);
}
