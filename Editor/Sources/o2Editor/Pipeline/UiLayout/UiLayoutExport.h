#pragma once

#include "o2Editor/Pipeline/UiLayout/UiLayoutRun.h"
#include "o2Editor/Pipeline/UiLayout/UiLayoutWidgets.h"

namespace Editor::UiLayout
{
    // What to export
    struct ExportRequest
    {
        String                  assetsPath; // The project's assets folder, with a trailing slash
        String                  folder;     // Folder inside it: Generated/UI
        String                  name;       // Screen name: the prototype's file name
        String                  layout;     // The skill's ui_structure.json
        Vector<ImageToUiSprite> sprites;    // Every sprite the layout names: <key>.png with its nine-slice borders
    };

    // What the export wrote
    struct ExportResult
    {
        bool           ok = false;
        String         error;
        String         prototype; // Assets path of the .proto, empty when it was not written
        Vector<String> files;     // Assets paths written or kept
        Vector<String> warnings;  // What the native widgets do not reproduce
        int            widgets = 0;
    };

    // Writes the prototype of a ui_structure.json into the project: <folder>/<name>.proto, <name>_Resources/Sprites/<key>.png,
    // Fonts/Roboto-*.ttf and FontStyles/*.fntstyle for outlined labels, every asset with a meta (a re-export keeps their uids
    // and the actor ids of the prototype). The widgets come from PlanWidgets (the skill's own layout resolution, the current
    // states, explicit drawing depths on the UI layer). Only the editor that owns the assets writes the .proto. Main thread
    // work; may block on the assets rebuild
    Coroutine<ExportResult> ExportPrototype(ExportRequest request);

    // The sprite infos of a plan by lower-case file name: sizes and nine-slice borders of the request's sprites
    Map<String, UiSpriteInfo> SpriteInfos(const Vector<ImageToUiSprite>& sprites);
}
