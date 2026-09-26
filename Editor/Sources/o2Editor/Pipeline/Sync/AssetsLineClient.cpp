#include "o2Editor/stdafx.h"
#include "AssetsLineClient.h"

#include "o2/Network/NetworkSystem.h"

namespace Editor
{
    Coroutine<AssetsLineResponse> AssetsLineClient::Request(HttpMethod method, const String& path, const String& body /*= ""*/,
                                                            const String& contentType /*= ""*/, float timeout /*= 60.0f*/) const
    {
        auto request = mmake<HttpRequest>(serverUrl + path, method);
        request->timeout = timeout;
        request->useCookies = false;
        request->cachePolicy = HttpCachePolicy::Bypass;
        if (!token.IsEmpty())
            request->headers["Authorization"] = "Bearer " + token;

        if (!projectId.IsEmpty())
            request->headers["X-Project-Id"] = projectId;

        if (!clientId.IsEmpty())
            request->headers["X-Sync-Client"] = clientId;

        if (!contentType.IsEmpty())
            request->SetBody(body, contentType);

        AssetsLineResponse result;
        Ref<HttpResponse> response = co_await o2Network.RequestAsync(request);
        if (!response || response->error != HttpError::None)
        {
            result.offline = true;
            result.error = !response ? String("no response")
                : response->error == HttpError::Timeout ? String("the server did not answer in time")
                : response->error == HttpError::TlsNotSupported ? String("https is not available on this platform")
                : "cannot reach " + serverUrl;
            co_return result;
        }

        result.status = response->status;
        result.body = response->body;
        result.ok = response->IsSuccess();
        String trimmed = result.body.Trimed(" \n\r\t");
        if (trimmed.StartsWith("{") || trimmed.StartsWith("["))
            result.json.LoadFromData(result.body);

        if (!result.ok)
        {
            if (auto code = result.json.IsObject() ? result.json.FindMember("code") : nullptr; code && code->IsString())
                result.code = code->GetString();

            String message;
            for (auto key : { "message", "error" })
            {
                if (auto value = result.json.IsObject() ? result.json.FindMember(key) : nullptr; value && value->IsString())
                {
                    message = value->GetString();
                    break;
                }
            }

            result.error = "HTTP " + (String)result.status + (message.IsEmpty() ? String() : ": " + message);
        }

        co_return result;
    }

    Coroutine<AssetsLineResponse> AssetsLineClient::SendJson(HttpMethod method, const String& path, const DataDocument& body) const
    {
        co_return co_await Request(method, path, body.SaveAsString(), "application/json");
    }

    String AssetsLineClient::Encode(const String& text)
    {
        static const char* hex = "0123456789ABCDEF";
        String out;
        for (int i = 0; i < text.Length(); i++)
        {
            unsigned char c = (unsigned char)text[i];
            if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.' || c == '~')
                out += (char)c;
            else
            {
                out += '%';
                out += hex[c >> 4];
                out += hex[c & 15];
            }
        }
        return out;
    }

    String AssetsLineClient::NormalizeServerUrl(const String& url)
    {
        String value = url.Trimed(" \n\r\t");
        while (value.EndsWith("/"))
            value = value.SubStr(0, value.Length() - 1);

        if (!value.IsEmpty() && !value.StartsWith("http://") && !value.StartsWith("https://"))
            value = "https://" + value;

        return value;
    }
}
