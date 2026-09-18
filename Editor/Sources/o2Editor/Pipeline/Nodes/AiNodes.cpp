#include "o2Editor/stdafx.h"
#include "o2Editor/Pipeline/Nodes/PipelineNodesCommon.h"
#include "o2Editor/Pipeline/PipelineRegions.h"

#include "o2Editor/Pipeline/Providers/ElevenLabsProvider.h"

namespace Editor
{
    static PipelineRunResult ImageBytesResult(const AiBytesResult& r)
    {
        if (!r.ok)
            return PipelineRunResult::Fail(r.error);

        auto bitmap = DecodeImageBytes(r.data);
        if (!bitmap)
            return PipelineRunResult::Fail("provider returned an undecodable image (" + r.mimeType + ")");

        return PipelineRunResult::Single(PipelineValue::Image(bitmap));
    }

    // Free-form text generation through Gemini; extra text inputs are appended, images attached
    class AiTextNode : public PipelineNodeBase
    {
    public:
        AiTextNode()
        {
            mSchema.type = "aiText";
            mSchema.label = "AI text gen";
            mSchema.category = PipelineNodeCategory::AI;
            mSchema.description = "Generate text via Google Gemini. Takes a prompt input; extra text inputs are appended, images are sent to the model.";
            mSchema.inputs = { In("prompt", PipelinePortType::Text) };
            mSchema.outputs = { Out("out", PipelinePortType::Text) };
            mSchema.addableInputs = { PipelinePortType::Text, PipelinePortType::Image };
        }

        Coroutine<PipelineRunResult> Run(const Ref<PipelineExecContext>& ctx, const Map<String, PipelineValue>& inputs,
                                         const Ref<PipelineNode>& node) override
        {
            auto promptVal = Input(inputs, "prompt");
            if (!promptVal || !promptVal->IsText())
                co_return PipelineRunResult::Fail("aiText: input \"prompt\" is not connected");

            Vector<String> extraTexts;
            Vector<AiImageRef> images;
            for (auto& port : node->inputs)
            {
                if (port.name == "prompt") continue;
                auto v = Input(inputs, port.name);
                if (!v || !v->IsValid()) continue;
                if (v->IsText()) { if (!String(v->data).Trimed(" \n\r\t").IsEmpty()) extraTexts.Add(port.name + ":\n" + v->data); }
                else if (v->IsImage()) images.Add({ "image/png", v->GetPngBytes() });
            }

            String model = node->GetConfigString("model", GeminiProvider::defaultTextModel);
            String systemPrompt = node->GetConfigString("systemPrompt", "").Trimed(" \n\r\t");
            Vector<String> parts = { systemPrompt, promptVal->data };
            parts.Add(extraTexts);
            String prompt = JoinNonEmpty(parts, "\n\n");

            ctx->Log("aiText: model=" + model + " promptChars=" + (String)prompt.Length() + " images=" + (String)images.Count());
            AiTextResult r = co_await GeminiProvider::GenerateText(ctx, ctx->settings.GetGeminiKey(), model, prompt, images);
            if (!r.ok) co_return PipelineRunResult::Fail(r.error);
            co_return PipelineRunResult::Single(PipelineValue::Text(r.text));
        }
    };

    // One described edit applied to a text through Gemini, the rest stays untouched
    class TextEditNode : public PipelineNodeBase
    {
    public:
        TextEditNode()
        {
            mSchema.type = "textEdit";
            mSchema.label = "AI text edit";
            mSchema.category = PipelineNodeCategory::AI;
            mSchema.description = "Apply a single described edit to the input text via Gemini, leaving everything else unchanged.";
            mSchema.inputs = { In("text", PipelinePortType::Text) };
            mSchema.outputs = { Out("out", PipelinePortType::Text) };
        }

        Coroutine<PipelineRunResult> Run(const Ref<PipelineExecContext>& ctx, const Map<String, PipelineValue>& inputs,
                                         const Ref<PipelineNode>& node) override
        {
            auto textVal = Input(inputs, "text");
            if (!textVal || !textVal->IsText())
                co_return PipelineRunResult::Fail("textEdit: input \"text\" is not connected");

            String instruction = node->GetConfigString("instruction", "").Trimed(" \n\r\t");
            if (instruction.IsEmpty())
                co_return PipelineRunResult::Fail("textEdit: no edit instruction provided");

            static const String systemPrompt =
                "You are a precise text-editing tool. You are given an ORIGINAL TEXT and an EDIT INSTRUCTION. "
                "Apply ONLY the specific change requested in the instruction. "
                "Leave everything else exactly as in the original - do not rephrase, reformat, fix typos, correct grammar, "
                "restructure, summarize, add, or remove anything that the instruction does not explicitly require. "
                "Preserve the original language of the text. Make the requested change directly and decisively. "
                "Output only the resulting edited text - no explanations, notes, preamble, or surrounding quotes.";

            String model = node->GetConfigString("model", GeminiProvider::defaultTextModel);
            String prompt = systemPrompt + "\n\nEDIT INSTRUCTION:\n" + instruction + "\n\nORIGINAL TEXT:\n" + textVal->data;
            ctx->Log("textEdit: model=" + model + " chars=" + (String)textVal->data.Length());
            AiTextResult r = co_await GeminiProvider::GenerateText(ctx, ctx->settings.GetGeminiKey(), model, prompt, {});
            if (!r.ok) co_return PipelineRunResult::Fail(r.error);
            co_return PipelineRunResult::Single(PipelineValue::Text(r.text));
        }
    };

    // Expands a short description into a prompt written for the target model kind
    class PromptGenNode : public PipelineNodeBase
    {
    public:
        PromptGenNode()
        {
            mSchema.type = "promptGen";
            mSchema.label = "AI prompt gen";
            mSchema.category = PipelineNodeCategory::AI;
            mSchema.description = "Expand a short description into a prompt written for the chosen target: image, video, text, sound effect, music, or a spoken line.";
            mSchema.inputs = { In("description", PipelinePortType::Text) };
            mSchema.outputs = { Out("out", PipelinePortType::Text) };
            mSchema.addableInputs = { PipelinePortType::Text, PipelinePortType::Image };
        }

        static int MaxCharsFor(const String& target)
        {
            if (target == "sfx") return ElevenLabsProvider::sfxMaxPromptChars;
            if (target == "music") return 1200;
            if (target == "speech") return 300;
            return 0;
        }

        static String BuildSystemPrompt(const String& target, int maxChars)
        {
            Vector<String> common = {
                "You are a prompt engineer. You receive a short, simple DESCRIPTION and must turn it into one polished, ready-to-use prompt.",
                "Faithfully expand the description into a richer, clearer, well-structured prompt - elaborate ONLY on what the description implies; never add unrelated subjects, never change its intent, never ask questions.",
                "You may also receive ADDITIONAL REFERENCE text and/or images; use them to inform, enrich, and ground the prompt, while keeping the DESCRIPTION as the primary intent.",
            };
            if (target != "speech")
                common.Add("Always write the prompt in ENGLISH, even when the description is in another language.");
            common.Add("Output ONLY the prompt itself: no explanations, no preamble, no commentary, no labels, no surrounding quotes, no markdown, no trailing notes.");
            if (maxChars > 0)
                common.Add("HARD LIMIT: the prompt MUST be at most " + (String)maxChars + " characters, including spaces. Count as you write and stay comfortably under it. A longer prompt is unusable and will be cut off.");

            Vector<String> specific;
            if (target == "sfx")
            {
                specific = {
                    "The prompt targets a TEXT-TO-SOUND-EFFECT model (ElevenLabs). Aim for 10-60 words - this model works best with a short, concrete description, NOT a cinematic paragraph.",
                    "Name the sound source concretely: material, size, force, distance and the acoustic space (e.g. \"small room\", \"wide open field\", \"tiled bathroom reverb\").",
                    "Describe how the sound evolves in time when it matters (e.g. \"sharp attack, short decay\", \"starts quiet and swells into a crash\").",
                    "Use sound-design vocabulary where it fits: impact, whoosh, thud, click, rustle, crackle, ambience, drone, one-shot, riser, braam, glitch. Onomatopoeia alongside the description helps.",
                    "If the description implies a CONTINUOUS background (rain, wind, engine hum, crowd, machinery), write it as a steady seamless ambience with no distinct beginning or end.",
                    "If it implies a single EVENT (hit, jump, coin pickup, door, UI click), write it as a clean isolated one-shot with silence around it.",
                    "Add a production-quality cue such as \"high-quality, dry, close-miked game sound effect, no music, no speech\" when it does not crowd out the description.",
                    "Never include dialogue, song lyrics, or musical melody in a sound-effect prompt." };
            }
            else if (target == "music")
            {
                specific = {
                    "The prompt targets a MUSIC-generation model (Google Lyria 3). Describe the track, not a scene.",
                    "Cover, in this order where they apply: genre or blend of genres, the specific instruments, tempo in BPM, musical key or scale, mood adjectives, and how the piece develops.",
                    "For game background music, state that it is instrumental with no vocals and no lyrics, that it sits under gameplay without demanding attention, and that it should loop cleanly.",
                    "You may use structure tags such as [Intro], [Verse], [Chorus], [Bridge], [Outro] when the description implies sections.",
                    "NEVER name a real artist, band, song title, or record label, and never quote existing lyrics - the model blocks such prompts." };
            }
            else if (target == "speech")
            {
                specific = {
                    "The prompt targets a TEXT-TO-SPEECH model, so what you write is NOT a description of speech - it is the exact LINE OF DIALOGUE that will be spoken aloud.",
                    "Write only the words to be voiced, in natural spoken language, punctuated for delivery (commas and full stops set the pacing).",
                    "Do NOT include stage directions, emotion labels, speaker names, asterisks, brackets, or any description of the voice - those belong in the node's separate delivery field, not in the spoken text.",
                    "Keep it to what a character would actually say in the moment the description implies - usually one or two short sentences, as game dialogue and combat barks must be brief.",
                    "Write it in the language the DESCRIPTION asks to be spoken; when no language is specified, use English." };
            }
            else if (target == "text")
            {
                specific = { "The prompt targets a TEXT-generation model. Make it a precise, self-contained instruction that fully specifies the task, the desired output, tone, format, and any constraints implied by the description." };
            }
            else if (target == "video")
            {
                specific = {
                    "The prompt targets a VIDEO-generation model (such as Veo or Kling). Vividly describe the subject and scene, then the ACTION and MOTION over the course of the clip - what happens from the first frame to the last.",
                    "Specify camera work (angle and movement, e.g. static shot, slow pan, tracking, dolly-in), the art style or medium, lighting, color palette, mood, and pacing.",
                    "Keep it one flowing descriptive prompt (not a list) - concise but rich, suited to a short clip of a few seconds." };
            }
            else
            {
                specific = {
                    "The prompt targets an IMAGE-generation model. Vividly describe the subject and its key attributes, plus the art style or medium, composition and framing, lighting, color palette, mood, and level of detail.",
                    "Keep it a flowing descriptive prompt (not a list of questions) - concise but rich." };
            }

            common.Add(specific);
            return JoinNonEmpty(common, " ");
        }

        Coroutine<PipelineRunResult> Run(const Ref<PipelineExecContext>& ctx, const Map<String, PipelineValue>& inputs,
                                         const Ref<PipelineNode>& node) override
        {
            auto descVal = Input(inputs, "description");
            if (!descVal || !descVal->IsText())
                co_return PipelineRunResult::Fail("promptGen: input \"description\" is not connected");

            String description = String(descVal->data).Trimed(" \n\r\t");
            if (description.IsEmpty())
                co_return PipelineRunResult::Fail("promptGen: description is empty");

            Vector<String> extraTexts;
            Vector<AiImageRef> images;
            for (auto& port : node->inputs)
            {
                if (port.name == "description") continue;
                auto v = Input(inputs, port.name);
                if (!v || !v->IsValid()) continue;
                if (v->IsText()) { if (!String(v->data).Trimed(" \n\r\t").IsEmpty()) extraTexts.Add(port.name + ":\n" + v->data); }
                else if (v->IsImage()) images.Add({ "image/png", v->GetPngBytes() });
            }

            String model = node->GetConfigString("model", GeminiProvider::defaultTextModel);
            String target = node->GetConfigString("target", "image");
            static const Vector<String> targets = { "image", "video", "text", "sfx", "music", "speech" };
            if (!targets.Contains(target)) target = "image";

            String overrideRaw = node->GetConfigString("maxChars", "").Trimed();
            int maxChars = MaxCharsFor(target);
            if (!overrideRaw.IsEmpty())
            {
                int o = (int)PipelineUtils::ValueToNumber(*node->GetConfigValue("maxChars"), 0);
                if (o > 0) maxChars = o;
            }
            if (target == "sfx" && (maxChars <= 0 || maxChars > ElevenLabsProvider::sfxMaxPromptChars))
            {
                if (maxChars > ElevenLabsProvider::sfxMaxPromptChars)
                    ctx->Log("promptGen: max characters capped at " + (String)ElevenLabsProvider::sfxMaxPromptChars + " (ElevenLabs hard limit)");
                maxChars = ElevenLabsProvider::sfxMaxPromptChars;
            }

            Vector<String> parts = { BuildSystemPrompt(target, maxChars) };
            String custom = node->GetConfigString("systemPrompt", "").Trimed(" \n\r\t");
            if (!custom.IsEmpty())
                parts.Add("ADDITIONAL INSTRUCTIONS (follow them as long as they do not conflict with the rules above):\n" + custom);
            parts.Add("DESCRIPTION:\n" + description);
            if (!extraTexts.IsEmpty()) parts.Add("ADDITIONAL REFERENCE TEXT:\n" + JoinNonEmpty(extraTexts, "\n\n"));
            if (!images.IsEmpty()) parts.Add("ADDITIONAL REFERENCE IMAGES are attached - use them to inform the prompt.");
            String prompt = JoinNonEmpty(parts, "\n\n");

            ctx->Log("promptGen: model=" + model + " target=" + target + " chars=" + (String)description.Length() +
                     (maxChars ? " budget=" + (String)maxChars : String()));
            AiTextResult r = co_await GeminiProvider::GenerateText(ctx, ctx->settings.GetGeminiKey(), model, prompt, images);
            if (!r.ok) co_return PipelineRunResult::Fail(r.error);

            String result = String(r.text).Trimed(" \n\r\t");
            if (maxChars > 0 && result.Length() > maxChars)
            {
                String trimmed = PipelineUtils::ClampPromptChars(result, maxChars);
                ctx->Log("promptGen: model returned " + (String)result.Length() + " chars over the " + (String)maxChars + " budget - trimmed to " + (String)trimmed.Length());
                result = trimmed;
            }
            co_return PipelineRunResult::Single(PipelineValue::Text(result));
        }
    };

    // Common transparent-render flow of the image nodes: chroma raw render, or white/black matte
    static Coroutine<PipelineRunResult> GenerateWithTransparency(const Ref<PipelineExecContext>& ctx, const PipelineNode& node,
                                                                 const String& model, const String& prompt,
                                                                 const Vector<AiImageRef>& references, const String& chromaSuffixPrompt,
                                                                 const String& whitePrompt)
    {
        String apiKey = ctx->settings.GetGeminiKey();
        bool transparent = node.GetConfigBool("transparentBg", false);
        auto tr = PipelineTransparency::Read(node);

        if (transparent && tr.mode == "chroma")
        {
            AiBytesResult raw = co_await GeminiProvider::GenerateImage(ctx, apiKey, model, chromaSuffixPrompt, references, ctx->seed);
            co_return ImageBytesResult(raw);
        }

        if (transparent)
        {
            AiBytesResult white = co_await GeminiProvider::GenerateImage(ctx, apiKey, model, whitePrompt, references, ctx->seed);
            if (!white.ok) co_return PipelineRunResult::Fail(white.error);
            AiBytesResult black = co_await GeminiProvider::GenerateImage(ctx, apiKey, model, PipelineTransparency::blackBgInstruction,
                                                                         { { "image/png", white.data } }, ctx->seed);
            if (!black.ok) co_return PipelineRunResult::Fail(black.error);

            auto w = DecodeImageBytes(white.data);
            auto b = DecodeImageBytes(black.data);
            if (!w || !b) co_return PipelineRunResult::Fail("provider returned undecodable images");
            co_return PipelineRunResult::Single(PipelineValue::Image(PipelineTransparency::MatteFromPair(*w, *b)));
        }

        AiBytesResult png = co_await GeminiProvider::GenerateImage(ctx, apiKey, model, prompt, references, ctx->seed);
        co_return ImageBytesResult(png);
    }

    // Image generation through the Gemini image models, with the optional transparent background flow
    class NanoBananaGenNode : public PipelineNodeBase
    {
    public:
        NanoBananaGenNode()
        {
            mSchema.type = "nanoBananaGen";
            mSchema.label = "AI image gen";
            mSchema.category = PipelineNodeCategory::AI;
            mSchema.description = "Generate an image via Google Gemini image model. Prompt and reference are optional; add any number of extra reference image inputs.";
            mSchema.inputs = { In("prompt", PipelinePortType::Text), In("reference", PipelinePortType::Image) };
            mSchema.outputs = { Out("out", PipelinePortType::Image) };
            mSchema.addableInputs = { PipelinePortType::Image };
        }

        Coroutine<PipelineRunResult> Run(const Ref<PipelineExecContext>& ctx, const Map<String, PipelineValue>& inputs,
                                         const Ref<PipelineNode>& node) override
        {
            auto promptVal = Input(inputs, "prompt");
            auto references = ImageInputs(inputs, *node);
            String model = node->GetConfigString("model", GeminiProvider::defaultImageModel);
            String basePrompt = promptVal && promptVal->IsText() ? promptVal->data : String();
            String prompt = JoinNonEmpty({ basePrompt, node->GetConfigString("extraPrompt", "") }, "\n");
            auto tr = PipelineTransparency::Read(*node);
            bool transparent = node->GetConfigBool("transparentBg", false);

            ctx->Log("nanoBananaGen: model=" + model + " promptChars=" + (String)prompt.Length() + " refs=" + (String)references.Count() +
                     " transparent=" + (transparent ? "true" : "false") + (transparent ? " mode=" + tr.mode : String()));

            PipelineRunResult r = co_await GenerateWithTransparency(ctx, *node, model, prompt, references,
                JoinNonEmpty({ PipelineTransparency::ChromaBgInstruction(tr), prompt }, "\n\n"),
                JoinNonEmpty({ PipelineTransparency::whiteBgInstruction, prompt }, "\n\n"));
            co_return r;
        }
    };

    // Prompted image edit through Gemini; a drawn overlay marks where and what to change
    class ImageEditNode : public PipelineNodeBase
    {
    public:
        ImageEditNode()
        {
            mSchema.type = "imageEdit";
            mSchema.label = "AI image edit";
            mSchema.category = PipelineNodeCategory::AI;
            mSchema.description = "Edit the input image via Gemini using a text prompt and an optional hand-drawn overlay marking the change.";
            mSchema.inputs = { In("image", PipelinePortType::Image) };
            mSchema.outputs = { Out("out", PipelinePortType::Image) };
        }

        Coroutine<PipelineRunResult> Run(const Ref<PipelineExecContext>& ctx, const Map<String, PipelineValue>& inputs,
                                         const Ref<PipelineNode>& node) override
        {
            static const String systemPrompt =
                "You are a precise image-editing tool. You are given an ORIGINAL image and an instruction describing a change. "
                "Apply ONLY the requested change. Keep everything else in the image exactly as in the original - "
                "do not alter the composition, framing, style, colors, lighting, or any region the instruction does not mention. "
                "If a SECOND image is provided, it is the original with a hand-drawn overlay marking where and what to change; "
                "use that drawing only as guidance for the location and intent of the edit, and produce a clean result without the drawn marks. "
                "Preserve the resolution and aspect ratio of the original. Make the change directly and decisively. "
                "Output only the edited image.";
            static const String whiteBgSystem =
                "Then isolate the main subject of the edited result and place it on a solid, uniform, pure white (#FFFFFF) background "
                "that fills the whole frame, removing the original background entirely. Keep the subject fully opaque with crisp edges. "
                "Do not add shadows, gradients, reflections, or vignettes.";

            auto imageVal = Input(inputs, "image");
            if (!imageVal || !imageVal->IsImage()) co_return PipelineRunResult::Fail("imageEdit: input \"image\" is not connected");
            String prompt = node->GetConfigString("prompt", "").Trimed(" \n\r\t");
            if (prompt.IsEmpty()) co_return PipelineRunResult::Fail("imageEdit: no edit prompt provided");

            String model = node->GetConfigString("model", GeminiProvider::defaultImageModel);
            Vector<AiImageRef> references = { { "image/png", imageVal->GetPngBytes() } };
            if (auto drawing = DecodeImageBytes(PipelineUtils::DataUrlToBytes(node->GetConfigString("drawing", ""))))
            {
                if (PipelineImageOps::HasContent(*drawing))
                {
                    if (auto base = imageVal->GetBitmap())
                        references.Add({ "image/png", EncodeBitmapPng(*PipelineImageOps::CompositeOverlay(*base, *drawing)) });
                }
            }

            auto tr = PipelineTransparency::Read(*node);
            ctx->Log("imageEdit: model=" + model + " refs=" + (String)references.Count() + " promptChars=" + (String)prompt.Length());

            PipelineRunResult r = co_await GenerateWithTransparency(ctx, *node, model, systemPrompt + "\n\n" + prompt, references,
                systemPrompt + "\n\n" + prompt + "\n\nThen isolate the main subject of the edited result and remove the original background entirely. " + PipelineTransparency::ChromaBgInstruction(tr),
                systemPrompt + "\n\n" + prompt + "\n\n" + whiteBgSystem);
            co_return r;
        }
    };

    // Cuts a described element out of an image as a transparent sprite through Gemini
    class ImageExtractNode : public PipelineNodeBase
    {
    public:
        ImageExtractNode()
        {
            mSchema.type = "imageExtract";
            mSchema.label = "AI extract part";
            mSchema.category = PipelineNodeCategory::AI;
            mSchema.description = "Cut a described (and optionally drawn-over) part out of the input image as a complete, transparent game sprite.";
            mSchema.inputs = { In("image", PipelinePortType::Image) };
            mSchema.outputs = { Out("out", PipelinePortType::Image) };
            mSchema.perPortRun = true;
        }

        // Editing one part must not invalidate the others: the shared config is hashed without the
        // part list, and this part's own name and box are folded back in
        Vector<String> PortCacheExcludedKeys() const override { return { "regions", "roi", "prompt" }; }

        bool SyncPorts(const Ref<PipelineNode>& node) const override
        {
            PipelineRegions::SyncPorts(*node);
            return true;
        }

        String PortCacheVariant(const PipelineNode& node, const String& portId) const override
        {
            auto region = PipelineRegions::RegionOfPort(node, portId);
            return "region:" + region.name + ":" + (String)region.x + "," + (String)region.y + "," + (String)region.w + "," + (String)region.h;
        }

        static String BuildExtractPrompt(const String& part, const String& backdrop)
        {
            return "This image is a cropped region of a 2D game that contains the element to extract: " + part + ". "
                "Keep ONLY that element, exactly as it appears - same shape, position, size, colors, shading, texture and art style. Do NOT move, resize, recolor, restyle, or redraw it. "
                "Erase EVERYTHING else in the crop to " + backdrop + ": the background AND any other objects, pieces, icons, text or fragments that are not the requested element. "
                "If a SECOND image with hand-drawn marks is provided, the marked areas are EXTRA things to remove - erase them too, and never paint the drawn strokes into the result. "
                "This is NOT background removal - remove the other objects as well. The result must be almost entirely that flat backdrop colour with only the requested element visible. If the element is partly hidden, plausibly complete it. "
                "Output only the resulting image.";
        }

        Coroutine<PipelineRunResult> Run(const Ref<PipelineExecContext>& ctx, const Map<String, PipelineValue>& inputs,
                                         const Ref<PipelineNode>& node) override
        {
            auto imageVal = Input(inputs, "image");
            if (!imageVal || !imageVal->IsImage()) co_return PipelineRunResult::Fail("imageExtract: input \"image\" is not connected");

            // Each output port carries one part: its box is the region of interest, its name the prompt
            auto region = PipelineRegions::RegionOfPort(*node, ctx->outputPortId);
            String partName = ctx->outputPort.IsEmpty() ? String("out") : ctx->outputPort;
            String prompt = region.name.Trimed(" \n\r\t");
            if (prompt.IsEmpty()) co_return PipelineRunResult::Fail("imageExtract: no extraction prompt for \"" + partName + "\" - name the part to cut out");

            auto source = imageVal->GetBitmap();
            if (!source) co_return PipelineRunResult::Fail("imageExtract: input is not a decodable image");

            String model = node->GetConfigString("model", GeminiProvider::defaultImageModel);
            String apiKey = ctx->settings.GetGeminiKey();

            PipelineImageOps::CropRect roi{ region.x, region.y, region.w, region.h };
            bool hasRoi = region.x > 0.0f || region.y > 0.0f || region.w < 1.0f || region.h < 1.0f;
            Ref<Bitmap> roiInput = hasRoi ? PipelineImageOps::Crop(*source, roi) : source;
            Vector<AiImageRef> references = { { "image/png", EncodeBitmapPng(*roiInput) } };
            if (auto drawing = DecodeImageBytes(PipelineUtils::DataUrlToBytes(node->GetConfigString("drawing", ""))))
            {
                if (PipelineImageOps::HasContent(*drawing))
                {
                    Ref<Bitmap> roiDrawing = hasRoi ? PipelineImageOps::Crop(*drawing, roi) : drawing;
                    references.Add({ "image/png", EncodeBitmapPng(*PipelineImageOps::CompositeOverlay(*roiInput, *roiDrawing)) });
                }
            }

            bool transparent = node->GetConfigBool("transparentBg", false);
            auto tr = PipelineTransparency::Read(*node);
            ctx->Log("imageExtract . " + partName + ": model=" + model + " refs=" + (String)references.Count() + " transparent=" + (transparent ? "true" : "false"));

            if (transparent && tr.mode == "chroma")
            {
                AiBytesResult raw = co_await GeminiProvider::GenerateImage(ctx, apiKey, model, BuildExtractPrompt(prompt, PipelineTransparency::ChromaEraseInstruction(tr)), references, ctx->seed);
                co_return ImageBytesResult(raw);
            }

            AiBytesResult white = co_await GeminiProvider::GenerateImage(ctx, apiKey, model, BuildExtractPrompt(prompt, "flat, solid, pure white (#FFFFFF)"), references, ctx->seed);
            if (!white.ok) co_return PipelineRunResult::Fail(white.error);
            auto whiteBmp = DecodeImageBytes(white.data);
            if (!whiteBmp) co_return PipelineRunResult::Fail("provider returned an undecodable image");

            if (!transparent)
                co_return PipelineRunResult::Single(PipelineValue::Image(PipelineImageOps::CropToContent(*whiteBmp, PipelineImageOps::ContentMode::White)));

            AiBytesResult black = co_await GeminiProvider::GenerateImage(ctx, apiKey, model, PipelineTransparency::blackBgInstruction, { { "image/png", white.data } }, ctx->seed);
            if (!black.ok) co_return PipelineRunResult::Fail(black.error);
            auto blackBmp = DecodeImageBytes(black.data);
            if (!blackBmp) co_return PipelineRunResult::Fail("provider returned an undecodable image");

            auto recovered = PipelineTransparency::MatteFromPair(*whiteBmp, *blackBmp);
            if (PipelineImageOps::HasContent(*recovered))
                co_return PipelineRunResult::Single(PipelineValue::Image(PipelineImageOps::CropToContent(*recovered, PipelineImageOps::ContentMode::Alpha)));

            ctx->Log("imageExtract . " + partName + ": matte was empty - falling back to the white result");
            co_return PipelineRunResult::Single(PipelineValue::Image(PipelineImageOps::CropToContent(*whiteBmp, PipelineImageOps::ContentMode::White)));
        }
    };

    // Background removal by the image model: the subject stays exactly as it is, everything
    // around it is painted over with a flat backdrop, which is then keyed out or matted away
    class AiRemoveBgNode : public PipelineNodeBase
    {
    public:
        AiRemoveBgNode()
        {
            mSchema.type = "aiRemoveBg";
            mSchema.label = "AI remove background";
            mSchema.category = PipelineNodeCategory::AI;
            mSchema.description = "Make the background of an image transparent with the image model: the subject stays exactly as it is, everything around it goes.";
            mSchema.inputs = { In("image", PipelinePortType::Image) };
            mSchema.outputs = { Out("out", PipelinePortType::Image) };
        }

        void InitNode(const Ref<PipelineNode>& node) const override
        {
            // The node exists to make the background transparent: only the method is a choice
            node->SetConfigBool("transparentBg", true);
            node->SetConfigString("transparentMode", "chroma");
        }

        static String BuildPrompt(const String& hint, const String& backdrop)
        {
            String subject = hint.IsEmpty() ?
                String("Keep the main subject - the foreground object(s) or character(s) - exactly as it appears: same shape, position, size, colors, shading, texture and art style.") :
                "The subject to keep is: " + hint + ". Keep it exactly as it appears - same shape, position, size, colors, shading, texture and art style.";

            return "Remove the background of this image. " + subject +
                " Do NOT move, resize, recolor, restyle or redraw the subject. Keep a shadow, outline or glow that belongs to the subject itself. "
                "Replace EVERYTHING that is background with " + backdrop + ": scenery, floor, sky, walls, gradients, patterns, props and clutter that are not part of the subject. "
                "The result must be the untouched subject on that flat backdrop and nothing else. Output only the resulting image.";
        }

        Coroutine<PipelineRunResult> Run(const Ref<PipelineExecContext>& ctx, const Map<String, PipelineValue>& inputs,
                                         const Ref<PipelineNode>& node) override
        {
            auto imageVal = Input(inputs, "image");
            if (!imageVal || !imageVal->IsImage()) co_return PipelineRunResult::Fail("aiRemoveBg: input \"image\" is not connected");

            String model = node->GetConfigString("model", GeminiProvider::defaultImageModel);
            String hint = node->GetConfigString("hint", "").Trimed(" \n\r\t");
            auto tr = PipelineTransparency::Read(*node);
            Vector<AiImageRef> references = { { imageVal->mimeType.IsEmpty() ? String("image/png") : imageVal->mimeType, imageVal->data } };

            ctx->Log("aiRemoveBg: model=" + model + " hintChars=" + (String)hint.Length() + " mode=" + tr.mode);

            co_return co_await GenerateWithTransparency(ctx, *node, model, BuildPrompt(hint, "a flat, solid backdrop"), references,
                BuildPrompt(hint, PipelineTransparency::ChromaEraseInstruction(tr)),
                BuildPrompt(hint, "flat, solid, pure white (#FFFFFF)"));
        }
    };

    void RegisterAiNodes()
    {
        PipelineNodeRegistry::Register(mmake<AiTextNode>());
        PipelineNodeRegistry::Register(mmake<TextEditNode>());
        PipelineNodeRegistry::Register(mmake<PromptGenNode>());
        PipelineNodeRegistry::Register(mmake<NanoBananaGenNode>());
        PipelineNodeRegistry::Register(mmake<ImageEditNode>());
        PipelineNodeRegistry::Register(mmake<ImageExtractNode>());
        PipelineNodeRegistry::Register(mmake<AiRemoveBgNode>());
    }
}
