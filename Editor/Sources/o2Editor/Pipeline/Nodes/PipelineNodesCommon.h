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

    // Transparency approaches shared by the image gen / edit / extract nodes. A part of the extract node may carry
    // its own background settings ("transparency" of its region), which replace the node's for that part.
    // A model that renders the alpha itself is asked for it (native), whatever approach the config names
    namespace PipelineTransparency
    {
        // Transparency settings of a node or of one of its parts
        struct Config
        {
            bool                            transparent = false; // Transparent background asked for
            String                          mode = "twoPass";    // Effective approach: twoPass (white and black renders), chroma (key colour cut) or native (the model renders the alpha)
            String                          storedMode = "twoPass"; // Approach the config names, twoPass or chroma: used again once the model cannot render the alpha
            PipelineImageOps::ChromaOptions chroma;              // Key colour, tolerance, softness and spill of the chroma cut
            String                          colorName;           // Key colour name used in prompts
            String                          colorHex;            // Key colour as upper-case hex used in prompts
        };

        // True for the models that render a transparent background themselves when asked: the OpenAI GPT Image models
        // and the OpenRouter models whose images endpoint takes the request
        bool SupportsNativeTransparency(const String& model);

        // True when the node, or the part the port carries, gets its transparent background from the model itself
        bool UsesNativeTransparency(const PipelineNode& node, const String& portId = "");

        // The six config keys a part's own background settings replace: transparentBg, transparentMode and the chroma keys
        const Vector<String>& SettingKeys();

        // Returns a background setting of the part the port carries: its own when it has one, else the node's; null when
        // neither is set. An empty port id reads the node
        const DataValue* SettingValue(const PipelineNode& node, const String& portId, const char* key);

        // Reads the effective settings of the node, or of the part the port carries, clamping the percentages
        Config Read(const PipelineNode& node, const String& portId = "");

        // Writes the complete effective settings of the part the port carries into target, as its own copy
        void WriteSettings(const PipelineNode& node, const String& portId, DataValue& target);

        // Prompt part asking for the subject on a flat chroma key backdrop
        String ChromaBgInstruction(const Config& config);

        // Backdrop description for prompts that erase everything else to the key colour
        String ChromaEraseInstruction(const Config& config);

        extern const String nativeBackdrop;     // What the prompts call the background when the model renders the alpha
        extern const String nativeEditSuffix;   // Ending of an edit prompt when the model renders the alpha
        extern const String whiteBgInstruction; // First two-pass prompt: subject on pure white
        extern const String blackBgInstruction; // Second two-pass prompt: the same render with the background turned black

        // Config keys that only feed the local chroma post-step, so changing them reuses the render
        const Vector<String>& ChromaConfigKeys();

        // True when the node, or the part the port carries, renders onto a key colour the executor must cut
        bool UsesChromaPostStep(const PipelineNode& node, const String& portId = "");

        // True when the node or any of its parts cuts a key colour
        bool AnyChromaPostStep(const PipelineNode& node);

        // Adds the keys the signature of a per-port output drops to exclude and returns what it appends to the port
        // variant: a part with its own settings drops the node's and adds its own ("|bg:[...]", downstream also the cut),
        // a raw chroma render is tagged "|chroma-raw". A part without own settings hashes as before they existed
        String PortCacheSuffix(const PipelineNode& node, const String& portId, bool rawRender, Vector<String>& exclude);

        // Cuts the key colour out of a raw render (trims margins for extract nodes) with the settings of the node or the part
        Ref<Bitmap> ApplyChromaPostStep(const PipelineNode& node, const Bitmap& raw, const String& portId = "");

        // Recovers alpha from two renders of the same subject on white and on black
        Ref<Bitmap> MatteFromPair(const Bitmap& white, const Bitmap& black);

        // Image generations one run of the node takes: per part, two for a transparent two-pass render, else one;
        // a background rendered by the model takes one
        int ImageGenerations(const PipelineNode& node);
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

    // Value of a source node (text, image, audio) straight from its config; false with the reason when it has none.
    // assetsPath is the folder asset paths of the config are relative to
    bool ResolveSourceValue(const PipelineNode& node, const String& assetsPath, PipelineValue& value, String& error);
}
