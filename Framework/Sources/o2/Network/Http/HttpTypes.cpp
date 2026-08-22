#include "o2/stdafx.h"
#include "HttpTypes.h"
// --- META ---

ENUM_META(o2::HttpMethod, o2__HttpMethod)
{
    ENUM_ENTRY(Delete);
    ENUM_ENTRY(Get);
    ENUM_ENTRY(Head);
    ENUM_ENTRY(Options);
    ENUM_ENTRY(Patch);
    ENUM_ENTRY(Post);
    ENUM_ENTRY(Put);
}
END_ENUM_META;

ENUM_META(o2::HttpError, o2__HttpError)
{
    ENUM_ENTRY(ConnectionClosed);
    ENUM_ENTRY(ConnectionFailed);
    ENUM_ENTRY(Internal);
    ENUM_ENTRY(InvalidUrl);
    ENUM_ENTRY(None);
    ENUM_ENTRY(ResolveFailed);
    ENUM_ENTRY(Timeout);
    ENUM_ENTRY(TlsNotSupported);
    ENUM_ENTRY(TooManyRedirects);
}
END_ENUM_META;

ENUM_META(o2::HttpCachePolicy, o2__HttpCachePolicy)
{
    ENUM_ENTRY(Bypass);
    ENUM_ENTRY(Default);
    ENUM_ENTRY(PreferCached);
}
END_ENUM_META;
// --- END META ---
