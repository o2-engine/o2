#include "o2Editor/stdafx.h"
#include "ElevenLabsProvider.h"

#include "o2Editor/Pipeline/PipelineAudio.h"
#include "o2Editor/Pipeline/PipelineUtils.h"

namespace Editor::ElevenLabsProvider
{
    static const String baseUrl = "https://api.elevenlabs.io/v1";

    static Map<String, String> KeyHeaders(const String& apiKey)
    {
        Map<String, String> headers;
        headers["xi-api-key"] = apiKey;
        return headers;
    }

    String MimeForOutputFormat(const String& outputFormat)
    {
        if (outputFormat.StartsWith("pcm")) return "audio/wav";
        if (outputFormat.StartsWith("opus")) return "audio/ogg";
        if (outputFormat.StartsWith("ulaw") || outputFormat.StartsWith("alaw")) return "audio/wav";
        return "audio/mpeg";
    }

    static AiBytesResult FinishAudio(const AiHttpResult& http, const String& outputFormat, const String& opName, const String& model)
    {
        AiBytesResult result;
        if (!http.ok)
        {
            result.error = AiHttp::DescribeError(http, opName, model);
            return result;
        }

        result.data = http.body;
        result.mimeType = MimeForOutputFormat(outputFormat);
        if (outputFormat.StartsWith("pcm"))
        {
            int rate = atoi(outputFormat.SubStr(4).Data());
            result.data = PipelineAudio::PcmToWav(http.body, rate > 0 ? rate : 44100, 1);
        }
        result.ok = !result.data.IsEmpty();
        if (!result.ok)
            result.error = opName + " returned an empty clip";
        return result;
    }

    Coroutine<AiBytesResult> GenerateSoundEffect(const Ref<PipelineExecContext>& ctx, const String& apiKey, const String& textIn,
                                                 const String& modelIn, float durationSeconds, bool loop,
                                                 float promptInfluence, const String& outputFormatIn)
    {
        AiBytesResult result;
        if (apiKey.IsEmpty()) { result.error = "ElevenLabs API key is not configured (Pipeline settings)"; co_return result; }

        String text = PipelineUtils::ClampPromptChars(textIn, sfxMaxPromptChars);
        if (text.IsEmpty()) { result.error = "sfxGen: the prompt is empty - connect a text input"; co_return result; }

        String model = modelIn.Trimed().IsEmpty() ? sfxModel : modelIn.Trimed();
        String outputFormat = outputFormatIn.IsEmpty() ? String("mp3_44100_128") : outputFormatIn;

        DataDocument body;
        body.SetObject();
        body["text"] = text;
        body["model_id"] = model;
        if (durationSeconds > 0)
            body["duration_seconds"] = Math::Clamp(durationSeconds, sfxMinSeconds, sfxMaxSeconds);
        if (loop)
            body["loop"] = true;
        if (promptInfluence >= 0)
            body["prompt_influence"] = Math::Clamp(promptInfluence, 0.0f, 1.0f);

        auto request = AiHttp::MakeJsonPost(baseUrl + "/sound-generation?output_format=" + outputFormat, body, KeyHeaders(apiKey), 300.0f);
        AiHttpResult http = co_await AiHttp::Send(ctx, request, "ElevenLabs sound effect", false);
        if (!http.ok)
            http.jsonParsed = http.json.LoadFromData(http.body);

        co_return FinishAudio(http, outputFormat, "ElevenLabs sound effect", model);
    }

    Coroutine<AiBytesResult> GenerateSpeech(const Ref<PipelineExecContext>& ctx, const String& apiKey, const String& textIn,
                                            const String& voiceIdIn, const String& modelIn, const String& outputFormatIn)
    {
        AiBytesResult result;
        if (apiKey.IsEmpty()) { result.error = "ElevenLabs API key is not configured (Pipeline settings)"; co_return result; }

        String text = textIn.Trimed(" \n\r\t");
        if (text.IsEmpty()) { result.error = "ttsSpeech: the text is empty - connect a text input"; co_return result; }

        String voiceId = voiceIdIn.Trimed().IsEmpty() ? String("21m00Tcm4TlvDq8ikWAM") : voiceIdIn.Trimed();
        String model = modelIn.Trimed().IsEmpty() ? defaultTtsModel : modelIn.Trimed();
        String outputFormat = outputFormatIn.IsEmpty() ? String("mp3_44100_128") : outputFormatIn;

        DataDocument body;
        body.SetObject();
        body["text"] = text;
        body["model_id"] = model;

        auto request = AiHttp::MakeJsonPost(baseUrl + "/text-to-speech/" + voiceId + "?output_format=" + outputFormat, body, KeyHeaders(apiKey), 300.0f);
        AiHttpResult http = co_await AiHttp::Send(ctx, request, "ElevenLabs speech", false);
        if (!http.ok)
            http.jsonParsed = http.json.LoadFromData(http.body);

        co_return FinishAudio(http, outputFormat, "ElevenLabs speech", model);
    }

    Coroutine<ElevenVoicesResult> ListVoices(const String& apiKey)
    {
        ElevenVoicesResult result;
        if (apiKey.IsEmpty()) { result.error = "ElevenLabs API key is not configured"; co_return result; }

        auto request = AiHttp::MakeGet(baseUrl + "/voices", KeyHeaders(apiKey), 60.0f);
        AiHttpResult http = co_await AiHttp::Send(nullptr, request, "ElevenLabs voices");
        if (!http.ok)
        {
            result.error = AiHttp::DescribeError(http, "ElevenLabs voices", "");
            co_return result;
        }

        auto voices = AiHttp::Member(&http.json, "voices");
        if (voices && voices->IsArray())
        {
            for (auto& v : *voices)
            {
                ElevenVoiceInfo info;
                info.voiceId = AiHttp::StringOf(AiHttp::Member(&v, "voice_id"));
                info.name = AiHttp::StringOf(AiHttp::Member(&v, "name"));
                info.category = AiHttp::StringOf(AiHttp::Member(&v, "category"));
                if (!info.voiceId.IsEmpty())
                    result.voices.Add(info);
            }
        }

        result.ok = true;
        co_return result;
    }
}
