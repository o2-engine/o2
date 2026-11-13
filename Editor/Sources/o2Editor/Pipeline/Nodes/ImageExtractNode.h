#pragma once

#include "o2/Utils/Bitmap/Bitmap.h"
#include "o2/Utils/Coroutines/Coroutines.h"
#include "o2Editor/Pipeline/PipelineNodeType.h"

using namespace o2;

namespace Editor
{
    // ---------------------------------------------------------------------------------------------------------------
    // The AI extract part's generation (AssetsLine backend/src/nodes/imageExtract.node.ts): one part drawn from its crop
    // of the source, shared by the node's run and the regenerations of its check against the reference
    // ---------------------------------------------------------------------------------------------------------------
    namespace PipelineExtract
    {
        // tEXt key of where the model drew a part inside its frame: "frameW,frameH,left,top" of the content box the result
        // was trimmed to. Written only while the check against the reference is on; a chroma part needs none (its stored
        // render IS the frame)
        extern const char* const frameKey;

        // ---------------------------------------------
        // Where a part sat in the frame the model drew
        // ---------------------------------------------
        struct Frame
        {
            int w = 0, h = 0, left = 0, top = 0;
        };

        // Reads the frame a part's PNG carries; false when it carries none
        bool ReadPngFrame(const String& png, Frame& out);

        // One generation of the part the port carries from its crop of the source: the raw key-colour render of a chroma
        // part (the executor cuts it), else the finished cut-out, tagged with how much larger the model drew it and, while
        // the check is on, its frame. extra is added to the prompt: the clarifications a regeneration gets from the check
        Coroutine<PipelineRunResult> GeneratePart(Ref<PipelineExecContext> ctx, Ref<PipelineNode> node, String portId,
                                                  Ref<Bitmap> roi, String prompt, String extra);

        // The crop of the source a part is drawn from: its region of the picture, the whole picture for a whole region
        Ref<Bitmap> RoiOf(const Bitmap& source, const PipelineNode& node, const String& portId);
    }

    // Registers the AI extract part node
    void RegisterImageExtractNode();
}
