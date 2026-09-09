#pragma once

#include "o2/Network/Http/HttpRequest.h"
#include "o2/Network/Http/HttpResponse.h"
#include "o2/Utils/Coroutines/Coroutines.h"
#include "o2/Utils/Serialization/DataValue.h"
#include "o2Editor/Pipeline/PipelineNodeType.h"

using namespace o2;

namespace Editor
{
    // -------------------------------
    // Outcome of a provider HTTP call
    // -------------------------------
    struct AiHttpResult
    {
        bool         ok = false; // True when the transfer finished with a success status
        int          status = 0; // HTTP status code, 0 when no response arrived
        String       error;      // Error description when not ok
        String       body;       // Raw response body
        DataDocument json;       // Parsed when the body is JSON

        // Returns true when the body was parsed into json
        bool HasJson() const { return jsonParsed; }

        bool jsonParsed = false; // True when json holds the parsed body
    };

    // ---------------------------------------------------------------------
    // Shared HTTP layer for the AI providers: JSON requests with retries on
    // transient statuses (408, 429, 5xx) and exponential backoff
    // ---------------------------------------------------------------------
    namespace AiHttp
    {
        const int maxAttempts = 6; // Tries per request including the first one

        // True for the statuses worth a retry: 408, 429 and the 5xx gateway errors
        bool IsRetryableStatus(int status);

        // Sends the request with retries; the context receives retry signals and cancellation
        Coroutine<AiHttpResult> Send(const Ref<PipelineExecContext>& ctx, const Ref<HttpRequest>& request,
                                     const String& opName, bool parseJson = true);

        // Builds a POST request with a JSON body, cookies and caching off
        Ref<HttpRequest> MakeJsonPost(const String& url, const DataDocument& body, const Map<String, String>& headers,
                                      float timeout = 300.0f);

        // Builds a GET request with cookies and caching off
        Ref<HttpRequest> MakeGet(const String& url, const Map<String, String>& headers, float timeout = 60.0f);

        // Error message from a provider body, or a generic one
        String DescribeError(const AiHttpResult& result, const String& opName, const String& model);

        // Returns the named member of an object, nullptr when the value is missing or not an object
        const DataValue* Member(const DataValue* value, const char* name);

        // Returns the array element at index, nullptr when the value is missing, not an array or out of range
        const DataValue* Element(const DataValue* value, int index);

        // Returns the value converted to a string, def when the value is missing
        String StringOf(const DataValue* value, const String& def = "");

        // Returns the value converted to a number, def when the value is missing
        float NumberOf(const DataValue* value, float def = 0.0f);
    }
}
