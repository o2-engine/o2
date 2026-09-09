#pragma once

#include "o2Editor/Pipeline/PipelineImageOps.h"
#include "o2Editor/Pipeline/PipelineNodeType.h"
#include "o2Editor/Pipeline/PipelineUtils.h"
#include "o2Editor/Pipeline/Providers/GeminiProvider.h"

using namespace o2;

namespace Editor
{
    // -----------------------------------------------------------------
    // Base for built-in nodes: owns the schema and offers input helpers
    // -----------------------------------------------------------------
    class PipelineNodeBase : public IPipelineNodeImpl
    {
    public:
        // Returns the schema filled in by the derived constructor: type, label, category and ports
        const PipelineNodeSchema& GetSchema() const override { return mSchema; }

    protected:
        PipelineNodeSchema mSchema; // Node type, label, category, ports and flags, filled in by the derived constructor

    protected:
        // Makes an input port whose id equals its name
        static PipelinePort In(const String& name, PipelinePortType type) { return PipelinePort(name, name, type, false); }

        // Makes an output port whose id equals its name
        static PipelinePort Out(const String& name, PipelinePortType type) { return PipelinePort(name, name, type, false); }

        // Returns the value connected to the named input port, nullptr when nothing is connected
        static const PipelineValue* Input(const Map<String, PipelineValue>& inputs, const String& name)
        {
            auto it = inputs.find(name);
            return it == inputs.end() ? nullptr : &it->second;
        }

        // Every connected text input of the node in port order
        static Vector<String> TextInputs(const Map<String, PipelineValue>& inputs, const PipelineNode& node)
        {
            Vector<String> res;
            for (auto& port : node.inputs)
            {
                if (port.portType != PipelinePortType::Text) continue;
                auto v = Input(inputs, port.name);
                if (v && v->IsText()) res.Add(v->data);
            }
            return res;
        }

        // Every connected image input of the node in port order, as references
        static Vector<AiImageRef> ImageInputs(const Map<String, PipelineValue>& inputs, const PipelineNode& node, const String& skipName = "")
        {
            Vector<AiImageRef> res;
            for (auto& port : node.inputs)
            {
                if (port.portType != PipelinePortType::Image || port.name == skipName) continue;
                auto v = Input(inputs, port.name);
                if (v && v->IsImage()) res.Add({ "image/png", v->GetPngBytes() });
            }
            return res;
        }

        // Joins the non-empty parts with the separator
        static String JoinNonEmpty(const Vector<String>& parts, const String& separator)
        {
            String res;
            for (auto& p : parts)
            {
                if (p.IsEmpty()) continue;
                if (!res.IsEmpty()) res += separator;
                res += p;
            }
            return res;
        }
    };

    // Transparency approaches shared by the image gen / edit / extract nodes
    namespace PipelineTransparency
    {
        // Transparency settings of a node
        struct Config
        {
            String                          mode = "twoPass"; // Approach: twoPass (white and black renders) or chroma (key colour cut)
            PipelineImageOps::ChromaOptions chroma;           // Key colour, tolerance, softness and spill of the chroma cut
            String                          colorName;        // Key colour name used in prompts
            String                          colorHex;         // Key colour as upper-case hex used in prompts
        };

        // Reads the mode and the chroma settings from the node config, clamping the percentages
        Config Read(const PipelineNode& node);

        // Prompt part asking for the subject on a flat chroma key backdrop
        String ChromaBgInstruction(const Config& config);

        // Backdrop description for prompts that erase everything else to the key colour
        String ChromaEraseInstruction(const Config& config);

        extern const String whiteBgInstruction; // First two-pass prompt: subject on pure white
        extern const String blackBgInstruction; // Second two-pass prompt: the same render with the background turned black

        // Config keys that only feed the chroma post-step
        const Vector<String>& ChromaConfigKeys();

        // True when the node renders onto a key colour the executor must cut
        bool UsesChromaPostStep(const PipelineNode& node);

        // Cuts the key colour out of a raw render (trims margins for extract nodes)
        Ref<Bitmap> ApplyChromaPostStep(const PipelineNode& node, const Bitmap& raw);

        // Recovers alpha from two renders of the same subject on white and on black
        Ref<Bitmap> MatteFromPair(const Bitmap& white, const Bitmap& black);
    }

    // ----------------------------------------------------------------------------------------
    // Composer layer: an image input port or a duplicate of one, in draw order (back to front)
    // ----------------------------------------------------------------------------------------
    struct ComposerLayerRef
    {
        String id;          // Port id, or the duplicate id from the "dupLayers" config
        String portId;      // Id of the image input port the pixels come from
        String name;        // Layer name shown in the composer
        bool   dup = false; // True when the layer duplicates another port

        // Layers are equal when their ids match
        bool operator==(const ComposerLayerRef& other) const { return id == other.id; }
    };

    // Image input ports first, duplicates after them, explicit "layerOrder" overrides
    Vector<ComposerLayerRef> ResolveComposerLayers(const PipelineNode& node);

    // Reads the crop of a node config into a rect, false when whole frame
    bool ReadNodeCrop(const PipelineNode& node, const char* key, PipelineImageOps::CropRect& crop);
}
