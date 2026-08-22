#pragma once

#include "o2/Utils/Types/String.h"

namespace o2
{
    // ----------------------------------------------------------------------------
    // Parsed URL: scheme, host, port and path with query. Only http/https URLs are
    // considered valid for the HTTP client
    // ----------------------------------------------------------------------------
    struct Url
    {
        String scheme;       // Lower-case scheme, e.g. "http"
        String host;         // Lower-case host name or address
        int    port = 0;     // Port; the scheme default when not specified
        String path = "/";   // Path with query, always starts with "/"
        bool   isValid = false; // True when parsing succeeded

        // Parses the URL string
        static Url Parse(const String& url);

        // Returns the URL string
        String ToString() const;

        // Resolves a redirect location against this URL: absolute URLs are parsed as is,
        // absolute paths keep the origin, relative paths resolve against the current path
        Url ResolveRedirect(const String& location) const;

        // Returns true when all parts are equal
        bool operator==(const Url& other) const;
    };
}
