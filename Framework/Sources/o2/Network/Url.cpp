#include "o2/stdafx.h"
#include "Url.h"

namespace o2
{
    static String ToLowerAscii(const String& value)
    {
        String result = value;
        for (size_t i = 0; i < result.size(); i++)
        {
            if (result[i] >= 'A' && result[i] <= 'Z')
                result[i] = (char)(result[i] - 'A' + 'a');
        }

        return result;
    }

    Url Url::Parse(const String& url)
    {
        Url result;

        size_t schemeEnd = url.find("://");
        if (schemeEnd == std::string::npos || schemeEnd == 0)
            return result;

        result.scheme = ToLowerAscii(url.substr(0, schemeEnd));
        if (result.scheme != "http" && result.scheme != "https")
            return result;

        size_t hostBegin = schemeEnd + 3;
        size_t pathBegin = url.find_first_of("/?", hostBegin);

        String hostPort = url.substr(hostBegin, pathBegin == std::string::npos ? std::string::npos : pathBegin - hostBegin);
        if (hostPort.IsEmpty())
            return result;

        size_t portBegin = hostPort.rfind(':');
        if (portBegin != std::string::npos)
        {
            String portString = hostPort.substr(portBegin + 1);
            if (portString.IsEmpty())
                return result;

            for (size_t i = 0; i < portString.size(); i++)
            {
                if (portString[i] < '0' || portString[i] > '9')
                    return result;
            }

            result.port = atoi(portString.Data());
            if (result.port <= 0 || result.port > 65535)
                return result;

            result.host = ToLowerAscii(hostPort.substr(0, portBegin));
        }
        else
        {
            result.host = ToLowerAscii(hostPort);
            result.port = result.scheme == "https" ? 443 : 80;
        }

        if (result.host.IsEmpty())
            return result;

        if (pathBegin != std::string::npos)
        {
            result.path = url.substr(pathBegin);
            if (result.path[0] == '?')
                result.path = "/" + result.path;
        }
        else
            result.path = "/";

        result.isValid = true;
        return result;
    }

    String Url::ToString() const
    {
        String result = scheme + "://" + host;

        bool defaultPort = (scheme == "http" && port == 80) || (scheme == "https" && port == 443);
        if (!defaultPort)
            result += ":" + String(port);

        result += path;
        return result;
    }

    Url Url::ResolveRedirect(const String& location) const
    {
        if (location.find("://") != std::string::npos)
            return Parse(location);

        Url result = *this;
        if (!location.IsEmpty() && location[0] == '/')
            result.path = location;
        else
        {
            size_t queryBegin = result.path.find('?');
            String pathOnly = queryBegin == std::string::npos ? result.path : (String)result.path.substr(0, queryBegin);
            size_t lastSlash = pathOnly.rfind('/');
            result.path = pathOnly.substr(0, lastSlash + 1) + location;
        }

        return result;
    }

    bool Url::operator==(const Url& other) const
    {
        return scheme == other.scheme && host == other.host && port == other.port &&
            path == other.path && isValid == other.isValid;
    }
}
