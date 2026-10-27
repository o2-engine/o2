#pragma once

#include "o2/Utils/Serialization/DataValue.h"

using namespace o2;

namespace Editor
{
    class PipelineNodeWidget;

    // ----------------------------------------------------------------------------------------
    // Layout of a card as blocks (kind, rectangle from the card's top-left with y down, text), the
    // format the AssetsLine web shots are dumped in; the theme gallery compares the two editors with it
    // ----------------------------------------------------------------------------------------
    struct PipelineLayoutDump
    {
        // Writes the card's blocks into out: {card: {w, h}, blocks: [...]}
        static void Dump(const PipelineNodeWidget& card, DataValue& out);
    };
}
