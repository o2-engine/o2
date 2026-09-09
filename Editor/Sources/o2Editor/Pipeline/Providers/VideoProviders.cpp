#include "o2Editor/stdafx.h"
#include "VideoProviders.h"

#include "o2/Utils/System/Time/Time.h"
#include "o2Editor/Pipeline/PipelineUtils.h"

#include <ctime>

namespace Editor
{
    namespace VeoProvider
    {
        static const String baseUrl = "https://generativelanguage.googleapis.com/v1beta";
        static const float pollIntervalSeconds = 8.0f;
        static const float pollTimeoutSeconds = 600.0f;

        static Map<String, String> KeyHeaders(const String& apiKey)
        {
            Map<String, String> headers;
            headers["x-goog-api-key"] = apiKey;
            return headers;
        }

        int SnapDuration(const String& model, int requested, bool withReferences)
        {
            if (model.StartsWith("veo-2"))
                return requested <= 5 ? 5 : requested <= 6 ? 6 : 8;

            if (withReferences) return 8;
            if (requested <= 4) return 4;
            if (requested <= 6) return 6;
            return 8;
        }

        String SnapAspectRatio(const String& requested)
        {
            return requested == "9:16" ? "9:16" : "16:9";
        }

        static void SetInlineImage(DataValue& target, const AiImageRef& ref)
        {
            target.SetObject();
            target["bytesBase64Encoded"] = PipelineUtils::Base64Encode(ref.data);
            target["mimeType"] = ref.mimeType.IsEmpty() ? String("image/png") : ref.mimeType;
        }

        Coroutine<AiBytesResult> GenerateVideo(const Ref<PipelineExecContext>& ctx, const String& apiKey, const VideoGenerateInput& input)
        {
            AiBytesResult result;
            if (apiKey.IsEmpty()) { result.error = "Gemini API key is not configured (Pipeline settings)"; co_return result; }

            String model = input.model.IsEmpty() ? defaultModel : input.model;
            bool hasRefs = !input.references.IsEmpty();
            int seconds = SnapDuration(model, input.durationSeconds, hasRefs);
            if (seconds != input.durationSeconds && ctx)
                ctx->Log("veo: " + model + " renders " + (String)seconds + "s clips for this request (" + (String)input.durationSeconds + "s asked)");

            String aspect = SnapAspectRatio(input.aspectRatio);
            if (aspect != input.aspectRatio && ctx)
                ctx->Log("veo: " + model + " renders 16:9 or 9:16 only - " + input.aspectRatio + " -> " + aspect);

            DataDocument body;
            body.SetObject();
            auto& instances = body["instances"];
            instances.SetArray();
            auto& instance = instances.AddElement();
            instance.SetObject();
            instance["prompt"] = input.prompt;
            if (hasRefs)
            {
                if (model.StartsWith("veo-3.1"))
                {
                    auto& refs = instance["referenceImages"];
                    refs.SetArray();
                    int count = Math::Min(3, input.references.Count());
                    for (int i = 0; i < count; i++)
                    {
                        auto& item = refs.AddElement();
                        item.SetObject();
                        SetInlineImage(item["image"], input.references[i]);
                        item["referenceType"] = String("asset");
                    }
                }
                else
                    SetInlineImage(instance["image"], input.references[0]);
            }
            auto& parameters = body["parameters"];
            parameters.SetObject();
            parameters["durationSeconds"] = seconds;
            parameters["aspectRatio"] = aspect;

            auto request = AiHttp::MakeJsonPost(baseUrl + "/models/" + model + ":predictLongRunning", body, KeyHeaders(apiKey), 300.0f);
            AiHttpResult start = co_await AiHttp::Send(ctx, request, "Veo start");
            if (!start.ok)
            {
                result.error = AiHttp::DescribeError(start, "Veo start", model);
                if (start.status == 400 && result.error.ToLowerCase().Contains("not supported"))
                {
                    result.error = model + " refused this request: " + (String)seconds + "s, " + aspect + ", " + (String)input.references.Count() +
                        " reference image(s). Veo 3.x renders 16:9 or 9:16 and 8-second clips with references; Veo 2 renders 5, 6 or 8 seconds.";
                }
                co_return result;
            }

            String opName = AiHttp::StringOf(AiHttp::Member(&start.json, "name"));
            if (opName.IsEmpty()) { result.error = "Veo did not return an operation name (model " + model + ")"; co_return result; }
            if (ctx) ctx->Log("veo: operation started (" + opName + ")");

            DataDocument op;
            op = static_cast<const DataValue&>(start.json);
            float waited = 0.0f;
            while (!(AiHttp::Member(&op, "done") && (bool)*AiHttp::Member(&op, "done")))
            {
                if (waited > pollTimeoutSeconds) { result.error = "Veo generation timed out after 10 min (model " + model + ")"; co_return result; }
                if (ctx && ctx->IsCancelled()) { result.error = "Cancelled"; co_return result; }

                co_await WaitTime(pollIntervalSeconds);
                co_await SwitchToMain();
                waited += pollIntervalSeconds;

                auto poll = AiHttp::MakeGet(baseUrl + "/" + opName, KeyHeaders(apiKey), 60.0f);
                AiHttpResult polled = co_await AiHttp::Send(ctx, poll, "Veo poll");
                if (!polled.ok) { result.error = AiHttp::DescribeError(polled, "Veo poll", model); co_return result; }
                op = static_cast<const DataValue&>(polled.json);
            }

            if (auto err = AiHttp::Member(&op, "error"))
            {
                result.error = "Veo generation failed (model " + model + "): " + AiHttp::StringOf(AiHttp::Member(err, "message"), "unknown error");
                co_return result;
            }

            auto response = AiHttp::Member(&op, "response");
            auto sample = AiHttp::Element(AiHttp::Member(AiHttp::Member(response, "generateVideoResponse"), "generatedSamples"), 0);
            if (!sample)
                sample = AiHttp::Element(AiHttp::Member(response, "generatedVideos"), 0);
            auto video = AiHttp::Member(sample, "video");
            if (!video)
                video = sample;

            String uri = AiHttp::StringOf(AiHttp::Member(video, "uri"));
            String b64 = AiHttp::StringOf(AiHttp::Member(video, "bytesBase64Encoded"));
            if (b64.IsEmpty()) b64 = AiHttp::StringOf(AiHttp::Member(video, "encodedVideo"));

            if (!b64.IsEmpty())
                result.data = PipelineUtils::Base64Decode(b64);
            else if (!uri.IsEmpty())
            {
                auto download = AiHttp::MakeGet(uri, KeyHeaders(apiKey), 600.0f);
                AiHttpResult got = co_await AiHttp::Send(ctx, download, "Veo download", false);
                if (!got.ok) { result.error = "Veo video download failed: HTTP " + (String)got.status; co_return result; }
                result.data = got.body;
            }
            else
            {
                auto filtered = AiHttp::Element(AiHttp::Member(AiHttp::Member(response, "generateVideoResponse"), "raiMediaFilteredReasons"), 0);
                if (filtered) result.error = "Veo filtered the result: " + AiHttp::StringOf(filtered);
                else result.error = "Veo returned no video (model " + model + ")";
                co_return result;
            }

            result.ok = true;
            result.mimeType = "video/mp4";
            result.seconds = seconds;
            co_return result;
        }
    }

    namespace KlingProvider
    {
        static const String baseUrl = "https://api-singapore.klingai.com";
        static const float pollIntervalSeconds = 5.0f;
        static const float pollTimeoutSeconds = 900.0f;

        const Vector<String> models = { "kling-v2-1-master", "kling-v2-1", "kling-v2-master", "kling-v1-6", "kling-v1-5", "kling-v1" };

        static String Base64Url(const String& data)
        {
            String s = PipelineUtils::Base64Encode(data);
            s.ReplaceAll("+", "-");
            s.ReplaceAll("/", "_");
            s.ReplaceAll("=", "");
            return s;
        }

        String MakeJwt(const String& accessKey, const String& secretKey)
        {
            time_t now = time(nullptr);
            String header = Base64Url("{\"alg\":\"HS256\",\"typ\":\"JWT\"}");
            String payload = Base64Url("{\"iss\":\"" + accessKey + "\",\"exp\":" + (String)(int)(now + 1800) + ",\"nbf\":" + (String)(int)(now - 5) + "}");
            String signature = Base64Url(PipelineUtils::HmacSha256(secretKey, header + "." + payload));
            return header + "." + payload + "." + signature;
        }

        String Bearer(const String& accessKey, const String& secretKey)
        {
            if (accessKey.StartsWith("api-key-") || secretKey.IsEmpty())
                return accessKey;

            return MakeJwt(accessKey, secretKey);
        }

        static Map<String, String> AuthHeaders(const String& accessKey, const String& secretKey)
        {
            Map<String, String> headers;
            headers["Authorization"] = "Bearer " + Bearer(accessKey, secretKey);
            return headers;
        }

        Coroutine<AiBytesResult> GenerateVideo(const Ref<PipelineExecContext>& ctx, const String& accessKey, const String& secretKey,
                                               const VideoGenerateInput& input)
        {
            AiBytesResult result;
            if (accessKey.IsEmpty()) { result.error = "Kling API key is not configured (Pipeline settings)"; co_return result; }

            String model = input.model.IsEmpty() ? models[0] : input.model;
            int refs = input.references.Count();
            String endpoint = refs == 0 ? "/v1/videos/text2video" : refs == 1 ? "/v1/videos/image2video" : "/v1/videos/multi-image2video";

            DataDocument body;
            body.SetObject();
            body["model_name"] = model;
            body["prompt"] = input.prompt;
            body["duration"] = String(input.durationSeconds >= 10 ? "10" : "5");
            body["aspect_ratio"] = input.aspectRatio;
            if (refs == 1)
                body["image"] = PipelineUtils::Base64Encode(input.references[0].data);
            else if (refs > 1)
            {
                auto& list = body["image_list"];
                list.SetArray();
                for (int i = 0; i < Math::Min(4, refs); i++)
                {
                    auto& item = list.AddElement();
                    item.SetObject();
                    item["image"] = PipelineUtils::Base64Encode(input.references[i].data);
                }
            }

            auto request = AiHttp::MakeJsonPost(baseUrl + endpoint, body, AuthHeaders(accessKey, secretKey), 120.0f);
            AiHttpResult start = co_await AiHttp::Send(ctx, request, "Kling start");
            if (!start.ok) { result.error = AiHttp::DescribeError(start, "Kling start", model); co_return result; }

            String taskId = AiHttp::StringOf(AiHttp::Member(AiHttp::Member(&start.json, "data"), "task_id"));
            if (taskId.IsEmpty()) { result.error = "Kling did not return a task id: " + AiHttp::StringOf(AiHttp::Member(&start.json, "message")); co_return result; }
            if (ctx) ctx->Log("kling: task " + taskId + " created");

            float waited = 0.0f;
            while (true)
            {
                if (waited > pollTimeoutSeconds) { result.error = "Kling generation timed out (model " + model + ")"; co_return result; }
                if (ctx && ctx->IsCancelled()) { result.error = "Cancelled"; co_return result; }

                co_await WaitTime(pollIntervalSeconds);
                co_await SwitchToMain();
                waited += pollIntervalSeconds;

                auto poll = AiHttp::MakeGet(baseUrl + endpoint + "/" + taskId, AuthHeaders(accessKey, secretKey), 60.0f);
                AiHttpResult polled = co_await AiHttp::Send(ctx, poll, "Kling poll");
                if (!polled.ok) { result.error = AiHttp::DescribeError(polled, "Kling poll", model); co_return result; }

                auto data = AiHttp::Member(&polled.json, "data");
                String status = AiHttp::StringOf(AiHttp::Member(data, "task_status"));
                if (status == "failed")
                {
                    result.error = "Kling generation failed: " + AiHttp::StringOf(AiHttp::Member(data, "task_status_msg"), "unknown error");
                    co_return result;
                }
                if (status != "succeed")
                    continue;

                auto video = AiHttp::Element(AiHttp::Member(AiHttp::Member(data, "task_result"), "videos"), 0);
                String url = AiHttp::StringOf(AiHttp::Member(video, "url"));
                if (url.IsEmpty()) { result.error = "Kling returned no video url"; co_return result; }

                auto download = AiHttp::MakeGet(url, {}, 600.0f);
                AiHttpResult got = co_await AiHttp::Send(ctx, download, "Kling download", false);
                if (!got.ok) { result.error = "Kling video download failed: HTTP " + (String)got.status; co_return result; }

                result.ok = true;
                result.data = got.body;
                result.mimeType = "video/mp4";
                result.seconds = (int)AiHttp::NumberOf(AiHttp::Member(video, "duration"), (float)input.durationSeconds);
                co_return result;
            }
        }
    }
}
