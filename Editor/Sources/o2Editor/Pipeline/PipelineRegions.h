#pragma once

#include "o2Editor/Pipeline/PipelineGraph.h"

using namespace o2;

namespace Editor
{
    // ------------------------------------------------------------------------------
    // Parts the AI extract node cuts out: a normalised box on the source image and
    // what is inside it. A region id is the id of the output port carrying its result,
    // so links follow a region when it is renamed
    // ------------------------------------------------------------------------------
    struct PipelineExtractRegion
    {
        String id;   // Id of the output port that carries this part
        String name; // What to extract, also the port name
        float  x = 0.0f, y = 0.0f, w = 1.0f, h = 1.0f; // Normalised box on the source image

        // Returns true when both refer to the same part
        bool operator==(const PipelineExtractRegion& other) const { return id == other.id; }
    };

    namespace PipelineRegions
    {
        // Clamps a box into the unit square, keeping at least a sliver of size
        void NormalizeBox(float& x, float& y, float& w, float& h);

        // Returns the regions of the node; a node saved before regions existed has one,
        // built from its prompt and region of interest and carried by its existing output port
        Vector<PipelineExtractRegion> Read(const PipelineNode& node);

        // Writes the regions into the node config
        void Write(PipelineNode& node, const Vector<PipelineExtractRegion>& regions);

        // Returns unique non-empty port names for the regions, in their order
        Vector<String> PortNames(const Vector<PipelineExtractRegion>& regions);

        // Rebuilds the output ports of the node from its regions, keeping the ids so links survive
        void SyncPorts(PipelineNode& node);

        // Returns the region a port carries, the first one when the port is unknown
        PipelineExtractRegion RegionOfPort(const PipelineNode& node, const String& portId);

        // Prompt asking a vision model to list the parts of an image as boxes
        extern const String autoSplitPrompt;

        // Parses the model answer into regions: names and boxes, padded a little and clamped
        Vector<PipelineExtractRegion> ParseAutoSplit(const String& answer);
    }
}
