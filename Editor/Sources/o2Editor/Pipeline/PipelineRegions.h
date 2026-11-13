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

        bool   ownTransparency = false; // True when the part carries its own background settings ("transparency")
        String model;                   // The part's own image model ("model"), empty when it is cut with the node's
        bool   background = false;      // A background plate found by the auto-split (only set on its answers)

        // Returns true when the part has settings of its own: its model or its background
        bool IsOwn() const { return ownTransparency || !model.IsEmpty(); }

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

        // Returns the stored object of the part the port carries for editing, null when the node keeps no region list
        DataValue* FindPart(PipelineNode& node, const String& portId);

        // Returns the own background settings ("transparency") of the part the port carries, null when it follows the node
        const DataValue* FindTransparency(const PipelineNode& node, const String& portId);

        // Returns the own background settings of the part the port carries for editing, null when it follows the node
        DataValue* FindTransparency(PipelineNode& node, const String& portId);

        // Gives the part its own background settings, copied complete from its effective ones, or drops them so it
        // follows the node again; returns false when nothing changed
        bool SetOwnTransparency(PipelineNode& node, const String& portId, bool own);

        // Returns the part's own image model, empty when it is cut with the node's
        String OwnModel(const PipelineNode& node, const String& portId);

        // Returns the image model the part the port carries is cut with: its own, else the node's
        String PartModel(const PipelineNode& node, const String& portId);

        // Gives the part its own model and background settings (copied from what it uses now), or drops both so it follows
        // the node again; returns false when nothing changed
        bool SetOwnPart(PipelineNode& node, const String& portId, bool own);

        // Sets the own model of a part that has its own settings; returns false when the part follows the node
        bool SetPartModel(PipelineNode& node, const String& portId, const String& model);

        // True for a part that is the background plate behind a screen: drawn opaque, everything on top of it removed. The
        // auto-split names such a part "bg_..." and turns its transparency off; the extraction recognises it by both
        bool IsBackgroundPart(const String& name, bool transparent);

        extern const char* autoSplitDefaultModel;    // The vision model the auto-split asks when the node names none
        extern const char* extractDefaultImageModel; // The image model a new extract node cuts its parts with

        // How the auto-split asks for boxes: Gemini's box_2d grid, or pixels of the w x h picture it is sent
        struct SplitFrame
        {
            String boxes = "box_2d"; // box_2d or pixels
            int    w = 0;
            int    h = 0;
        };

        // The prompt that splits a picture into the pieces of a UI kit (AssetsLine's autoSplitPrompt)
        String AutoSplitPrompt(const SplitFrame& frame);

        // Parses the model answer into regions with fresh ids: the kit name then what to cut as the name, the box padded a
        // little and clamped, the background first (at most one, whole when it covers nearly everything); frame says how
        // the boxes were asked for, without it a "box" is read by its magnitude
        Vector<PipelineExtractRegion> ParseAutoSplit(const String& answer, const SplitFrame* frame = nullptr);

        // How much larger the model drew a part than the crop it was given: the crop is fitted inside, so the smaller
        // ratio, at 0.0001; 0 when a size is missing
        double PartScale(int cropW, int cropH, int frameW, int frameH);

        // The source's density for the parts: the node's sourceDpi, else what the picture carries, else the base
        double ExtractSourceDpi(const PipelineNode& node, const String& sourcePng);

        // Tags a part's result with its density (source DPI x scale). stored is what the cache holds (the raw render of a
        // chroma part, else the result itself); a raw render cached before parts carried their scale is measured against
        // its crop. Returns the result unchanged when no scale is known
        String TagExtractPart(const PipelineNode& node, const String& portId, const String& sourcePng, const String& storedPng,
                              const String& outPng, bool raw);

        // The scale of a finished part against its region of the source, when the part is clearly not at the scale its
        // generation claims (nominal); none when it is, or when there is nothing to measure by
        std::optional<double> MeasuredPartScale(const String& sourcePng, const PipelineExtractRegion& region, const String& partPng,
                                                double nominal);
    }
}
