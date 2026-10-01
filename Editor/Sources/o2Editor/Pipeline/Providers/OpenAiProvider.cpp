#include "o2Editor/stdafx.h"
#include "OpenAiProvider.h"

#include "o2Editor/Pipeline/PipelineUtils.h"

namespace Editor::OpenAiProvider
{
    static Map<String, String> KeyHeaders(const String& apiKey)
    {
        Map<String, String> headers;
        headers["Authorization"] = "Bearer " + apiKey;
        return headers;
    }

    static String MissingKey()
    {
        return "OpenAI API key is not configured (Pipeline settings)";
    }

    static String PromptOrDefault(const String& prompt)
    {
        return prompt.Trimed(" \n\r\t").IsEmpty() ? String("Generate the image.") : prompt;
    }

    static String MimeOf(const AiImageRef& image)
    {
        return image.mimeType.IsEmpty() ? String("image/png") : image.mimeType;
    }

    static String RandomBoundary()
    {
        static const char digits[] = "0123456789abcdef";
        String boundary = "----AssetsLineFormBoundary";
        for (int i = 0; i < 24; i++)
            boundary += digits[Math::Random(0, 15)];
        return boundary;
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

    void BuildChatBody(DataDocument& body, const String& model, const String& prompt, const Vector<AiImageRef>& images)
    {
        body.SetObject();
        body["model"] = model;
        auto& messages = body["messages"];
        messages.SetArray();
        auto& message = messages.AddElement();
        message.SetObject();
        message["role"] = String("user");
        if (images.IsEmpty())
        {
            message["content"] = prompt;
            return;
        }

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
            url["url"] = PipelineUtils::BytesToDataUrl(image.data, MimeOf(image));
        }
    }

    String SizeFor(const String& model, const String& aspectRatio)
    {
        if (model.ToLowerCase().StartsWith("dall-e") || aspectRatio.IsEmpty())
            return imageSize;

        auto parts = aspectRatio.Split(":");
        double a = parts.Count() == 2 ? atof(parts[0].Data()) : 0.0, b = parts.Count() == 2 ? atof(parts[1].Data()) : 0.0;
        double r = a > 0.0 && b > 0.0 ? a/b : 1.0;
        return r >= 1.2 ? String("1536x1024") : r <= 1.0/1.2 ? String("1024x1536") : String("1024x1024");
    }

    void BuildGenerationsBody(DataDocument& body, const String& model, const String& prompt, bool transparentBackground /*= false*/,
                              const String& size /*= ""*/)
    {
        body.SetObject();
        body["model"] = model;
        body["prompt"] = PromptOrDefault(prompt);
        body["n"] = 1;
        body["size"] = size.IsEmpty() ? imageSize : size;

        String lower = model.ToLowerCase();
        if (lower.StartsWith("gpt-image"))
            body["output_format"] = String("png");
        else if (lower.StartsWith("dall-e"))
            body["response_format"] = String("b64_json");

        if (transparentBackground)
        {
            body["background"] = String("transparent");
            body["output_format"] = String("png");
        }
    }

    AiMultipartBody BuildEditsBody(const String& model, const String& prompt, const Vector<AiImageRef>& references,
                                   const String& boundary /*= ""*/, bool transparentBackground /*= false*/, const String& size /*= ""*/)
    {
        AiMultipartBody form;
        form.boundary = boundary.IsEmpty() ? RandomBoundary() : boundary;
        form.contentType = "multipart/form-data; boundary=" + form.boundary;

        int reserve = 256;
        for (auto& ref : references)
            reserve += ref.data.Length() + 256;
        form.body.Reserve(reserve + prompt.Length());

        auto field = [&](const String& name, const String& value)
        {
            form.body += "--" + form.boundary + "\r\n";
            form.body += "Content-Disposition: form-data; name=\"" + name + "\"\r\n\r\n";
            form.body += value;
            form.body += "\r\n";
        };

        field("model", model);
        field("prompt", PromptOrDefault(prompt));
        field("n", "1");
        field("size", size.IsEmpty() ? imageSize : size);
        if (transparentBackground)
        {
            field("background", "transparent");
            field("output_format", "png");
        }
        for (int i = 0; i < references.Count(); i++)
        {
            form.body += "--" + form.boundary + "\r\n";
            form.body += "Content-Disposition: form-data; name=\"image[]\"; filename=\"ref" + (String)i + ".png\"\r\n";
            form.body += "Content-Type: " + MimeOf(references[i]) + "\r\n\r\n";
            form.body += references[i].data;
            form.body += "\r\n";
        }
        form.body += "--" + form.boundary + "--\r\n";
        return form;
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

    bool IsQuotaError(const DataValue& json)
    {
        auto err = AiHttp::Member(&json, "error");
        return AiHttp::StringOf(AiHttp::Member(err, "code")) == "insufficient_quota" ||
            AiHttp::StringOf(AiHttp::Member(err, "type")) == "insufficient_quota";
    }

    String StatusHint(int status, const DataValue& json)
    {
        if (status == 401) return " - the OpenAI key was rejected, check it in the pipeline settings";
        if (status == 429 && IsQuotaError(json)) return " - no credits left on the OpenAI account";
        if (status == 404) return " - no such model at OpenAI";
        return "";
    }

    AiTextResult ParseTextResponse(const DataValue& json, const String& model)
    {
        AiTextResult result;
        String error = ParseError(json);
        if (!error.IsEmpty())
        {
            result.error = "OpenAI text call failed (model " + model + "): " + error;
            return result;
        }

        auto first = AiHttp::Element(AiHttp::Member(&json, "choices"), 0);
        String out = TextOfContent(AiHttp::Member(AiHttp::Member(first, "message"), "content")).Trimed(" \n\r\t");
        if (out.IsEmpty())
        {
            String reason = AiHttp::StringOf(AiHttp::Member(first, "finish_reason"));
            result.error = "OpenAI returned no text (model " + model + ")" + (reason.IsEmpty() ? String() : ": " + reason);
            return result;
        }

        result.ok = true;
        result.text = out;
        return result;
    }

    AiBytesResult ParseImageResponse(const DataValue& json, const String& model, String& downloadUrl)
    {
        AiBytesResult result;
        downloadUrl = "";
        String error = ParseError(json);
        if (!error.IsEmpty())
        {
            result.error = "OpenAI image call failed (model " + model + "): " + error;
            return result;
        }

        auto first = AiHttp::Element(AiHttp::Member(&json, "data"), 0);
        String b64 = AiHttp::StringOf(AiHttp::Member(first, "b64_json"));
        if (!b64.IsEmpty())
        {
            result.data = PipelineUtils::Base64Decode(b64);
            result.mimeType = "image/png";
            result.ok = !result.data.IsEmpty();
            if (!result.ok)
                result.error = "OpenAI returned undecodable image data (model " + model + ")";
            return result;
        }

        downloadUrl = AiHttp::StringOf(AiHttp::Member(first, "url"));
        if (downloadUrl.IsEmpty())
            result.error = "OpenAI returned no image data (model " + model + ")";
        return result;
    }

    Coroutine<AiTextResult> GenerateText(const Ref<PipelineExecContext>& ctx, const String& apiKey, const String& model,
                                         const String& prompt, const Vector<AiImageRef>& images)
    {
        AiTextResult result;
        if (apiKey.IsEmpty()) { result.error = MissingKey(); co_return result; }

        DataDocument body;
        BuildChatBody(body, model, prompt, images);

        auto request = AiHttp::MakeJsonPost(baseUrl + "/chat/completions", body, KeyHeaders(apiKey), 300.0f);
        AiHttpResult http = co_await AiHttp::Send(ctx, request, "OpenAI text call");
        if (!http.ok)
        {
            result.error = AiHttp::DescribeError(http, "OpenAI text call", model) + StatusHint(http.status, http.json);
            co_return result;
        }

        result = ParseTextResponse(http.json, model);
        co_return result;
    }

    Coroutine<AiBytesResult> GenerateImage(const Ref<PipelineExecContext>& ctx, const String& apiKey, const String& model,
                                           const String& prompt, const Vector<AiImageRef>& references, int seed,
                                           bool transparentBackground /*= false*/, const AiImageOptions& options /*= AiImageOptions()*/)
    {
        AiBytesResult result;
        if (apiKey.IsEmpty()) { result.error = MissingKey(); co_return result; }

        String size = SizeFor(model, options.aspectRatio);
        Ref<HttpRequest> request;
        if (references.IsEmpty())
        {
            DataDocument body;
            BuildGenerationsBody(body, model, prompt, transparentBackground, size);
            request = AiHttp::MakeJsonPost(baseUrl + "/images/generations", body, KeyHeaders(apiKey), 300.0f);
        }
        else
        {
            AiMultipartBody form = BuildEditsBody(model, prompt, references, "", transparentBackground, size);
            request = AiHttp::MakeRawPost(baseUrl + "/images/edits", form.body, form.contentType, KeyHeaders(apiKey), 300.0f);
        }

        AiHttpResult http = co_await AiHttp::Send(ctx, request, "OpenAI image call");
        if (!http.ok)
        {
            result.error = AiHttp::DescribeError(http, "OpenAI image call", model) + StatusHint(http.status, http.json);
            co_return result;
        }

        String downloadUrl;
        result = ParseImageResponse(http.json, model, downloadUrl);
        if (result.ok || downloadUrl.IsEmpty())
            co_return result;

        auto download = AiHttp::MakeGet(downloadUrl, {}, 300.0f);
        AiHttpResult file = co_await AiHttp::Send(ctx, download, "OpenAI image download", false);
        if (!file.ok || file.body.IsEmpty())
        {
            result.error = AiHttp::DescribeError(file, "OpenAI image download", model);
            co_return result;
        }

        result.ok = true;
        result.data = file.body;
        result.mimeType = "image/png";
        co_return result;
    }
}
