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

        bool ownTransparency = false; // True when the part carries its own background settings ("transparency")

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

        // Writes the regions into the node config; the members a region has besides its id, name and box (its own
        // "transparency", anything newer) stay with the region of the same id
        void Write(PipelineNode& node, const Vector<PipelineExtractRegion>& regions);

        // Returns unique non-empty port names for the regions, in their order
        Vector<String> PortNames(const Vector<PipelineExtractRegion>& regions);

        // Rebuilds the output ports of the node from its regions, keeping the ids so links survive; a node without a
        // region list keeps one output named "out" (its first port, the others dropped), as AssetsLine does
        void SyncPorts(PipelineNode& node);

        // Returns the region a port carries, the first one when the port is unknown
        PipelineExtractRegion RegionOfPort(const PipelineNode& node, const String& portId);

        // Returns the own background settings ("transparency") of the part the port carries, null when it follows the node
        const DataValue* FindTransparency(const PipelineNode& node, const String& portId);

        // Returns the own background settings of the part the port carries for editing, null when it follows the node
        DataValue* FindTransparency(PipelineNode& node, const String& portId);

        // Gives the part its own background settings, copied complete from its effective ones, or drops them so it
        // follows the node again; returns false when nothing changed
        bool SetOwnTransparency(PipelineNode& node, const String& portId, bool own);

        // Prompt asking a vision model to list the parts of an image as boxes
        extern const String autoSplitPrompt;

        // Parses the model answer into regions: names and boxes, padded a little and clamped
        Vector<PipelineExtractRegion> ParseAutoSplit(const String& answer);
    }
}
