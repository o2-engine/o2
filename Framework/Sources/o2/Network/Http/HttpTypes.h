#pragma once

namespace o2
{
    // HTTP request method
    enum class HttpMethod
    {
        Get,
        Post,
        Put,
        Delete,
        Head,
        Patch,
        Options
    };

    // HTTP transfer error. Errors are reported by value in HttpResponse, never thrown
    enum class HttpError
    {
        None,             // No error, the transfer finished (any status code)
        InvalidUrl,       // The URL could not be parsed or has an unsupported scheme
        ResolveFailed,    // Host name resolution failed
        ConnectionFailed, // Could not connect to the host
        ConnectionClosed, // The connection closed before the whole response arrived
        Timeout,          // The transfer did not finish in time
        TlsNotSupported,  // https is not available with the current backend
        TooManyRedirects, // The redirect chain exceeded the limit
        Internal          // Unexpected backend failure
    };

    // HTTP cache usage per request
    enum class HttpCachePolicy
    {
        Default,     // Serve fresh cached responses, revalidate stale ones with the server
        Bypass,      // Ignore the cache completely, do not store the response
        PreferCached // Serve any cached response regardless of freshness, request only when missing
    };
}
// --- META ---

PRE_ENUM_META(o2::HttpMethod);

PRE_ENUM_META(o2::HttpError);

PRE_ENUM_META(o2::HttpCachePolicy);
// --- END META ---
