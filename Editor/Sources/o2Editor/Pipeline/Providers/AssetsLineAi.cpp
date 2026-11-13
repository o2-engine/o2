#include "o2Editor/stdafx.h"
#include "AssetsLineAi.h"

#include "o2Editor/Pipeline/PipelineUtils.h"
#include "o2Editor/Pipeline/Providers/AiHttp.h"
#include "o2Editor/Pipeline/Sync/AssetsLineSync.h"

namespace Editor::AssetsLineAi
{
    const String outOfTokens = "Out of AssetsLine tokens";
    const String connectToRun = "Connect AssetsLine to run AI nodes";

    static const String basePath = "/api/sync";

    static ClientSource clientSource;

    void SetClientSource(const ClientSource& source)
    {
        clientSource = source;
    }

    bool GetClient(AssetsLineClient& client)
    {
        if (clientSource)
            return clientSource(client);

        if (PipelineUtils::HasWorkPathOverride() || !AssetsLineSync::IsSingletonInitialzed())
            return false;

        return o2AssetsLineSync.MakeAiClient(client);
    }

    bool IsConnected()
    {
        AssetsLineClient client;
        return GetClient(client);
    }

    static void AddMedia(DataValue& list, const Vector<AiImageRef>& images)
    {
        list.SetArray();
        for (auto& image : images)
        {
            auto& item = list.AddElement();
            item.SetObject();
            item["mimeType"] = image.mimeType.IsEmpty() ? String("image/png") : image.mimeType;
            item["data"] = PipelineUtils::Base64Encode(image.data);
        }
    }

    // The pipeline and the node the call runs for, so the billing history of AssetsLine names them
    static void AddContext(DataDocument& body, const Ref<PipelineExecContext>& ctx)
    {
        if (!ctx)
            return;

        String pipeline = ctx->pipelineName.IsEmpty() ? ctx->pipelineId : ctx->pipelineName;
        if (!pipeline.IsEmpty())
            body["pipeline"] = pipeline;

        if (!ctx->nodeId.IsEmpty())
        {
            auto& node = body["node"];
            node.SetObject();
            node["id"] = ctx->nodeId;
            node["type"] = ctx->nodeType;
        }
    }

    DataDocument TextBody(const Ref<PipelineExecContext>& ctx, const String& model, const String& prompt,
                          const Vector<AiImageRef>& images)
    {
        DataDocument body;
        body.SetObject();
        body["model"] = model;
        body["prompt"] = prompt;
        if (!images.IsEmpty())
            AddMedia(body["images"], images);

        AddContext(body, ctx);
        return body;
    }

    DataDocument ImageBody(const Ref<PipelineExecContext>& ctx, const String& model, const String& prompt,
                           const Vector<AiImageRef>& references, int seed, bool transparentBackground,
                           const AiImageOptions& options)
    {
        DataDocument body;
        body.SetObject();
        body["model"] = model;
        body["prompt"] = prompt;
        if (!references.IsEmpty())
            AddMedia(body["references"], references);

        if (seed >= 0)
            body["seed"] = seed;

        if (transparentBackground)
            body["transparentBackground"] = true;

        if (!options.renderSize.IsEmpty())
            body["renderSize"] = options.renderSize;

        if (!options.aspectRatio.IsEmpty())
            body["aspectRatio"] = options.aspectRatio;

        AddContext(body, ctx);
        return body;
    }

    DataDocument VideoBody(const Ref<PipelineExecContext>& ctx, const String& provider, const VideoGenerateInput& input)
    {
        DataDocument body;
        body.SetObject();
        body["provider"] = provider;
        body["model"] = input.model;
        body["prompt"] = input.prompt;
        if (!input.references.IsEmpty())
            AddMedia(body["references"], input.references);

        if (!input.aspectRatio.IsEmpty())
            body["aspectRatio"] = input.aspectRatio;

        if (input.durationSeconds > 0)
            body["durationSeconds"] = input.durationSeconds;

        AddContext(body, ctx);
        return body;
    }

    DataDocument SpeechBody(const Ref<PipelineExecContext>& ctx, const String& provider, const String& model,
                            const String& text, const String& voice, const String& styleInstructions)
    {
        DataDocument body;
        body.SetObject();
        body["provider"] = provider;
        body["text"] = text;
        body["voice"] = voice;
        if (!model.IsEmpty())
            body["model"] = model;

        if (!styleInstructions.IsEmpty())
            body["styleInstructions"] = styleInstructions;

        AddContext(body, ctx);
        return body;
    }

    DataDocument SfxBody(const Ref<PipelineExecContext>& ctx, const String& model, const String& text, float durationSeconds,
                         bool loop, float promptInfluence)
    {
        DataDocument body;
        body.SetObject();
        body["text"] = text;
        body["loop"] = loop;
        if (!model.IsEmpty())
            body["model"] = model;

        if (durationSeconds > 0.0f)
            body["durationSeconds"] = durationSeconds;

        if (promptInfluence >= 0.0f)
            body["promptInfluence"] = promptInfluence;

        AddContext(body, ctx);
        return body;
    }

    DataDocument MusicBody(const Ref<PipelineExecContext>& ctx, const String& model, const String& prompt)
    {
        DataDocument body;
        body.SetObject();
        body["prompt"] = prompt;
        if (!model.IsEmpty())
            body["model"] = model;

        AddContext(body, ctx);
        return body;
    }

    String ErrorMessage(int status, const String& code, const String& serverMessage)
    {
        String detail = serverMessage.Trimed(" \n\r\t");
        String suffix = detail.IsEmpty() ? String() : ": " + detail;
        switch (status)
        {
            case 401: return "AssetsLine rejected the access token: connect again in the AssetsLine settings";
            case 402: return outOfTokens + ": top up your balance to run AI nodes";
            case 403:
                if (code == "route_blocked")
                    return "AssetsLine: " + (detail.IsEmpty() ? String("every AI route is switched off in your AssetsLine settings") : detail);
                return "AssetsLine: " + (detail.IsEmpty() ? String("your role in this project cannot run AI calls") : detail);
            case 422: return "AssetsLine refused the request" + suffix;
            case 502: return "AssetsLine: the provider failed" + suffix;
            case 400: return "AssetsLine: bad request" + suffix;
            default: return "AssetsLine: HTTP " + (String)status + suffix;
        }
    }

    static long long HeaderNumber(const AssetsLineResponse& response, const String& name, long long def)
    {
        String value = response.GetHeader(name).Trimed(" \t");
        if (value.IsEmpty())
            return def;

        char* end = nullptr;
        double number = strtod(value.Data(), &end);
        return end == value.Data() ? def : (long long)number;
    }

    static String JsonString(const DataDocument& json, const char* key)
    {
        auto value = json.IsObject() ? json.FindMember(key) : nullptr;
        return value && value->IsString() ? String(value->GetString()) : String();
    }

    AssetsLineAiAnswer ReadAnswer(const AssetsLineResponse& response, const String& serverUrl)
    {
        AssetsLineAiAnswer answer;
        if (response.offline)
        {
            answer.error = "AssetsLine: cannot reach " + serverUrl + (response.error.IsEmpty() ? String() : " (" + response.error + ")");
            return answer;
        }

        answer.status = response.status;
        answer.body = response.body;
        String mime = response.GetHeader("Content-Type");
        int semicolon = mime.Find(";");
        answer.mimeType = (semicolon >= 0 ? mime.SubStr(0, semicolon) : mime).Trimed(" \t").ToLowerCase();
        answer.spent = HeaderNumber(response, "X-AssetsLine-Spent", 0);
        answer.balance = HeaderNumber(response, "X-AssetsLine-Balance", -1);
        answer.route = response.GetHeader("X-AssetsLine-Route");
        answer.seconds = (int)HeaderNumber(response, "X-AssetsLine-Seconds", 0);
        if (response.ok)
        {
            answer.ok = true;
            return answer;
        }

        answer.code = JsonString(response.json, "code");
        answer.topUpUrl = JsonString(response.json, "topUpUrl");
        answer.accountUrl = JsonString(response.json, "accountUrl");
        answer.error = ErrorMessage(response.status, answer.code, JsonString(response.json, "error"));
        if (!answer.topUpUrl.IsEmpty() && response.status == 402)
            answer.error += " (" + answer.topUpUrl + ")";
        else if (!answer.accountUrl.IsEmpty() && response.status == 403)
            answer.error += " (" + answer.accountUrl + ")";

        return answer;
    }

    static String FormatTokens(long long tokens)
    {
        String digits = std::to_string(tokens).c_str();
        String out;
        for (int i = 0; i < digits.Length(); i++)
        {
            if (i > 0 && (digits.Length() - i) % 3 == 0)
                out += " ";
            out += digits[i];
        }
        return out;
    }

    String RouteLine(const AssetsLineAiAnswer& answer)
    {
        String line = "route: AssetsLine - " + (answer.route.IsEmpty() ? String("your AssetsLine account") : answer.route);
        String notes;
        if (answer.spent > 0)
            notes = FormatTokens(answer.spent) + (answer.spent == 1 ? " token" : " tokens");

        if (answer.balance >= 0)
            notes += (notes.IsEmpty() ? String() : String(", ")) + FormatTokens(answer.balance) + " left";

        if (!notes.IsEmpty())
            line += " (" + notes + ")";

        return line;
    }

    Coroutine<AssetsLineAiAnswer> Call(const Ref<PipelineExecContext>& ctx, const String& kind, const DataDocument& body)
    {
        AssetsLineClient client;
        if (!GetClient(client))
        {
            AssetsLineAiAnswer answer;
            answer.error = connectToRun;
            co_return answer;
        }

        if (ctx && ctx->IsCancelled())
        {
            AssetsLineAiAnswer answer;
            answer.error = "Cancelled";
            co_return answer;
        }

        AssetsLineResponse response = co_await client.Request(HttpMethod::Post, basePath + "/ai/" + kind, body.SaveAsString(),
                                                              "application/json", 900.0f);
        if (ctx && ctx->IsCancelled())
        {
            AssetsLineAiAnswer answer;
            answer.error = "Cancelled";
            co_return answer;
        }

        AssetsLineAiAnswer answer = ReadAnswer(response, client.serverUrl);
        if (answer.ok && ctx)
            ctx->Log(RouteLine(answer));

        co_return answer;
    }

    Coroutine<AiTextResult> GenerateText(const Ref<PipelineExecContext>& ctx, const String& model, const String& prompt,
                                         const Vector<AiImageRef>& images)
    {
        AiTextResult result;
        String id = model.IsEmpty() ? GeminiProvider::defaultTextModel : model;
        AssetsLineAiAnswer answer = co_await Call(ctx, "text", TextBody(ctx, id, prompt, images));
        if (!answer.ok)
        {
            result.error = answer.error;
            co_return result;
        }

        DataDocument json;
        if (!json.LoadFromData(answer.body) || !json.IsObject())
        {
            result.error = "AssetsLine returned no text (model " + id + ")";
            co_return result;
        }

        result.text = JsonString(json, "text").Trimed(" \n\r\t");
        result.ok = true;
        co_return result;
    }

    // A media answer as the nodes take it
    static AiBytesResult Bytes(const AssetsLineAiAnswer& answer, const String& fallbackMime, const String& what)
    {
        AiBytesResult result;
        if (!answer.ok)
        {
            result.error = answer.error;
            return result;
        }

        if (answer.body.IsEmpty())
        {
            result.error = "AssetsLine returned no " + what;
            return result;
        }

        result.ok = true;
        result.data = answer.body;
        result.mimeType = answer.mimeType.IsEmpty() || answer.mimeType == "application/octet-stream" ? fallbackMime : answer.mimeType;
        result.seconds = answer.seconds;
        return result;
    }

    Coroutine<AiBytesResult> GenerateImage(const Ref<PipelineExecContext>& ctx, const String& model, const String& prompt,
                                           const Vector<AiImageRef>& references, int seed, bool transparentBackground,
                                           const AiImageOptions& options)
    {
        AssetsLineAiAnswer answer = co_await Call(ctx, "image", ImageBody(ctx, model, prompt, references, seed, transparentBackground, options));
        co_return Bytes(answer, "image/png", "image");
    }

    Coroutine<AiBytesResult> GenerateVideo(const Ref<PipelineExecContext>& ctx, const String& provider, const VideoGenerateInput& input)
    {
        AssetsLineAiAnswer answer = co_await Call(ctx, "video", VideoBody(ctx, provider, input));
        co_return Bytes(answer, "video/mp4", "video");
    }

    Coroutine<AiBytesResult> GenerateSpeech(const Ref<PipelineExecContext>& ctx, const String& provider, const String& model,
                                            const String& text, const String& voice, const String& styleInstructions)
    {
        AssetsLineAiAnswer answer = co_await Call(ctx, "speech", SpeechBody(ctx, provider, model, text, voice, styleInstructions));
        co_return Bytes(answer, "audio/wav", "audio");
    }

    Coroutine<AiBytesResult> GenerateSoundEffect(const Ref<PipelineExecContext>& ctx, const String& model, const String& text,
                                                 float durationSeconds, bool loop, float promptInfluence)
    {
        AssetsLineAiAnswer answer = co_await Call(ctx, "sfx", SfxBody(ctx, model, text, durationSeconds, loop, promptInfluence));
        co_return Bytes(answer, "audio/mpeg", "audio");
    }

    Coroutine<AiBytesResult> GenerateMusic(const Ref<PipelineExecContext>& ctx, const String& model, const String& prompt)
    {
        AssetsLineAiAnswer answer = co_await Call(ctx, "music", MusicBody(ctx, model, prompt));
        co_return Bytes(answer, "audio/mpeg", "audio");
    }
}
