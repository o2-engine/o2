#pragma once

#include "o2/Network/Http/HttpRequest.h"
#include "o2/Network/Http/HttpResponse.h"
#include "o2/Utils/Basic/IObject.h"

namespace o2
{
    // -------------------------------------------------------------------------------------------
    // HTTP shortcuts over o2Network, also exposed to scripts as o2.Http: Get/Post with callbacks
    // and Send for a configured HttpRequest. Coroutine variants live on o2Network
    // -------------------------------------------------------------------------------------------
    class Http: public IObject
    {
    public:
        // Sends a GET request; the callback is invoked on the main thread with the response @SCRIPTABLE
        static void Get(const String& url, const Function<void(const Ref<HttpResponse>&)>& onCompleted);

        // Sends a POST request with the body @SCRIPTABLE
        static void Post(const String& url, const String& body,
                         const Function<void(const Ref<HttpResponse>&)>& onCompleted);

        // Sends a configured request @SCRIPTABLE
        static void Send(const Ref<HttpRequest>& request, const Function<void(const Ref<HttpResponse>&)>& onCompleted);

        IOBJECT(Http);
    };
}
// --- META ---

CLASS_BASES_META(o2::Http)
{
    BASE_CLASS(o2::IObject);
}
END_META;
CLASS_FIELDS_META(o2::Http)
{
}
END_META;
CLASS_METHODS_META(o2::Http)
{

    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE_STATIC(void, Get, const String&, const Function<void(const Ref<HttpResponse>&)>&);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE_STATIC(void, Post, const String&, const String&, const Function<void(const Ref<HttpResponse>&)>&);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE_STATIC(void, Send, const Ref<HttpRequest>&, const Function<void(const Ref<HttpResponse>&)>&);
}
END_META;
// --- END META ---
