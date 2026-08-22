#pragma once

#include "o2/Network/Url.h"
#include "o2/Utils/Types/Containers/Vector.h"
#include "o2/Utils/Types/Ref.h"
#include "o2/Utils/Types/String.h"

namespace o2
{
    // Stored HTTP cookie
    struct HttpCookie
    {
        String name;     // Cookie name
        String value;    // Cookie value
        String domain;   // Matching domain, without the leading dot
        String path = "/"; // Matching path prefix
        Int64  expiresAt = 0; // Expiration unix time in seconds; 0 = session cookie
        bool   secure = false;   // Sent only over https
        bool   hostOnly = false; // Matches the exact host only, no subdomains

        // Returns true when name, domain and path match — the cookie identity
        bool operator==(const HttpCookie& other) const;
    };

    // -------------------------------------------------------------------------------------------
    // HTTP cookie storage. Parses Set-Cookie response headers and builds Cookie request headers
    // with domain, path, expiration and secure matching. Used automatically by the HTTP client;
    // per-request opt-out with HttpRequest::useCookies
    // -------------------------------------------------------------------------------------------
    class HttpCookieJar: public RefCounterable
    {
    public:
        // Default constructor
        explicit HttpCookieJar(RefCounter* refCounter);

        // Parses one Set-Cookie header received from the URL and stores, replaces or removes
        // the cookie
        void SetFromHeader(const Url& url, const String& setCookieHeader);

        // Returns the Cookie header value for a request to the URL, empty when nothing matches
        String GetCookieHeader(const Url& url);

        // Adds or replaces a cookie directly
        void Add(const HttpCookie& cookie);

        // Returns all stored cookies
        const Vector<HttpCookie>& GetCookies() const;

        // Removes all cookies
        void Clear();

    protected:
        Vector<HttpCookie> mCookies; // Stored cookies

    protected:
        // Removes expired cookies
        void RemoveExpired();
    };
}
