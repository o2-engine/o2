#include "o2Editor/stdafx.h"
#include "GeminiProvider.h"

#include "o2Editor/Pipeline/PipelineAudio.h"
#include "o2Editor/Pipeline/PipelineUtils.h"

namespace Editor::GeminiProvider
{
    static const String baseUrl = "https://generativelanguage.googleapis.com/v1beta";

    const Vector<String> voices = {
        "Zephyr", "Puck", "Charon", "Kore", "Fenrir", "Leda", "Orus", "Aoede",
        "Callirrhoe", "Autonoe", "Enceladus", "Iapetus", "Umbriel", "Algieba",
        "Despina", "Erinome", "Algenib", "Rasalgethi", "Laomedeia", "Achernar",
        "Alnilam", "Schedar", "Gacrux", "Pulcherrima", "Achird", "Zubenelgenubi",
        "Vindemiatrix", "Sadachbia", "Sadaltager", "Sulafat"
    };

    static Map<String, String> KeyHeaders(const String& apiKey)
    {
        Map<String, String> headers;
        headers["x-goog-api-key"] = apiKey;
        return headers;
    }

    static String MissingKey()
    {
        return "Gemini API key is not configured (Pipeline settings)";
    }

    static void AddInlineImage(DataValue& parts, const AiImageRef& image)
    {
        auto& part = parts.AddElement();
        part.SetObject();
        auto& inline_ = part["inlineData"];
        inline_.SetObject();
        inline_["mimeType"] = image.mimeType.IsEmpty() ? String("image/png") : image.mimeType;
        inline_["data"] = PipelineUtils::Base64Encode(image.data);
    }

    Coroutine<AiTextResult> GenerateText(const Ref<PipelineExecContext>& ctx, const String& apiKey, const String& modelIn,
                                         const String& prompt, const Vector<AiImageRef>& images)
    {
        AiTextResult result;
        if (apiKey.IsEmpty()) { result.error = MissingKey(); co_return result; }

        String model = modelIn.IsEmpty() ? defaultTextModel : modelIn;

        DataDocument body;
        body.SetObject();
        auto& contents = body["contents"];
        contents.SetArray();
        auto& content = contents.AddElement();
        content.SetObject();
        content["role"] = String("user");
        auto& parts = content["parts"];
        parts.SetArray();
        auto& textPart = parts.AddElement();
        textPart.SetObject();
        textPart["text"] = prompt;
        for (auto& image : images)
            AddInlineImage(parts, image);

        auto request = AiHttp::MakeJsonPost(baseUrl + "/models/" + model + ":generateContent", body, KeyHeaders(apiKey), 300.0f);
        AiHttpResult http = co_await AiHttp::Send(ctx, request, "Gemini text call");
        if (!http.ok)
        {
            result.error = AiHttp::DescribeError(http, "Gemini text call", model);
            co_return result;
        }

        String out;
        auto candidates = AiHttp::Member(&http.json, "candidates");
        auto first = AiHttp::Element(candidates, 0);
        auto outParts = AiHttp::Member(AiHttp::Member(first, "content"), "parts");
        if (outParts && outParts->IsArray())
        {
            for (auto& part : *outParts)
                out += AiHttp::StringOf(AiHttp::Member(&part, "text"));
        }

        out = out.Trimed(" \n\r\t");
        if (out.IsEmpty())
        {
            String reason = AiHttp::StringOf(AiHttp::Member(first, "finishReason"));
            result.error = "Gemini returned no text (model " + model + ")" + (reason.IsEmpty() ? String() : ": " + reason) +
                " - " + http.body.SubStr(0, Math::Min(300, http.body.Length()));
            co_return result;
        }

        result.ok = true;
        result.text = out;
        co_return result;
    }

    static Coroutine<AiBytesResult> ImagenGenerate(const Ref<PipelineExecContext>& ctx, const String& apiKey, const String& model,
                                                   const String& prompt, int seed)
    {
        AiBytesResult result;
        DataDocument body;
        body.SetObject();
        auto& instances = body["instances"];
        instances.SetArray();
        auto& instance = instances.AddElement();
        instance.SetObject();
        instance["prompt"] = prompt;
        auto& params = body["parameters"];
        params.SetObject();
        params["sampleCount"] = 1;
        params["aspectRatio"] = String("1:1");
        if (seed >= 0)
            params["seed"] = seed;

        auto request = AiHttp::MakeJsonPost(baseUrl + "/models/" + model + ":predict", body, KeyHeaders(apiKey), 300.0f);
        AiHttpResult http = co_await AiHttp::Send(ctx, request, "Imagen call");
        if (!http.ok)
        {
            result.error = AiHttp::DescribeError(http, "Imagen call", model);
            co_return result;
        }

        auto predictions = AiHttp::Member(&http.json, "predictions");
        auto first = AiHttp::Element(predictions, 0);
        String b64 = AiHttp::StringOf(AiHttp::Member(first, "bytesBase64Encoded"));
        if (b64.IsEmpty())
        {
            result.error = "Imagen returned no image data (model " + model + ")";
            co_return result;
        }

        result.ok = true;
        result.data = PipelineUtils::Base64Decode(b64);
        result.mimeType = AiHttp::StringOf(AiHttp::Member(first, "mimeType"), "image/png");
        co_return result;
    }

    Coroutine<AiBytesResult> GenerateImage(const Ref<PipelineExecContext>& ctx, const String& apiKey, const String& modelIn,
                                           const String& prompt, const Vector<AiImageRef>& references, int seed)
    {
        AiBytesResult result;
        if (apiKey.IsEmpty()) { result.error = MissingKey(); co_return result; }

        String model = modelIn.IsEmpty() ? defaultImageModel : modelIn;
        if (model.ToLowerCase().StartsWith("imagen"))
        {
            result = co_await ImagenGenerate(ctx, apiKey, model, prompt, seed);
            co_return result;
        }

        DataDocument body;
        body.SetObject();
        auto& contents = body["contents"];
        contents.SetArray();
        auto& content = contents.AddElement();
        content.SetObject();
        content["role"] = String("user");
        auto& parts = content["parts"];
        parts.SetArray();
        if (!prompt.IsEmpty() || references.IsEmpty())
        {
            auto& textPart = parts.AddElement();
            textPart.SetObject();
            textPart["text"] = prompt;
        }
        for (auto& ref : references)
            AddInlineImage(parts, ref);

        auto& config = body["generationConfig"];
        config.SetObject();
        auto& modalities = config["responseModalities"];
        modalities.SetArray();
        modalities.AddElement() = String("IMAGE");
        modalities.AddElement() = String("TEXT");
        if (seed >= 0)
            config["seed"] = seed;

        auto request = AiHttp::MakeJsonPost(baseUrl + "/models/" + model + ":generateContent", body, KeyHeaders(apiKey), 300.0f);
        AiHttpResult http = co_await AiHttp::Send(ctx, request, "Gemini image call");
        if (!http.ok)
        {
            result.error = AiHttp::DescribeError(http, "Gemini image call", model);
            co_return result;
        }

        auto candidates = AiHttp::Member(&http.json, "candidates");
        auto first = AiHttp::Element(candidates, 0);
        auto outParts = AiHttp::Member(AiHttp::Member(first, "content"), "parts");
        String texts;
        if (outParts && outParts->IsArray())
        {
            for (auto& part : *outParts)
            {
                auto inlineData = AiHttp::Member(&part, "inlineData");
                if (!inlineData)
                    inlineData = AiHttp::Member(&part, "inline_data");

                String b64 = AiHttp::StringOf(AiHttp::Member(inlineData, "data"));
                if (!b64.IsEmpty())
                {
                    result.ok = true;
                    result.data = PipelineUtils::Base64Decode(b64);
                    result.mimeType = AiHttp::StringOf(AiHttp::Member(inlineData, "mimeType"), "image/png");
                    co_return result;
                }

                texts += AiHttp::StringOf(AiHttp::Member(&part, "text"));
            }
        }

        if (!texts.IsEmpty())
            result.error = "Gemini returned text instead of an image (model " + model + "): " + texts.SubStr(0, Math::Min(300, texts.Length()));
        else
        {
            String reason = AiHttp::StringOf(AiHttp::Member(first, "finishReason"));
            result.error = "Gemini returned no image data (model " + model + ")" + (reason.IsEmpty() ? String() : ": " + reason);
        }
        co_return result;
    }

    Coroutine<AiBytesResult> GenerateSpeech(const Ref<PipelineExecContext>& ctx, const String& apiKey, const String& modelIn,
                                            const String& textIn, const String& voiceIn, const String& styleInstructions)
    {
        AiBytesResult result;
        if (apiKey.IsEmpty()) { result.error = MissingKey(); co_return result; }

        String text = textIn.Trimed(" \n\r\t");
        if (text.IsEmpty()) { result.error = "ttsSpeech: the text is empty - connect a text input"; co_return result; }

        String model = modelIn.Trimed().IsEmpty() ? defaultTtsModel : modelIn.Trimed();
        String voice = voiceIn.Trimed().IsEmpty() ? String("Kore") : voiceIn.Trimed();
        String style = styleInstructions.Trimed(" \n\r\t");
        String prompt = style.IsEmpty() ? text : style + ": " + text;

        DataDocument body;
        body.SetObject();
        auto& contents = body["contents"];
        contents.SetArray();
        auto& content = contents.AddElement();
        content.SetObject();
        auto& parts = content["parts"];
        parts.SetArray();
        auto& textPart = parts.AddElement();
        textPart.SetObject();
        textPart["text"] = prompt;
        auto& config = body["generationConfig"];
        config.SetObject();
        auto& modalities = config["responseModalities"];
        modalities.SetArray();
        modalities.AddElement() = String("AUDIO");
        auto& speech = config["speechConfig"];
        speech.SetObject();
        auto& voiceConfig = speech["voiceConfig"];
        voiceConfig.SetObject();
        auto& prebuilt = voiceConfig["prebuiltVoiceConfig"];
        prebuilt.SetObject();
        prebuilt["voiceName"] = voice;

        auto request = AiHttp::MakeJsonPost(baseUrl + "/models/" + model + ":generateContent", body, KeyHeaders(apiKey), 300.0f);
        AiHttpResult http = co_await AiHttp::Send(ctx, request, "Gemini speech");
        if (!http.ok)
        {
            result.error = AiHttp::DescribeError(http, "Gemini speech", model);
            co_return result;
        }

        auto candidates = AiHttp::Member(&http.json, "candidates");
        auto first = AiHttp::Element(candidates, 0);
        auto outParts = AiHttp::Member(AiHttp::Member(first, "content"), "parts");
        if (outParts && outParts->IsArray())
        {
            for (auto& part : *outParts)
            {
                auto inlineData = AiHttp::Member(&part, "inlineData");
                String b64 = AiHttp::StringOf(AiHttp::Member(inlineData, "data"));
                if (b64.IsEmpty())
                    continue;

                String raw = PipelineUtils::Base64Decode(b64);
                String mime = AiHttp::StringOf(AiHttp::Member(inlineData, "mimeType"));
                bool isPcm = mime.ToLowerCase().Contains("l16") || mime.ToLowerCase().Contains("pcm");
                if (isPcm)
                {
                    result.data = PipelineAudio::PcmToWav(raw, PipelineAudio::PcmRateFromMime(mime), 1);
                    result.mimeType = "audio/wav";
                }
                else
                {
                    result.data = raw;
                    result.mimeType = mime.IsEmpty() ? String("audio/wav") : mime;
                }
                result.ok = true;
                co_return result;
            }
        }

        String reason = AiHttp::StringOf(AiHttp::Member(first, "finishReason"), "no audio in response");
        result.error = "Gemini TTS returned no audio (model " + model + "): " + reason;
        co_return result;
    }

    Coroutine<AiBytesResult> GenerateMusic(const Ref<PipelineExecContext>& ctx, const String& apiKey, const String& modelIn,
                                           const String& promptIn)
    {
        AiBytesResult result;
        if (apiKey.IsEmpty()) { result.error = MissingKey(); co_return result; }

        String prompt = promptIn.Trimed(" \n\r\t");
        if (prompt.IsEmpty()) { result.error = "musicGen: the prompt is empty - connect a text input or type one in the node"; co_return result; }

        String model = modelIn.Trimed().IsEmpty() ? defaultMusicModel : modelIn.Trimed();

        DataDocument body;
        body.SetObject();
        body["model"] = model;
        body["input"] = prompt;

        auto request = AiHttp::MakeJsonPost(baseUrl + "/interactions", body, KeyHeaders(apiKey), 600.0f);
        AiHttpResult http = co_await AiHttp::Send(ctx, request, "Lyria music");
        if (!http.ok)
        {
            result.error = AiHttp::DescribeError(http, "Lyria music", model);
            co_return result;
        }

        auto steps = AiHttp::Member(&http.json, "steps");
        if (steps && steps->IsArray())
        {
            for (auto& step : *steps)
            {
                auto content = AiHttp::Member(&step, "content");
                if (!content || !content->IsArray())
                    continue;

                for (auto& block : *content)
                {
                    if (AiHttp::StringOf(AiHttp::Member(&block, "type")) != "audio")
                        continue;

                    String b64 = AiHttp::StringOf(AiHttp::Member(&block, "data"));
                    if (b64.IsEmpty())
                        continue;

                    result.ok = true;
                    result.data = PipelineUtils::Base64Decode(b64);
                    result.mimeType = AiHttp::StringOf(AiHttp::Member(&block, "mime_type"), "audio/mpeg");
                    co_return result;
                }
            }
        }

        // Some responses put the outputs at the top level
        auto outputs = AiHttp::Member(&http.json, "outputs");
        if (outputs && outputs->IsArray())
        {
            for (auto& block : *outputs)
            {
                String b64 = AiHttp::StringOf(AiHttp::Member(&block, "data"));
                if (b64.IsEmpty())
                    continue;

                result.ok = true;
                result.data = PipelineUtils::Base64Decode(b64);
                result.mimeType = AiHttp::StringOf(AiHttp::Member(&block, "mime_type"), "audio/mpeg");
                co_return result;
            }
        }

        String status = AiHttp::StringOf(AiHttp::Member(&http.json, "status"), "unknown");
        result.error = "Lyria returned no audio (model " + model + ", status " + status + ")";
        co_return result;
    }

    Coroutine<GeminiModelsResult> ListModels(const String& apiKey)
    {
        GeminiModelsResult result;
        if (apiKey.IsEmpty()) { result.error = MissingKey(); co_return result; }

        String pageToken;
        do
        {
            String url = baseUrl + "/models?pageSize=1000";
            if (!pageToken.IsEmpty())
                url += "&pageToken=" + PipelineUtils::UrlEncode(pageToken);

            auto request = AiHttp::MakeGet(url, KeyHeaders(apiKey), 60.0f);
            AiHttpResult http = co_await AiHttp::Send(nullptr, request, "ListModels");
            if (!http.ok)
            {
                result.error = AiHttp::DescribeError(http, "ListModels", "");
                co_return result;
            }

            auto models = AiHttp::Member(&http.json, "models");
            if (models && models->IsArray())
            {
                for (auto& m : *models)
                {
                    GeminiModelInfo info;
                    info.name = AiHttp::StringOf(AiHttp::Member(&m, "name"));
                    if (info.name.StartsWith("models/"))
                        info.name = info.name.SubStr(7);
                    info.displayName = AiHttp::StringOf(AiHttp::Member(&m, "displayName"), info.name);
                    auto methods = AiHttp::Member(&m, "supportedGenerationMethods");
                    if (methods && methods->IsArray())
                    {
                        for (auto& method : *methods)
                            info.methods.Add(AiHttp::StringOf(&method));
                    }
                    result.models.Add(info);
                }
            }

            pageToken = AiHttp::StringOf(AiHttp::Member(&http.json, "nextPageToken"));
        }
        while (!pageToken.IsEmpty());

        result.ok = true;
        co_return result;
    }
}
