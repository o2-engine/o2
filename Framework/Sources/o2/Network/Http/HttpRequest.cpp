#include "o2/stdafx.h"
#include "HttpRequest.h"

namespace o2
{
    HttpRequest::HttpRequest(RefCounter* refCounter):
        RefCounterable(refCounter)
    {}

    HttpRequest::HttpRequest(RefCounter* refCounter, const String& url, HttpMethod method /*= HttpMethod::Get*/):
        RefCounterable(refCounter), url(url), method(method)
    {}

    void HttpRequest::SetBodyJson(const DataDocument& document)
    {
        SetBody(document.SaveAsString(), "application/json");
    }

    void HttpRequest::SetBody(const String& data, const String& contentType)
    {
        body = data;
        headers[String("Content-Type")] = contentType;
    }
}
// --- META ---

DECLARE_CLASS(o2::HttpRequest, o2__HttpRequest);
// --- END META ---
