#pragma once

#include "o2/Network/Http/HttpTypes.h"
#include "o2/Utils/Basic/IObject.h"
#include "o2/Utils/Serialization/DataValue.h"
#include "o2/Utils/Types/Containers/Map.h"
#include "o2/Utils/Types/Ref.h"
#include "o2/Utils/Types/String.h"

namespace o2
{
    // ------------------------------------------------------------------------------------------
    // HTTP request description: URL, method, headers, body and transfer options. Sent through
    // o2Network.SendRequest / RequestAsync or the o2::Http shortcuts
    // ------------------------------------------------------------------------------------------
    class HttpRequest: public IObject, public RefCounterable
    {
    public:
        String     url;                  // Request URL, http or https @SCRIPTABLE
        HttpMethod method = HttpMethod::Get; // Request method @SCRIPTABLE

        Map<String, String> headers; // Extra request headers @SCRIPTABLE

        String body; // Request body bytes; binary-safe @SCRIPTABLE

        float timeout = 30.0f; // Transfer timeout in seconds @SCRIPTABLE

        bool followRedirects = true; // Follow 3xx redirects automatically @SCRIPTABLE
        int  maxRedirects = 10;      // Redirect chain limit @SCRIPTABLE

        HttpCachePolicy cachePolicy = HttpCachePolicy::Default; // Cache usage for this request @SCRIPTABLE

        bool useCookies = true; // Attach stored cookies and store received ones @SCRIPTABLE

    public:
        // Default constructor @SCRIPTABLE
        explicit HttpRequest(RefCounter* refCounter);

        // Constructor with url and method
        HttpRequest(RefCounter* refCounter, const String& url, HttpMethod method = HttpMethod::Get);

        // Sets the body to the document's JSON text and the JSON content type
        void SetBodyJson(const DataDocument& document);

        // Sets the body string and the content type header @SCRIPTABLE
        void SetBody(const String& data, const String& contentType);

        IOBJECT(HttpRequest);
    };
}
// --- META ---

CLASS_BASES_META(o2::HttpRequest)
{
    BASE_CLASS(o2::IObject);
    BASE_CLASS(o2::RefCounterable);
}
END_META;
CLASS_FIELDS_META(o2::HttpRequest)
{
    FIELD().PUBLIC().SCRIPTABLE_ATTRIBUTE().NAME(url);
    FIELD().PUBLIC().SCRIPTABLE_ATTRIBUTE().DEFAULT_VALUE(HttpMethod::Get).NAME(method);
    FIELD().PUBLIC().SCRIPTABLE_ATTRIBUTE().NAME(headers);
    FIELD().PUBLIC().SCRIPTABLE_ATTRIBUTE().NAME(body);
    FIELD().PUBLIC().SCRIPTABLE_ATTRIBUTE().DEFAULT_VALUE(30.0f).NAME(timeout);
    FIELD().PUBLIC().SCRIPTABLE_ATTRIBUTE().DEFAULT_VALUE(true).NAME(followRedirects);
    FIELD().PUBLIC().SCRIPTABLE_ATTRIBUTE().DEFAULT_VALUE(10).NAME(maxRedirects);
    FIELD().PUBLIC().SCRIPTABLE_ATTRIBUTE().DEFAULT_VALUE(HttpCachePolicy::Default).NAME(cachePolicy);
    FIELD().PUBLIC().SCRIPTABLE_ATTRIBUTE().DEFAULT_VALUE(true).NAME(useCookies);
}
END_META;
CLASS_METHODS_META(o2::HttpRequest)
{

    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*, const String&, HttpMethod);
    FUNCTION().PUBLIC().SIGNATURE(void, SetBodyJson, const DataDocument&);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetBody, const String&, const String&);
}
END_META;
// --- END META ---
