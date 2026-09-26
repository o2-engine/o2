#pragma once

#include "o2/Network/Http/HttpRequest.h"
#include "o2/Network/Http/HttpResponse.h"
#include "o2/Utils/Coroutines/Coroutines.h"
#include "o2/Utils/Serialization/DataValue.h"

using namespace o2;

namespace Editor
{
    // -----------------------------------
    // Answer of one AssetsLine API request
    // -----------------------------------
    struct AssetsLineResponse
    {
        bool         ok = false;      // A 2xx answer arrived
        bool         offline = false; // No answer: the server could not be reached
        int          status = 0;      // HTTP status, 0 without an answer
        String       code;            // Error code the server names: rev-mismatch, name-taken, stale-rev...
        String       error;           // Readable error
        String       body;            // Raw body
        DataDocument json;            // Body parsed as JSON, when it is JSON
    };

    // ------------------------------------------------------------------------
    // HTTP access to the AssetsLine API as a connected editor: bearer token,
    // project header, and the client id the server echoes in its change events
    // ------------------------------------------------------------------------
    class AssetsLineClient
    {
    public:
        String serverUrl; // Server address without a trailing slash
        String token;     // Access token; empty for the public connect calls
        String projectId; // Project the requests act on
        String clientId;  // Sent as X-Sync-Client

    public:
        // Sends a request to `path` (starting with /api/) and waits for the answer
        Coroutine<AssetsLineResponse> Request(HttpMethod method, const String& path, const String& body = "",
                                              const String& contentType = "", float timeout = 60.0f) const;

        // Sends a JSON body
        Coroutine<AssetsLineResponse> SendJson(HttpMethod method, const String& path, const DataDocument& body) const;

        // Percent-encodes a query or path component
        static String Encode(const String& text);

        // Trims spaces and trailing slashes, adds https:// when no scheme is given
        static String NormalizeServerUrl(const String& url);
    };
}
