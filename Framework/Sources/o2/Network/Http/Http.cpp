#include "o2/stdafx.h"
#include "Http.h"

#include "o2/Network/NetworkSystem.h"

namespace o2
{
    void Http::Get(const String& url, const Function<void(const Ref<HttpResponse>&)>& onCompleted)
    {
        o2Network.HttpGet(url, onCompleted);
    }

    void Http::Post(const String& url, const String& body,
                    const Function<void(const Ref<HttpResponse>&)>& onCompleted)
    {
        o2Network.HttpPost(url, body, onCompleted);
    }

    void Http::Send(const Ref<HttpRequest>& request, const Function<void(const Ref<HttpResponse>&)>& onCompleted)
    {
        o2Network.SendRequest(request, onCompleted);
    }
}
// --- META ---

DECLARE_CLASS(o2::Http, o2__Http);
// --- END META ---
