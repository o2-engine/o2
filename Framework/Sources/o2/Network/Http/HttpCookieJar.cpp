#include "o2/stdafx.h"
#include "HttpCookieJar.h"

#include <time.h>

namespace o2
{
    static String TrimSpaces(const String& value)
    {
        size_t begin = value.find_first_not_of(" \t\r\n");
        if (begin == std::string::npos)
            return String();

        size_t end = value.find_last_not_of(" \t\r\n");
        return value.substr(begin, end - begin + 1);
    }

    static String ToLowerAsciiCopy(const String& value)
    {
        String result = value;
        for (size_t i = 0; i < result.size(); i++)
        {
            if (result[i] >= 'A' && result[i] <= 'Z')
                result[i] = (char)(result[i] - 'A' + 'a');
        }

        return result;
    }

    // Parses an RFC 1123 date like "Wed, 21 Oct 2015 07:28:00 GMT" into unix seconds, 0 on failure
    static Int64 ParseHttpDate(const String& value)
    {
        static const char* months[] = { "jan", "feb", "mar", "apr", "may", "jun",
                                        "jul", "aug", "sep", "oct", "nov", "dec" };

        int day = 0, year = 0, hour = 0, minute = 0, second = 0;
        char monthName[4] = { 0 };

        String lower = ToLowerAsciiCopy(value);
        if (sscanf(lower.Data(), "%*[a-z], %d %3s %d %d:%d:%d", &day, monthName, &year, &hour, &minute, &second) != 6)
            return 0;

        int month = -1;
        for (int i = 0; i < 12; i++)
        {
            if (strncmp(monthName, months[i], 3) == 0)
            {
                month = i;
                break;
            }
        }

        if (month < 0)
            return 0;

        tm timeParts = {};
        timeParts.tm_mday = day;
        timeParts.tm_mon = month;
        timeParts.tm_year = year - 1900;
        timeParts.tm_hour = hour;
        timeParts.tm_min = minute;
        timeParts.tm_sec = second;

#ifdef PLATFORM_WINDOWS
        return (Int64)_mkgmtime(&timeParts);
#else
        return (Int64)timegm(&timeParts);
#endif
    }

    // Returns true when the host matches the cookie domain: equal, or a subdomain for
    // non-host-only cookies
    static bool DomainMatches(const HttpCookie& cookie, const String& host)
    {
        if (host == cookie.domain)
            return true;

        if (cookie.hostOnly)
            return false;

        if (host.size() <= cookie.domain.size())
            return false;

        size_t offset = host.size() - cookie.domain.size();
        return host[offset - 1] == '.' && host.compare(offset, cookie.domain.size(), cookie.domain) == 0;
    }

    static bool PathMatches(const HttpCookie& cookie, const String& path)
    {
        if (path.compare(0, cookie.path.size(), cookie.path) != 0)
            return false;

        return cookie.path.size() == path.size() ||
            cookie.path[cookie.path.size() - 1] == '/' ||
            path[cookie.path.size()] == '/' || path[cookie.path.size()] == '?';
    }

    bool HttpCookie::operator==(const HttpCookie& other) const
    {
        return name == other.name && domain == other.domain && path == other.path;
    }

    HttpCookieJar::HttpCookieJar(RefCounter* refCounter):
        RefCounterable(refCounter)
    {}

    void HttpCookieJar::SetFromHeader(const Url& url, const String& setCookieHeader)
    {
        HttpCookie cookie;
        cookie.domain = url.host;
        cookie.hostOnly = true;

        // Default path: the directory of the request path
        size_t queryBegin = url.path.find('?');
        String pathOnly = queryBegin == std::string::npos ? url.path : (String)url.path.substr(0, queryBegin);
        size_t lastSlash = pathOnly.rfind('/');
        cookie.path = lastSlash == 0 ? String("/") : (String)pathOnly.substr(0, lastSlash);

        bool remove = false;
        bool first = true;

        size_t position = 0;
        while (position <= setCookieHeader.size())
        {
            size_t partEnd = setCookieHeader.find(';', position);
            if (partEnd == std::string::npos)
                partEnd = setCookieHeader.size();

            String part = TrimSpaces(setCookieHeader.substr(position, partEnd - position));
            position = partEnd + 1;

            if (part.IsEmpty())
                continue;

            size_t equals = part.find('=');
            String key = equals == std::string::npos ? part : (String)part.substr(0, equals);
            String value = equals == std::string::npos ? String() : (String)part.substr(equals + 1);

            if (first)
            {
                if (equals == std::string::npos || key.IsEmpty())
                    return;

                cookie.name = TrimSpaces(key);
                cookie.value = TrimSpaces(value);
                first = false;
                continue;
            }

            String lowerKey = ToLowerAsciiCopy(TrimSpaces(key));
            value = TrimSpaces(value);

            if (lowerKey == "max-age")
            {
                int seconds = atoi(value.Data());
                if (seconds <= 0)
                    remove = true;
                else
                    cookie.expiresAt = (Int64)time(nullptr) + seconds;
            }
            else if (lowerKey == "expires" && cookie.expiresAt == 0)
            {
                Int64 expires = ParseHttpDate(value);
                if (expires != 0)
                {
                    if (expires <= (Int64)time(nullptr))
                        remove = true;
                    else
                        cookie.expiresAt = expires;
                }
            }
            else if (lowerKey == "domain" && !value.IsEmpty())
            {
                if (value[0] == '.')
                    value = value.substr(1);

                cookie.domain = ToLowerAsciiCopy(value);
                cookie.hostOnly = false;
            }
            else if (lowerKey == "path" && !value.IsEmpty() && value[0] == '/')
                cookie.path = value;
            else if (lowerKey == "secure")
                cookie.secure = true;
        }

        if (cookie.name.IsEmpty())
            return;

        mCookies.RemoveAll([&](const HttpCookie& other) { return other == cookie; });

        if (!remove)
            mCookies.Add(cookie);
    }

    String HttpCookieJar::GetCookieHeader(const Url& url)
    {
        RemoveExpired();

        String result;
        for (auto& cookie : mCookies)
        {
            if (cookie.secure && url.scheme != "https")
                continue;

            if (!DomainMatches(cookie, url.host) || !PathMatches(cookie, url.path))
                continue;

            if (!result.IsEmpty())
                result += "; ";

            result += cookie.name + "=" + cookie.value;
        }

        return result;
    }

    void HttpCookieJar::Add(const HttpCookie& cookie)
    {
        mCookies.RemoveAll([&](const HttpCookie& other) { return other == cookie; });
        mCookies.Add(cookie);
    }

    const Vector<HttpCookie>& HttpCookieJar::GetCookies() const
    {
        return mCookies;
    }

    void HttpCookieJar::Clear()
    {
        mCookies.Clear();
    }

    void HttpCookieJar::RemoveExpired()
    {
        Int64 now = (Int64)time(nullptr);
        mCookies.RemoveAll([&](const HttpCookie& cookie) { return cookie.expiresAt != 0 && cookie.expiresAt <= now; });
    }
}
