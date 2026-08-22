# Network

The network system is managed by `o2::NetworkSystem` (`o2Network` singleton). It pumps all asynchronous sockets and HTTP requests once per frame on the main thread; every network callback is invoked from that pump, so the whole network API is main-thread only. Every asynchronous operation exists in two forms: an event callback (`o2::Function<>`) and an awaitable [coroutine](/Docs/en/Architecture/Utils/coroutines.md). Errors are reported by value, never thrown.

## HTTP

Requests are described by `o2::HttpRequest` (`url`, `method`, `headers`, `body`, `timeout`, `followRedirects`/`maxRedirects`, `cachePolicy`, `useCookies`; `SetBody(data, contentType)`, `SetBodyJson(document)`) and answered with `o2::HttpResponse` (`error`, `status`, `headers`, `body`, `fromCache`; `IsSuccess()`, `GetHeader(name)` — case-insensitive, `GetBodyJson(document)`, `GetBodyImage()` — PNG to `Bitmap`, `GetJson()` in scripts). `body` is `o2::String`, which is binary-safe.

- **o2Network.SendRequest(request, onCompleted)** — sends the request; the callback receives the response on the main thread.
- **o2Network.RequestAsync(request)** — `Coroutine<Ref<HttpResponse>>`, `co_await` it for the response.
- **o2Network.HttpGet / HttpPost (+ Async)** — shortcuts; also exposed to scripts as `o2.Http.Get/Post/Send`.
- **HttpMethod** — `Get`, `Post`, `Put`, `Delete`, `Head`, `Patch`, `Options`.
- **HttpError** — `None`, `InvalidUrl`, `ResolveFailed`, `ConnectionFailed`, `ConnectionClosed`, `Timeout`, `TlsNotSupported`, `TooManyRedirects`, `Internal`. A served error status (e.g. 404) is not a transfer error: `error == None`, `IsSuccess() == false`.

Cookies (`o2Network.GetCookies()`, `o2::HttpCookieJar`) and the in-memory response cache (`o2Network.GetCache()`, `o2::HttpCache`) are engine-side and shared by all backends. The jar parses `Set-Cookie` with domain/path/expiration/`Secure` matching. The cache stores GET responses by `Cache-Control: max-age` and revalidates stale entries with `ETag`/`Last-Modified` (a 304 serves the stored body with `fromCache == true`). `HttpCachePolicy`: `Default`, `Bypass`, `PreferCached`. Redirects are followed by the client layer up to `maxRedirects`; 303 (and 301/302 with a body) turn the hop into GET.

Backends (`o2Network.SetHttpBackendType`): `Platform` (default) — NSURLSession on macOS/iOS, `HttpURLConnection` via JNI on Android, browser `fetch` on WebAssembly; `Generic` — a portable HTTP/1.1 client over the engine's own TCP sockets, plain http only (`TlsNotSupported` for https), the default on Windows/Linux. On WebAssembly the browser itself owns cookies, caching and redirects, so the engine-side jar and cache see nothing there.

## Sockets

Low level, payloads are binary-safe `o2::String`:

- **o2::TcpSocket** — non-blocking TCP connection: `Connect(host, port)` / `ConnectAsync`, `Send`, `ReceiveAsync`, `Close`; events `onConnected(success)`, `onDataReceived(data)`, `onClosed` (remote close or error only). Data goes to pending `ReceiveAsync` awaiters first, then to `onDataReceived`. `Send` is buffered while connecting.
- **o2::TcpListener** — `Listen(port)` (port 0 = ephemeral, see `GetLocalPort()`), `AcceptAsync` / `onAccepted(socket)`.
- **o2::UdpSocket** — datagrams for realtime gameplay: `Open(port)`, `SendTo(address, port, data)`, `Connect(address, port)` + `Send(data)` for a fixed remote, `ReceiveAsync` / `onDataReceived(data, address, port)`. Scriptable.

Message level — length-prefixed framing (4-byte little-endian size), each callback carries exactly one whole message; both classes are scriptable:

- **o2::TcpMessageChannel** — client for chats/lobbies: `Connect` / `ConnectAsync`, `Send(message)`, `ReceiveAsync`, events `onConnected`, `onMessage`, `onClosed`.
- **o2::TcpMessageServer** — `Listen(port)`, clients get numeric ids: `SendTo(clientId, message)`, `Broadcast(message)`, `DisconnectClient(clientId)`, `GetClientIds()`, events `onClientConnected`, `onClientMessage(clientId, message)`, `onClientDisconnected`.

Host name resolution in `TcpSocket::Connect` runs on a [job](/Docs/en/Architecture/Utils/jobs.md) worker; `UdpSocket` resolves synchronously with a per-socket cache. There is no cancellation: closing a socket completes its pending awaiters with empty results. Data, connections and messages arriving while there is neither a pending awaiter nor an event subscriber are buffered and handed to the first consumer, so nothing is lost between a send and a later `ReceiveAsync`/`AcceptAsync`. On WebAssembly raw sockets compile against Emscripten's WebSocket emulation and need a WebSocket-capable endpoint at runtime; UDP is unavailable in browsers.

## Scripting

Bound automatically through reflection: `o2.Http.Get(url, callback)` / `Post` / `Send(request, callback)`, `new o2.HttpRequest()`, response objects with `status`/`body`/`IsSuccess()`/`GetJson()`, `new o2.TcpMessageChannel()`, `new o2.UdpSocket()`; events are assigned as plain JS functions (`channel.onMessage = function(m) {...}`).

<details>
<summary>Example</summary>

```C++
// Callbacks
o2Network.HttpGet("http://example.com/state.json", [](const Ref<HttpResponse>& response)
{
    if (!response->IsSuccess())
        return;

    DataDocument document;
    response->GetBodyJson(document);
    Apply(document);
});

// Coroutine: sequential requests written linearly
auto flow = []() -> Coroutine<void>
{
    Ref<HttpResponse> config = co_await o2Network.HttpGetAsync("http://example.com/config.json");
    Ref<HttpResponse> icon = co_await o2Network.HttpGetAsync("http://example.com/icon.png");

    if (icon->IsSuccess())
        texture->SetData(*icon->GetBodyImage());
}();
flow.Start(JobThread::Main);

// Chat client over the message channel
auto chat = mmake<TcpMessageChannel>();
chat->onMessage = [](const String& message) { ShowMessage(message); };
chat->Connect("chat.example.com", 7777);
chat->Send("hello");

// Realtime gameplay over UDP
auto peer = mmake<UdpSocket>();
peer->Open();
peer->Connect("game.example.com", 7778);
peer->onDataReceived = [](const String& data, const String&, int) { ApplySnapshot(data); };
peer->Send(BuildInputPacket());
```
</details>
