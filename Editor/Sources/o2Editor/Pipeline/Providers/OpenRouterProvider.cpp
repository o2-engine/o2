#include "o2Editor/stdafx.h"
#include "OpenRouterProvider.h"

#include "o2Editor/Pipeline/PipelineUpscale.h"
#include "o2Editor/Pipeline/PipelineUtils.h"

namespace Editor::OpenRouterProvider
{
    static Map<String, String> KeyHeaders(const String& apiKey)
    {
        Map<String, String> headers;
        headers["Authorization"] = "Bearer " + apiKey;
        headers["HTTP-Referer"] = referer;
        headers["X-Title"] = title;
        return headers;
    }

    static String MissingKey()
    {
        return "OpenRouter API key is not configured (Pipeline settings)";
    }

    static String StatusHint(int status)
    {
        if (status == 401) return " - the OpenRouter key was rejected, check it in the pipeline settings";
        if (status == 402) return " - no credits left, top up at openrouter.ai/settings/credits";
        if (status == 404) return " - no such model on OpenRouter";
        return "";
    }

    static String MimeOfDataUrl(const String& url)
    {
        if (!url.StartsWith("data:"))
            return "image/png";

        int end = url.Find(";");
        int comma = url.Find(",");
        if (end < 0 || (comma >= 0 && comma < end))
            end = comma;

        String mime = end > 5 ? url.SubStr(5, end) : String();
        return mime.IsEmpty() ? String("image/png") : mime;
    }

    static String CostNote(const DataValue& json)
    {
        String cost = AiHttp::StringOf(AiHttp::Member(AiHttp::Member(&json, "usage"), "cost"));
        return cost.IsEmpty() ? String() : ", cost $" + cost;
    }

    static const DataValue* FirstMessage(const DataValue& json)
    {
        return AiHttp::Member(AiHttp::Element(AiHttp::Member(&json, "choices"), 0), "message");
    }

    static String TextOfContent(const DataValue* content)
    {
        if (!content)
            return "";

        if (content->IsString())
            return content->GetString();

        String text;
        if (content->IsArray())
        {
            for (auto& part : *content)
            {
                if (AiHttp::StringOf(AiHttp::Member(&part, "type"), "text") == "text")
                    text += AiHttp::StringOf(AiHttp::Member(&part, "text"));
            }
        }
        return text;
    }

    static String ImageUrlOf(const DataValue* holder)
    {
        return AiHttp::StringOf(AiHttp::Member(AiHttp::Member(holder, "image_url"), "url"));
    }

    static String FirstImageUrl(const DataValue* message)
    {
        auto images = AiHttp::Member(message, "images");
        if (images && images->IsArray())
        {
            for (auto& image : *images)
            {
                String url = ImageUrlOf(&image);
                if (!url.IsEmpty())
                    return url;
            }
        }

        auto content = AiHttp::Member(message, "content");
        if (content && content->IsArray())
        {
            for (auto& part : *content)
            {
                String url = ImageUrlOf(&part);
                if (!url.IsEmpty())
                    return url;
            }
        }
        return "";
    }

    void BuildRequestBody(DataDocument& body, const String& model, const String& prompt, const Vector<AiImageRef>& images,
                          bool withImage, int seed, const AiImageOptions& options /*= AiImageOptions()*/)
    {
        body.SetObject();
        body["model"] = model;
        auto& messages = body["messages"];
        messages.SetArray();
        auto& message = messages.AddElement();
        message.SetObject();
        message["role"] = String("user");
        if (images.IsEmpty())
            message["content"] = prompt;
        else
        {
            auto& parts = message["content"];
            parts.SetArray();
            auto& textPart = parts.AddElement();
            textPart.SetObject();
            textPart["type"] = String("text");
            textPart["text"] = prompt;
            for (auto& image : images)
            {
                auto& part = parts.AddElement();
                part.SetObject();
                part["type"] = String("image_url");
                auto& url = part["image_url"];
                url.SetObject();
                url["url"] = PipelineUtils::BytesToDataUrl(image.data, image.mimeType.IsEmpty() ? String("image/png") : image.mimeType);
            }
        }

        if (withImage)
        {
            auto& modalities = body["modalities"];
            modalities.SetArray();
            modalities.AddElement() = String("image");
            modalities.AddElement() = String("text");
            if (seed >= 0)
                body["seed"] = seed;

            bool size = !options.renderSize.IsEmpty() && PipelineUpscale::RendersLargeSizes(model);
            if (size || !options.aspectRatio.IsEmpty())
            {
                auto& imageConfig = body["image_config"];
                imageConfig.SetObject();
                if (!options.aspectRatio.IsEmpty())
                    imageConfig["aspect_ratio"] = options.aspectRatio;
                if (size)
                    imageConfig["image_size"] = options.renderSize;
            }
        }

        auto& usage = body["usage"];
        usage.SetObject();
        usage["include"] = true;
    }

    void BuildImagesBody(DataDocument& body, const String& model, const String& prompt, const Vector<AiImageRef>& references,
                         bool transparentBackground)
    {
        body.SetObject();
        body["model"] = model;
        body["prompt"] = prompt.Trimed(" \n\r\t").IsEmpty() ? String("Generate the image.") : prompt;
        body["n"] = 1;
        if (!references.IsEmpty())
        {
            auto& list = body["input_references"];
            list.SetArray();
            for (auto& image : references)
            {
                auto& item = list.AddElement();
                item.SetObject();
                item["type"] = String("image_url");
                auto& url = item["image_url"];
                url.SetObject();
                url["url"] = PipelineUtils::BytesToDataUrl(image.data, image.mimeType.IsEmpty() ? String("image/png") : image.mimeType);
            }
        }

        if (transparentBackground)
        {
            body["background"] = String("transparent");
            body["output_format"] = String("png");
        }
    }

    String ParseError(const DataValue& json)
    {
        auto err = AiHttp::Member(&json, "error");
        if (!err)
            return "";

        if (err->IsString())
            return err->GetString();

        if (!err->IsObject())
            return "";

        String message = AiHttp::StringOf(AiHttp::Member(err, "message"), "unknown error");
        String code = AiHttp::StringOf(AiHttp::Member(err, "code"));
        return code.IsEmpty() ? message : message + " (code " + code + ")";
    }

    AiTextResult ParseTextResponse(const DataValue& json, const String& model)
    {
        AiTextResult result;
        String error = ParseError(json);
        if (!error.IsEmpty())
        {
            result.error = "OpenRouter text call failed (model " + model + "): " + error;
            return result;
        }

        auto first = AiHttp::Element(AiHttp::Member(&json, "choices"), 0);
        String out = TextOfContent(AiHttp::Member(FirstMessage(json), "content")).Trimed(" \n\r\t");
        if (out.IsEmpty())
        {
            String reason = AiHttp::StringOf(AiHttp::Member(first, "finish_reason"));
            result.error = "OpenRouter returned no text (model " + model + ")" + (reason.IsEmpty() ? String() : ": " + reason);
            return result;
        }

        result.ok = true;
        result.text = out;
        return result;
    }

    AiBytesResult ParseImageResponse(const DataValue& json, const String& model)
    {
        AiBytesResult result;
        String error = ParseError(json);
        if (!error.IsEmpty())
        {
            result.error = "OpenRouter image call failed (model " + model + "): " + error;
            return result;
        }

        auto message = FirstMessage(json);
        String url = FirstImageUrl(message);
        String bytes = PipelineUtils::DataUrlToBytes(url);
        if (!bytes.IsEmpty())
        {
            result.ok = true;
            result.data = bytes;
            result.mimeType = MimeOfDataUrl(url.Trimed());
            return result;
        }

        String texts = TextOfContent(AiHttp::Member(message, "content")).Trimed(" \n\r\t");
        if (!texts.IsEmpty())
            result.error = "OpenRouter returned text instead of an image (model " + model + "): " + texts.SubStr(0, Math::Min(300, texts.Length()));
        else
            result.error = "OpenRouter returned no image data (model " + model + ")";
        return result;
    }

    AiBytesResult ParseImagesResponse(const DataValue& json, const String& model)
    {
        AiBytesResult result;
        String error = ParseError(json);
        if (!error.IsEmpty())
        {
            result.error = "OpenRouter image call failed (model " + model + "): " + error;
            return result;
        }

        auto first = AiHttp::Element(AiHttp::Member(&json, "data"), 0);
        String bytes = PipelineUtils::Base64Decode(AiHttp::StringOf(AiHttp::Member(first, "b64_json")));
        if (bytes.IsEmpty())
        {
            result.error = "OpenRouter returned no image data (model " + model + ")";
            return result;
        }

        result.ok = true;
        result.data = bytes;
        result.mimeType = AiHttp::StringOf(AiHttp::Member(first, "media_type"), "image/png");
        if (result.mimeType.IsEmpty())
            result.mimeType = "image/png";
        return result;
    }

    static Coroutine<AiBytesResult> GenerateThroughImagesApi(const Ref<PipelineExecContext>& ctx, const String& apiKey, const String& model,
                                                             const String& prompt, const Vector<AiImageRef>& references,
                                                             bool transparentBackground)
    {
        AiBytesResult result;
        DataDocument body;
        BuildImagesBody(body, model, prompt, references, transparentBackground);

        auto request = AiHttp::MakeJsonPost(baseUrl + "/images", body, KeyHeaders(apiKey), 300.0f);
        AiHttpResult http = co_await AiHttp::Send(ctx, request, "OpenRouter images call");
        if (!http.ok)
        {
            result.error = AiHttp::DescribeError(http, "OpenRouter images call", model) + StatusHint(http.status);
            co_return result;
        }

        result = ParseImagesResponse(http.json, model);
        if (ctx && result.ok)
            ctx->Log("OpenRouter image: " + result.mimeType + ", " + (String)result.data.Length() + " bytes" + CostNote(http.json));
        co_return result;
    }

    Coroutine<AiTextResult> GenerateText(const Ref<PipelineExecContext>& ctx, const String& apiKey, const String& model,
                                         const String& prompt, const Vector<AiImageRef>& images)
    {
        AiTextResult result;
        if (apiKey.IsEmpty()) { result.error = MissingKey(); co_return result; }

        DataDocument body;
        BuildRequestBody(body, model, prompt, images, false, -1);

        auto request = AiHttp::MakeJsonPost(baseUrl + "/chat/completions", body, KeyHeaders(apiKey), 300.0f);
        AiHttpResult http = co_await AiHttp::Send(ctx, request, "OpenRouter text call");
        if (!http.ok)
        {
            result.error = AiHttp::DescribeError(http, "OpenRouter text call", model) + StatusHint(http.status);
            co_return result;
        }

        result = ParseTextResponse(http.json, model);
        if (ctx && result.ok)
            ctx->Log("OpenRouter text: " + (String)result.text.Length() + " chars" + CostNote(http.json));
        co_return result;
    }

    Coroutine<AiBytesResult> GenerateImage(const Ref<PipelineExecContext>& ctx, const String& apiKey, const String& model,
                                           const String& prompt, const Vector<AiImageRef>& references, int seed,
                                           bool transparentBackground /*= false*/, const AiImageOptions& options /*= AiImageOptions()*/)
    {
        AiBytesResult result;
        if (apiKey.IsEmpty()) { result.error = MissingKey(); co_return result; }

        if (transparentBackground)
        {
            result = co_await GenerateThroughImagesApi(ctx, apiKey, model, prompt, references, true);
            co_return result;
        }

        DataDocument body;
        BuildRequestBody(body, model, prompt.IsEmpty() ? String("Generate the image.") : prompt, references, true, seed, options);

        auto request = AiHttp::MakeJsonPost(baseUrl + "/chat/completions", body, KeyHeaders(apiKey), 300.0f);
        AiHttpResult http = co_await AiHttp::Send(ctx, request, "OpenRouter image call");
        if (!http.ok && http.status == 404)
        {
            if (ctx)
                ctx->Log(model + " is an image-only model: using the OpenRouter images endpoint");

            result = co_await GenerateThroughImagesApi(ctx, apiKey, model, prompt, references, false);
            co_return result;
        }

        if (!http.ok)
        {
            result.error = AiHttp::DescribeError(http, "OpenRouter image call", model) + StatusHint(http.status);
            co_return result;
        }

        result = ParseImageResponse(http.json, model);
        if (ctx && result.ok)
            ctx->Log("OpenRouter image: " + result.mimeType + ", " + (String)result.data.Length() + " bytes" + CostNote(http.json));
        co_return result;
    }
}
