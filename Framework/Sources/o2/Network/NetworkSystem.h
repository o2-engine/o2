#pragma once

#include "o2/Network/Http/HttpTypes.h"
#include "o2/Network/Url.h"
#include "o2/Utils/Coroutines/Coroutines.h"
#include "o2/Utils/Function/Function.h"
#include "o2/Utils/Singleton.h"
#include "o2/Utils/Types/Containers/Vector.h"
#include "o2/Utils/Types/Ref.h"

// Network system access macros
#define o2Network o2::NetworkSystem::Instance()

namespace o2
{
    FORWARD_CLASS_REF(HttpCache);
    FORWARD_CLASS_REF(HttpCookieJar);
    FORWARD_CLASS_REF(HttpRequest);
    FORWARD_CLASS_REF(HttpResponse);
    FORWARD_CLASS_REF(LogStream);

    class TcpSocket;
    class TcpListener;
    class UdpSocket;
    class IHttpBackend;
    struct HttpTransfer;

    // HTTP backend selection
    enum class HttpBackendType
    {
        Platform, // The platform-native HTTP facility; falls back to Generic where there is none
        Generic   // The portable HTTP/1.1 backend over the engine's TCP sockets, http only
    };

    // ---------------------------------------------------------------------------------------
    // Network system. Pumps the asynchronous sockets and HTTP requests every frame on the main
    // thread; all network callbacks are invoked from this pump. The whole network API is
    // main-thread only. HTTP requests go through the platform backend with engine-side cookies,
    // cache and redirects; see also the o2::Http shortcuts
    // ---------------------------------------------------------------------------------------
    class NetworkSystem: public Singleton<NetworkSystem>
    {
    public:
        // Default constructor
        NetworkSystem(RefCounter* refCounter);

        // Destructor
        ~NetworkSystem();

        // Pumps sockets and HTTP requests, invoking ready callbacks. Called by the application
        // every frame; test runners without a frame loop call it manually
        void Update(float dt);

        // Sends the HTTP request; the callback is invoked on the main thread with the response
        void SendRequest(const Ref<HttpRequest>& request, const Function<void(const Ref<HttpResponse>&)>& onCompleted);

        // Coroutine HTTP request: completes with the response
        Coroutine<Ref<HttpResponse>> RequestAsync(const Ref<HttpRequest>& request);

        // Sends an HTTP GET request with the callback
        void HttpGet(const String& url, const Function<void(const Ref<HttpResponse>&)>& onCompleted);

        // Coroutine HTTP GET request
        Coroutine<Ref<HttpResponse>> HttpGetAsync(const String& url);

        // Sends an HTTP POST request with the body and callback
        void HttpPost(const String& url, const String& body,
                      const Function<void(const Ref<HttpResponse>&)>& onCompleted);

        // Coroutine HTTP POST request
        Coroutine<Ref<HttpResponse>> HttpPostAsync(const String& url, const String& body);

        // Returns the HTTP cookie storage
        const Ref<HttpCookieJar>& GetCookies() const;

        // Returns the HTTP response cache
        const Ref<HttpCache>& GetCache() const;

        // Selects the HTTP backend. Active transfers keep their backend
        void SetHttpBackendType(HttpBackendType type);

        // Returns the selected HTTP backend type
        HttpBackendType GetHttpBackendType() const;

        // Returns the network log stream
        const Ref<LogStream>& GetLog() const;

    protected:
        // One HTTP request in flight: the current hop transfer and the completion state
        struct PendingHttp
        {
            SharedRef<HttpTransfer> transfer;      // Current hop transfer at the backend
            Ref<HttpRequest>        request;       // The original request
            Function<void(const Ref<HttpResponse>&)> callback; // Completion callback
            Url                     url;           // Current hop URL
            HttpMethod              method;        // Current hop method, changes on 303-style redirects
            int                     redirectsLeft = 0; // Remaining redirect hops
        };

        // A response ready to be delivered on the next pump
        struct ReadyHttp
        {
            Function<void(const Ref<HttpResponse>&)> callback; // Completion callback
            Ref<HttpResponse>                        response; // The response to deliver
        };

        Ref<LogStream> mLog; // Network log stream

        Vector<WeakRef<TcpSocket>>   mTcpSockets;   // Pumped TCP sockets
        Vector<WeakRef<TcpListener>> mTcpListeners; // Pumped TCP listeners
        Vector<WeakRef<UdpSocket>>   mUdpSockets;   // Pumped UDP sockets

        Ref<HttpCookieJar> mCookies; // HTTP cookie storage
        Ref<HttpCache>     mCache;   // HTTP response cache

        IHttpBackend*   mHttpBackend = nullptr;                   // Active HTTP backend
        HttpBackendType mHttpBackendType = HttpBackendType::Platform; // Selected backend type

        Vector<PendingHttp> mPendingHttp; // Requests in flight
        Vector<ReadyHttp>   mReadyHttp;   // Responses to deliver on the next pump

    protected:
        // Pumps every alive entry of the list over a copy, so callbacks can register and remove
        // sockets freely, then drops the expired entries
        template<typename _socket_type>
        void PumpSockets(Vector<WeakRef<_socket_type>>& sockets, float dt);

        // Delivers ready responses and completed transfers, handling redirects, cookies and cache
        void UpdateHttp(float dt);

        // Builds and sends the current hop transfer to the backend
        void DispatchHttp(PendingHttp& pending);

        // Handles one finished hop: redirects, cookie storing and caching. Returns the response
        // to deliver, or null when a redirect hop was dispatched instead
        Ref<HttpResponse> ProcessFinishedHttp(PendingHttp& pending);

        // Queues the response for delivery on the next pump
        void CompleteHttp(const Function<void(const Ref<HttpResponse>&)>& callback, const Ref<HttpResponse>& response);

        // Builds a response with just an error
        Ref<HttpResponse> MakeErrorResponse(HttpError error) const;

        // Builds a response from a cache entry
        Ref<HttpResponse> MakeCachedResponse(const String& url) const;

        // Returns the active backend, creating it lazily
        IHttpBackend* GetHttpBackend();

        // Adds the socket to the pump
        void RegisterTcpSocket(const Ref<TcpSocket>& socket);

        // Removes the socket from the pump
        void UnregisterTcpSocket(TcpSocket* socket);

        // Adds the listener to the pump
        void RegisterTcpListener(const Ref<TcpListener>& listener);

        // Removes the listener from the pump
        void UnregisterTcpListener(TcpListener* listener);

        // Adds the socket to the pump
        void RegisterUdpSocket(const Ref<UdpSocket>& socket);

        // Removes the socket from the pump
        void UnregisterUdpSocket(UdpSocket* socket);

        friend class TcpSocket;
        friend class TcpListener;
        friend class UdpSocket;
    };
}
// --- META ---

PRE_ENUM_META(o2::HttpBackendType);
// --- END META ---
