#pragma once

#include <optional>

#include "o2/Utils/Coroutines/Coroutines.h"
#include "o2Editor/Pipeline/PipelineNodeType.h"
#include "o2Editor/Pipeline/UiDoc/UiDocAgent.h"
#include "o2Editor/Pipeline/UiDoc/UiDocOwn.h"
#include "o2Editor/Pipeline/UiLayout/UiLayoutAi.h"

// ------------------------------------------------------------------------------------------------
// The making of an AI UI layout node's own graphic (port of AssetsLine backend/src/uiKit/own.ts): a
// picture drawn by the image model from a description, or cut out of the reference picture, prepared the
// way the UI kit node prepares its inputs (margins trimmed, nine-slice found, the stretchable middle
// collapsed) and kept as an upload (PipelineUtils::GetUploadsPath)
// ------------------------------------------------------------------------------------------------
namespace Editor::UiLayout
{
    // ------------------------------------
    // A picture to keep as an own graphic
    // ------------------------------------
    struct OwnAssetInput
    {
        String                                 png;
        String                                 name;
        String                                 type = "image";
        String                                 assetType = "simple";
        String                                 description;
        String                                 purpose;
        std::optional<double>                  dpi;
        String                                 from = "drawn"; // drawn | cut
        std::optional<::Editor::UiDoc::UiNine> nineSlice;      // Borders already known, in the picture's pixels; else found here
        bool                                   asIs = false;   // The picture is final: no trim, no collapse
        std::optional<double>                  toDpi;          // The layout's density: a denser picture is resampled to it
    };

    // Prepares a picture as a kit graphic and keeps it as an upload; false with the reason when it cannot
    bool StoreOwnAsset(const OwnAssetInput& input, ::Editor::UiDoc::UiOwnAsset& out, String& error);

    // -------------------------------------------------------
    // What drawing a missing graphic works with
    // -------------------------------------------------------
    struct OwnDrawContext
    {
        Ref<PipelineExecContext> exec;
        String                   imageModel;
        double                   layoutDpi = 480;  // A graphic asked for at W x H design pixels gets the DPI that draws it so
        bool                     hasReference = false;
        String                   referencePng;     // The screen to rebuild, full size (design pixels)
        double                   refW = 0, refH = 0, refDpi = 480;
        Vector<String>           styleRefs;        // Pictures that show the style to draw in: the kit sheet, the reference
    };

    // An own graphic made, or why not
    struct OwnDrawResult
    {
        bool                        ok = false;
        String                      error;
        ::Editor::UiDoc::UiOwnAsset asset;
    };

    // The prompt that draws a graphic for a request (hasStyle: style pictures go along)
    String OwnDrawPrompt(const ::Editor::UiDoc::UiAssetRequest& request, bool hasStyle);

    // One graphic for a request: cut from the reference where the request points into it (the AI extract node's cut, the
    // element without what is drawn on it), else drawn by the image model in the style of the pictures
    Coroutine<OwnDrawResult> DrawOwnAsset(OwnDrawContext ctx, ::Editor::UiDoc::UiAssetRequest request, String name);
}
