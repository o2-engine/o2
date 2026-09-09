#include "o2Editor/stdafx.h"
#include "AiHttp.h"

#include "o2/Network/NetworkSystem.h"
#include "o2/Utils/Debug/Debug.h"
#include "o2/Utils/Math/Math.h"
#include "o2Editor/Pipeline/PipelineUtils.h"

namespace Editor::AiHttp
{
    bool IsRetryableStatus(int status)
    {
        return status == 408 || status == 429 || status == 500 || status == 502 || status == 503 || status == 504;
    }

    Ref<HttpRequest> MakeJsonPost(const String& url, const DataDocument& body, const Map<String, String>& headers, float timeout /*= 300.0f*/)
    {
        auto request = mmake<HttpRequest>(url, HttpMethod::Post);
        request->SetBodyJson(body);
        request->timeout = timeout;
        request->useCookies = false;
        request->cachePolicy = HttpCachePolicy::Bypass;
        for (auto& kv : headers)
            request->headers[kv.first] = kv.second;
        return request;
    }

    Ref<HttpRequest> MakeGet(const String& url, const Map<String, String>& headers, float timeout /*= 60.0f*/)
    {
        auto request = mmake<HttpRequest>(url, HttpMethod::Get);
        request->timeout = timeout;
        request->useCookies = false;
        request->cachePolicy = HttpCachePolicy::Bypass;
        for (auto& kv : headers)
            request->headers[kv.first] = kv.second;
        return request;
    }

    Coroutine<AiHttpResult> Send(const Ref<PipelineExecContext>& ctx, const Ref<HttpRequest>& request,
                                 const String& opName, bool parseJson /*= true*/)
    {
        AiHttpResult result;
        for (int attempt = 1; attempt <= maxAttempts; attempt++)
        {
            if (ctx && ctx->IsCancelled())
            {
                result.ok = false;
                result.error = "Cancelled";
                co_return result;
            }

            Ref<HttpResponse> response = co_await o2Network.RequestAsync(request);

            result = AiHttpResult();
            if (!response)
            {
                result.error = opName + ": no response";
            }
            else
            {
                result.status = response->status;
                result.body = response->body;
                if (response->error != HttpError::None)
                {
                    result.error = opName + ": transfer error " + (String)(int)response->error;
                    if (response->error == HttpError::Timeout)
                        result.error = opName + ": request timed out";
                }
                else if (!response->IsSuccess())
                    result.error = opName + ": HTTP " + (String)response->status;
                else
                    result.ok = true;

                if (parseJson && !result.body.IsEmpty())
                    result.jsonParsed = result.json.LoadFromData(result.body);
            }

            bool transient = response && (response->error == HttpError::Timeout || response->error == HttpError::ConnectionFailed ||
                                          response->error == HttpError::ConnectionClosed || IsRetryableStatus(response->status));
            if (result.ok || !transient || attempt == maxAttempts)
                co_return result;

            float base = Math::Min(10.0f, std::pow(2.0f, (float)(attempt - 1)));
            float delay = base + Math::Random(0.0f, 0.5f);
            if (ctx)
            {
                ctx->SignalRetry(attempt, maxAttempts, result.status, "HTTP " + (String)result.status);
                ctx->Log(opName + ": attempt " + (String)attempt + "/" + (String)maxAttempts + " got " + (String)result.status +
                         ", retrying in " + (String)delay + "s");
            }

            co_await WaitTime(delay);
            co_await SwitchToMain();
        }

        co_return result;
    }

    String DescribeError(const AiHttpResult& result, const String& opName, const String& model)
    {
        String message = result.error.IsEmpty() ? opName + " failed" : result.error;
        if (!model.IsEmpty())
            message += " (model " + model + ")";

        if (result.jsonParsed)
        {
            auto err = Member(&result.json, "error");
            String detail = StringOf(Member(err, "message"));
            if (detail.IsEmpty() && err && err->IsString())
                detail = err->GetString();
            if (!detail.IsEmpty())
                message += ": " + detail;
        }
        else if (!result.body.IsEmpty())
        {
            String snippet = result.body.SubStr(0, Math::Min(400, result.body.Length()));
            message += ": " + snippet;
        }

        return message;
    }

    const DataValue* Member(const DataValue* value, const char* name)
    {
        if (!value || !value->IsObject())
            return nullptr;

        return value->FindMember(name);
    }

    const DataValue* Element(const DataValue* value, int index)
    {
        if (!value || !value->IsArray() || index < 0 || index >= value->GetElementsCount())
            return nullptr;

        return &(*const_cast<DataValue*>(value))[index];
    }

    String StringOf(const DataValue* value, const String& def /*= ""*/)
    {
        if (!value)
            return def;

        return PipelineUtils::ValueToString(*value, def);
    }

    float NumberOf(const DataValue* value, float def /*= 0.0f*/)
    {
        if (!value)
            return def;

        return PipelineUtils::ValueToNumber(*value, def);
    }
}
