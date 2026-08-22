# Сеть

Сетевой системой управляет `o2::NetworkSystem` (синглтон `o2Network`). Она прокачивает все асинхронные сокеты и HTTP-запросы раз в кадр на главном потоке; все сетевые коллбеки вызываются из этой прокачки, поэтому весь сетевой API — только для главного потока. Каждая асинхронная операция существует в двух формах: коллбек-событие (`o2::Function<>`) и ожидаемая [корутина](/Docs/ru/Architecture/Utils/coroutines.md). Ошибки возвращаются значением, не исключениями.

## HTTP

Запрос описывается `o2::HttpRequest` (`url`, `method`, `headers`, `body`, `timeout`, `followRedirects`/`maxRedirects`, `cachePolicy`, `useCookies`; `SetBody(data, contentType)`, `SetBodyJson(document)`), ответ — `o2::HttpResponse` (`error`, `status`, `headers`, `body`, `fromCache`; `IsSuccess()`, `GetHeader(name)` — без учёта регистра, `GetBodyJson(document)`, `GetBodyImage()` — PNG в `Bitmap`, `GetJson()` в скриптах). `body` — `o2::String`, бинарно-безопасная.

- **o2Network.SendRequest(request, onCompleted)** — отправляет запрос; коллбек получает ответ на главном потоке.
- **o2Network.RequestAsync(request)** — `Coroutine<Ref<HttpResponse>>`, `co_await` возвращает ответ.
- **o2Network.HttpGet / HttpPost (+ Async)** — сокращения; в скриптах доступны как `o2.Http.Get/Post/Send`.
- **HttpMethod** — `Get`, `Post`, `Put`, `Delete`, `Head`, `Patch`, `Options`.
- **HttpError** — `None`, `InvalidUrl`, `ResolveFailed`, `ConnectionFailed`, `ConnectionClosed`, `Timeout`, `TlsNotSupported`, `TooManyRedirects`, `Internal`. Ответ с кодом ошибки (например 404) — не ошибка передачи: `error == None`, `IsSuccess() == false`.

Cookies (`o2Network.GetCookies()`, `o2::HttpCookieJar`) и кеш ответов в памяти (`o2Network.GetCache()`, `o2::HttpCache`) реализованы на стороне движка и общие для всех бэкендов. Jar разбирает `Set-Cookie` с сопоставлением домена/пути/срока/`Secure`. Кеш хранит GET-ответы по `Cache-Control: max-age` и ревалидирует устаревшие через `ETag`/`Last-Modified` (304 отдаёт сохранённое тело с `fromCache == true`). `HttpCachePolicy`: `Default`, `Bypass`, `PreferCached`. Редиректы выполняет клиентский слой, до `maxRedirects`; 303 (и 301/302 с телом) превращают переход в GET.

Бэкенды (`o2Network.SetHttpBackendType`): `Platform` (по умолчанию) — NSURLSession на macOS/iOS, `HttpURLConnection` через JNI на Android, браузерный `fetch` на WebAssembly; `Generic` — портируемый HTTP/1.1-клиент поверх собственных TCP-сокетов движка, только http (`TlsNotSupported` для https), используется по умолчанию на Windows/Linux. На WebAssembly cookies, кеш и редиректы принадлежат самому браузеру — движковые jar и кеш там ничего не видят.

## Сокеты

Низкий уровень, полезная нагрузка — бинарно-безопасная `o2::String`:

- **o2::TcpSocket** — неблокирующее TCP-соединение: `Connect(host, port)` / `ConnectAsync`, `Send`, `ReceiveAsync`, `Close`; события `onConnected(success)`, `onDataReceived(data)`, `onClosed` (только удалённое закрытие или ошибка). Данные сначала получают ожидающие `ReceiveAsync`, затем `onDataReceived`. `Send` буферизуется, пока идёт подключение.
- **o2::TcpListener** — `Listen(port)` (порт 0 = эфемерный, см. `GetLocalPort()`), `AcceptAsync` / `onAccepted(socket)`.
- **o2::UdpSocket** — датаграммы для онлайн-геймплея: `Open(port)`, `SendTo(address, port, data)`, `Connect(address, port)` + `Send(data)` для фиксированного адресата, `ReceiveAsync` / `onDataReceived(data, address, port)`. Доступен в скриптах.

Уровень сообщений — фрейминг с префиксом длины (4 байта little-endian), каждый коллбек несёт ровно одно целое сообщение; оба класса доступны в скриптах:

- **o2::TcpMessageChannel** — клиент для чатов/лобби: `Connect` / `ConnectAsync`, `Send(message)`, `ReceiveAsync`, события `onConnected`, `onMessage`, `onClosed`.
- **o2::TcpMessageServer** — `Listen(port)`, клиенты получают числовые id: `SendTo(clientId, message)`, `Broadcast(message)`, `DisconnectClient(clientId)`, `GetClientIds()`, события `onClientConnected`, `onClientMessage(clientId, message)`, `onClientDisconnected`.

Резолв имени хоста в `TcpSocket::Connect` выполняется на воркере [джоб-системы](/Docs/ru/Architecture/Utils/jobs.md); `UdpSocket` резолвит синхронно с кешем на сокет. Отмены нет: закрытие сокета завершает его ожидающие корутины пустыми результатами. Данные, подключения и сообщения, пришедшие, когда нет ни ожидающей корутины, ни подписчика события, буферизуются и отдаются первому потребителю — между отправкой и более поздним `ReceiveAsync`/`AcceptAsync` ничего не теряется. На WebAssembly сырые сокеты компилируются через эмуляцию WebSocket в Emscripten и в рантайме требуют WebSocket-совместимую точку подключения; UDP в браузерах недоступен.

## Скриптинг

Привязывается автоматически через рефлексию: `o2.Http.Get(url, callback)` / `Post` / `Send(request, callback)`, `new o2.HttpRequest()`, объекты ответа со `status`/`body`/`IsSuccess()`/`GetJson()`, `new o2.TcpMessageChannel()`, `new o2.UdpSocket()`; события назначаются обычными JS-функциями (`channel.onMessage = function(m) {...}`).

<details>
<summary>Пример</summary>

```C++
// Коллбеки
o2Network.HttpGet("http://example.com/state.json", [](const Ref<HttpResponse>& response)
{
    if (!response->IsSuccess())
        return;

    DataDocument document;
    response->GetBodyJson(document);
    Apply(document);
});

// Корутина: последовательные запросы записываются линейно
auto flow = []() -> Coroutine<void>
{
    Ref<HttpResponse> config = co_await o2Network.HttpGetAsync("http://example.com/config.json");
    Ref<HttpResponse> icon = co_await o2Network.HttpGetAsync("http://example.com/icon.png");

    if (icon->IsSuccess())
        texture->SetData(*icon->GetBodyImage());
}();
flow.Start(JobThread::Main);

// Чат-клиент поверх канала сообщений
auto chat = mmake<TcpMessageChannel>();
chat->onMessage = [](const String& message) { ShowMessage(message); };
chat->Connect("chat.example.com", 7777);
chat->Send("hello");

// Онлайн-геймплей поверх UDP
auto peer = mmake<UdpSocket>();
peer->Open();
peer->Connect("game.example.com", 7778);
peer->onDataReceived = [](const String& data, const String&, int) { ApplySnapshot(data); };
peer->Send(BuildInputPacket());
```
</details>
