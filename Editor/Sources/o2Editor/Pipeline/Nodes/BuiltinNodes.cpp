#include "o2Editor/stdafx.h"
#include "o2Editor/Pipeline/PipelineNodeType.h"

namespace Editor
{
    void RegisterTextNodes();
    void RegisterImageNodes();
    void RegisterAiNodes();
    void RegisterAudioVideoNodes();

    // Registration order is the order of the add-node menu inside each category
    void RegisterBuiltinPipelineNodes()
    {
        RegisterImageNodes();
        RegisterTextNodes();
        RegisterAudioVideoNodes();
        RegisterAiNodes();
    }
}
