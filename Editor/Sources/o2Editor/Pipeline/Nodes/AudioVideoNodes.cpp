#include "o2Editor/stdafx.h"
#include "o2Editor/Pipeline/Nodes/PipelineNodesCommon.h"

#include "o2Editor/Pipeline/PipelineAudio.h"
#include "o2Editor/Pipeline/Providers/ElevenLabsProvider.h"
#include "o2Editor/Pipeline/Providers/VideoProviders.h"

namespace Editor
{
    // Defined in TextNodes.cpp
    String WriteFinishAsset(const Ref<PipelineExecContext>& ctx, const PipelineNode& node, const String& defaultName,
                            const String& ext, const String& bytes);

    static PipelineRunResult AudioResult(const AiBytesResult& r)
    {
        if (!r.ok) return PipelineRunResult::Fail(r.error);
        return PipelineRunResult::Single(PipelineValue::Bytes(PipelinePortType::Audio, r.data, r.mimeType));
    }

    // Audio file or project sound asset picked in the node
    class SourceAudioNode : public PipelineNodeBase
    {
    public:
        SourceAudioNode()
        {
            mSchema.type = "sourceAudio";
            mSchema.label = "Audio source";
            mSchema.category = PipelineNodeCategory::Source;
            mSchema.description = "Pick an audio file (mp3 / wav / ogg) or a project sound asset - reused from cache on every run.";
            mSchema.outputs = { Out("out", PipelinePortType::Audio) };
        }

        Coroutine<PipelineRunResult> Run(const Ref<PipelineExecContext>& ctx, const Map<String, PipelineValue>& inputs,
                                         const Ref<PipelineNode>& node) override
        {
            PipelineValue value;
            String error;
            if (!ResolveSourceValue(*node, ctx->assetsPath, value, error))
                co_return PipelineRunResult::Fail(error);

            co_return PipelineRunResult::Single(value);
        }
    };

    // Sound effect generation through ElevenLabs
    class SfxGenNode : public PipelineNodeBase
    {
    public:
        SfxGenNode()
        {
            mSchema.type = "sfxGen";
            mSchema.label = "AI sound effect";
            mSchema.category = PipelineNodeCategory::AI;
            mSchema.description = "Generate a sound effect from a text description via ElevenLabs. Text inputs are joined into the prompt. \"Seamless loop\" makes a clip whose end joins its start.";
            mSchema.inputs = { In("prompt", PipelinePortType::Text) };
            mSchema.outputs = { Out("out", PipelinePortType::Audio) };
            mSchema.addableInputs = { PipelinePortType::Text };
        }

        Coroutine<PipelineRunResult> Run(const Ref<PipelineExecContext>& ctx, const Map<String, PipelineValue>& inputs,
                                         const Ref<PipelineNode>& node) override
        {
            Vector<String> parts = TextInputs(inputs, *node);
            parts.Add(node->GetConfigString("extraPrompt", ""));
            String joined = JoinNonEmpty(parts, "\n");
            String prompt = PipelineUtils::ClampPromptChars(joined, ElevenLabsProvider::sfxMaxPromptChars);
            if (prompt.Length() < String(joined).Trimed(" \n\r\t").Length())
                ctx->Log("sfxGen: prompt trimmed to " + (String)prompt.Length() + " chars (ElevenLabs limit is " + (String)ElevenLabsProvider::sfxMaxPromptChars + ")");

            String durationRaw = node->GetConfigString("duration", "auto");
            float duration = durationRaw == "auto" ? -1.0f : PipelineUtils::ValueToNumber(*node->GetConfigValue("duration"), -1.0f);
            String influenceRaw = node->GetConfigString("promptInfluence", "0.3").Trimed();
            float influence = influenceRaw.IsEmpty() ? -1.0f : PipelineUtils::ValueToNumber(*node->GetConfigValue("promptInfluence"), -1.0f);

            AiBytesResult r = co_await ElevenLabsProvider::GenerateSoundEffect(ctx, ctx->settings.GetElevenLabsKey(), prompt,
                node->GetConfigString("model", ElevenLabsProvider::sfxModel), duration, node->GetConfigBool("loop", false), influence,
                node->GetConfigString("outputFormat", "mp3_44100_128"));
            co_return AudioResult(r);
        }
    };

    // Speech synthesis through Gemini TTS or ElevenLabs
    class TtsSpeechNode : public PipelineNodeBase
    {
    public:
        TtsSpeechNode()
        {
            mSchema.type = "ttsSpeech";
            mSchema.label = "AI voice (TTS)";
            mSchema.category = PipelineNodeCategory::AI;
            mSchema.description = "Speak text with Gemini TTS (30 built-in voices) or ElevenLabs (more expressive, own voices). Text inputs are joined in port order.";
            mSchema.inputs = { In("text", PipelinePortType::Text) };
            mSchema.outputs = { Out("out", PipelinePortType::Audio) };
            mSchema.addableInputs = { PipelinePortType::Text };
        }

        Coroutine<PipelineRunResult> Run(const Ref<PipelineExecContext>& ctx, const Map<String, PipelineValue>& inputs,
                                         const Ref<PipelineNode>& node) override
        {
            String text = JoinNonEmpty(TextInputs(inputs, *node), "\n");
            String provider = node->GetConfigString("provider", "gemini") == "elevenlabs" ? "elevenlabs" : "gemini";
            String voice = node->GetConfigString("voice", "").Trimed();
            ctx->Log("ttsSpeech: provider=" + provider + " chars=" + (String)text.Length() + " voice=" + (voice.IsEmpty() ? String("(default)") : voice));

            if (provider == "elevenlabs")
            {
                AiBytesResult r = co_await ElevenLabsProvider::GenerateSpeech(ctx, ctx->settings.GetElevenLabsKey(), text, voice,
                    node->GetConfigString("model", ElevenLabsProvider::defaultTtsModel), node->GetConfigString("outputFormat", "mp3_44100_128"));
                co_return AudioResult(r);
            }

            AiBytesResult r = co_await GeminiProvider::GenerateSpeech(ctx, ctx->settings.GetGeminiKey(),
                node->GetConfigString("model", GeminiProvider::defaultTtsModel), text, voice.IsEmpty() ? String("Kore") : voice,
                node->GetConfigString("styleInstructions", ""));
            co_return AudioResult(r);
        }
    };

    // Music clip generation through Google Lyria
    class MusicGenNode : public PipelineNodeBase
    {
    public:
        MusicGenNode()
        {
            mSchema.type = "musicGen";
            mSchema.label = "AI music";
            mSchema.category = PipelineNodeCategory::AI;
            mSchema.description = "Generate a music clip via Google Lyria 3 - menu themes, background tracks. Text inputs are joined into the prompt.";
            mSchema.inputs = { In("prompt", PipelinePortType::Text) };
            mSchema.outputs = { Out("out", PipelinePortType::Audio) };
            mSchema.addableInputs = { PipelinePortType::Text };
        }

        Coroutine<PipelineRunResult> Run(const Ref<PipelineExecContext>& ctx, const Map<String, PipelineValue>& inputs,
                                         const Ref<PipelineNode>& node) override
        {
            Vector<String> parts = TextInputs(inputs, *node);
            parts.Add(node->GetConfigString("extraPrompt", ""));
            if (node->GetConfigBool("instrumental", true))
                parts.Add("Instrumental only, no vocals, no lyrics.");
            String prompt = JoinNonEmpty(parts, "\n");
            String model = node->GetConfigString("model", GeminiProvider::defaultMusicModel);
            ctx->Log("musicGen: model=" + model + " promptChars=" + (String)prompt.Length());
            AiBytesResult r = co_await GeminiProvider::GenerateMusic(ctx, ctx->settings.GetGeminiKey(), model, prompt);
            co_return AudioResult(r);
        }
    };

    // Offline conversion, trimming, loudness and seamless loop of a clip via ffmpeg
    class AudioProcessNode : public PipelineNodeBase
    {
    public:
        AudioProcessNode()
        {
            mSchema.type = "audioProcess";
            mSchema.label = "Audio process";
            mSchema.category = PipelineNodeCategory::Transform;
            mSchema.description = "Prepare a clip for the engine: convert to wav/ogg/mp3, force mono, trim silence, normalise loudness and crossfade it into a seamless loop (needs ffmpeg).";
            mSchema.inputs = { In("in", PipelinePortType::Audio) };
            mSchema.outputs = { Out("out", PipelinePortType::Audio) };
        }

        Coroutine<PipelineRunResult> Run(const Ref<PipelineExecContext>& ctx, const Map<String, PipelineValue>& inputs,
                                         const Ref<PipelineNode>& node) override
        {
            auto value = Input(inputs, "in");
            if (!value || !value->IsAudio()) co_return PipelineRunResult::Fail("audioProcess: input \"in\" is not connected");

            PipelineAudio::ProcessOptions opt;
            String format = node->GetConfigString("format", "keep");
            opt.format = (format == "wav" || format == "ogg" || format == "mp3") ? format : String("keep");
            String rate = node->GetConfigString("sampleRate", "keep");
            opt.sampleRate = rate == "keep" ? 0 : (int)PipelineUtils::ValueToNumber(*node->GetConfigValue("sampleRate"), 44100);
            String channels = node->GetConfigString("channels", "keep");
            opt.channels = channels == "mono" ? 1 : channels == "stereo" ? 2 : 0;
            opt.trimSilence = node->GetConfigBool("trimSilence", false);
            opt.normalize = node->GetConfigBool("normalize", false);
            opt.loudnessTarget = node->GetConfigNumber("loudnessTarget", -14.0f);
            opt.seamlessLoop = node->GetConfigBool("seamlessLoop", false);
            opt.crossfadeMs = node->GetConfigNumber("crossfadeMs", 250.0f);
            opt.fadeInMs = node->GetConfigNumber("fadeInMs", 0.0f);
            opt.fadeOutMs = node->GetConfigNumber("fadeOutMs", 0.0f);

            auto result = PipelineAudio::Process(value->data, value->mimeType, opt);
            if (result.ffmpegMissing)
            {
                ctx->Log("audioProcess: ffmpeg unavailable - passing the audio through unchanged");
                co_return PipelineRunResult::Single(*value);
            }
            if (!result.ok) co_return PipelineRunResult::Fail("audioProcess: " + result.error);

            ctx->Log("audioProcess: " + (String)result.data.Length() + " bytes out (" + result.mimeType + ")");
            co_return PipelineRunResult::Single(PipelineValue::Bytes(PipelinePortType::Audio, result.data, result.mimeType));
        }
    };

    // Terminal node saving the clip as a sound asset
    class FinishAudioNode : public PipelineNodeBase
    {
    public:
        FinishAudioNode()
        {
            mSchema.type = "finishAudio";
            mSchema.label = "Finish - audio";
            mSchema.category = PipelineNodeCategory::Output;
            mSchema.description = "Terminal node. Press Play to evaluate the pipeline; the clip is saved as a SoundAsset in the Assets folder.";
            mSchema.inputs = { In("in", PipelinePortType::Audio) };
            mSchema.hasPlay = true;
        }

        Coroutine<PipelineRunResult> Run(const Ref<PipelineExecContext>& ctx, const Map<String, PipelineValue>& inputs,
                                         const Ref<PipelineNode>& node) override
        {
            auto value = Input(inputs, "in");
            if (!value || !value->IsAudio()) co_return PipelineRunResult::Fail("finishAudio: input \"in\" is not connected");
            if (WriteFinishAsset(ctx, *node, "Generated/output", value->GetExtension(), value->data).IsEmpty())
                co_return PipelineRunResult::Fail("finishAudio: failed to write the asset file");
            co_return PipelineRunResult::Single(*value, "result");
        }
    };

    // Video generation through Veo or Kling, chosen by the model name
    class VideoGenNode : public PipelineNodeBase
    {
    public:
        VideoGenNode()
        {
            mSchema.type = "videoGen";
            mSchema.label = "AI video gen";
            mSchema.category = PipelineNodeCategory::AI;
            mSchema.description = "Generate a video via Google Veo or Kling. Text inputs are joined into the prompt; image inputs are references. Add extra inputs of either kind with \"+\".";
            mSchema.inputs = { In("prompt", PipelinePortType::Text), In("reference", PipelinePortType::Image) };
            mSchema.outputs = { Out("out", PipelinePortType::Video) };
            mSchema.addableInputs = { PipelinePortType::Text, PipelinePortType::Image };
        }

        Coroutine<PipelineRunResult> Run(const Ref<PipelineExecContext>& ctx, const Map<String, PipelineValue>& inputs,
                                         const Ref<PipelineNode>& node) override
        {
            VideoGenerateInput in;
            in.model = node->GetConfigString("model", VeoProvider::defaultModel);
            Vector<String> parts = TextInputs(inputs, *node);
            parts.Add(node->GetConfigString("extraPrompt", ""));
            if (node->GetConfigBool("bgColorEnabled", false))
            {
                String bg = node->GetConfigString("bgColor", "").Trimed();
                if (!bg.IsEmpty())
                    parts.Add("The entire background must be a single solid, uniform color " + bg + ", flat and clean, filling every part of the frame behind the subject for the whole duration of the video. No gradients, patterns, shadows or scenery in the background.");
            }
            in.prompt = JoinNonEmpty(parts, "\n");
            in.references = ImageInputs(inputs, *node);
            in.aspectRatio = node->GetConfigString("aspectRatio", "16:9");
            in.durationSeconds = (int)node->GetConfigNumber("duration", 8);

            ctx->Log("videoGen: model=" + in.model + " promptChars=" + (String)in.prompt.Length() + " refs=" + (String)in.references.Count() +
                     " aspect=" + in.aspectRatio + " duration=" + (String)in.durationSeconds + "s");

            AiBytesResult r;
            if (in.model.ToLowerCase().StartsWith("kling"))
                r = co_await KlingProvider::GenerateVideo(ctx, ctx->settings.GetKlingAccessKey(), ctx->settings.GetKlingSecretKey(), in);
            else
                r = co_await VeoProvider::GenerateVideo(ctx, ctx->settings.GetGeminiKey(), in);

            if (!r.ok) co_return PipelineRunResult::Fail(r.error);
            co_return PipelineRunResult::Single(PipelineValue::Bytes(PipelinePortType::Video, r.data, "video/mp4"));
        }
    };

    // Terminal node saving the clip as a video asset
    class FinishVideoNode : public PipelineNodeBase
    {
    public:
        FinishVideoNode()
        {
            mSchema.type = "finishVideo";
            mSchema.label = "Finish - video";
            mSchema.category = PipelineNodeCategory::Output;
            mSchema.description = "Terminal node. Press Play to evaluate the pipeline; the video is saved as a VideoAsset in the Assets folder.";
            mSchema.inputs = { In("in", PipelinePortType::Video) };
            mSchema.hasPlay = true;
        }

        Coroutine<PipelineRunResult> Run(const Ref<PipelineExecContext>& ctx, const Map<String, PipelineValue>& inputs,
                                         const Ref<PipelineNode>& node) override
        {
            auto value = Input(inputs, "in");
            if (!value || !value->IsVideo()) co_return PipelineRunResult::Fail("finishVideo: input \"in\" is not connected");
            if (WriteFinishAsset(ctx, *node, "Generated/output", "mp4", value->data).IsEmpty())
                co_return PipelineRunResult::Fail("finishVideo: failed to write the asset file");
            co_return PipelineRunResult::Single(*value, "result");
        }
    };

    void RegisterAudioVideoNodes()
    {
        PipelineNodeRegistry::Register(mmake<SourceAudioNode>());
        PipelineNodeRegistry::Register(mmake<SfxGenNode>());
        PipelineNodeRegistry::Register(mmake<TtsSpeechNode>());
        PipelineNodeRegistry::Register(mmake<MusicGenNode>());
        PipelineNodeRegistry::Register(mmake<AudioProcessNode>());
        PipelineNodeRegistry::Register(mmake<FinishAudioNode>());
        PipelineNodeRegistry::Register(mmake<VideoGenNode>());
        PipelineNodeRegistry::Register(mmake<FinishVideoNode>());
    }
}
