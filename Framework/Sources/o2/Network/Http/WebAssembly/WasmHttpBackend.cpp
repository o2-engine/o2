#include "o2/stdafx.h"

#ifdef PLATFORM_WASM

#include "o2/Network/Http/HttpBackend.h"

#include <emscripten/fetch.h>

namespace o2
{
    // One in-flight fetch: keeps the transfer and the request strings alive until the browser
    // callback fires, then deletes itself
    struct WasmFetchState
    {
        SharedRef<HttpTransfer>  transfer;
        Vector<String>           headerStorage;  // "Name" and "value" strings
        std::vector<const char*> headerPointers; // Pairs of pointers into headerStorage, null-terminated
    };

    static void OnWasmFetchSuccess(emscripten_fetch_t* fetch)
    {
        WasmFetchState* state = (WasmFetchState*)fetch->userData;
        HttpTransfer* transfer = state->transfer.Get();

        transfer->status = (int)fetch->status;

        size_t headersLength = emscripten_fetch_get_response_headers_length(fetch);
        if (headersLength > 0)
        {
            std::string headers(headersLength + 1, '\0');
            emscripten_fetch_get_response_headers(fetch, &headers[0], headersLength + 1);

            size_t lineBegin = 0;
            while (lineBegin < headers.size())
            {
                size_t lineEnd = headers.find("\r\n", lineBegin);
                if (lineEnd == std::string::npos)
                    lineEnd = headers.size();

                if (lineEnd > lineBegin)
                    transfer->responseHeaderLines.Add(String(headers.substr(lineBegin, lineEnd - lineBegin)));

                lineBegin = lineEnd + 2;
            }
        }

        if (fetch->data && fetch->numBytes > 0)
            transfer->responseBody.assign(fetch->data, (size_t)fetch->numBytes);

        transfer->done.Store(1);

        emscripten_fetch_close(fetch);
        delete state;
    }

    static void OnWasmFetchError(emscripten_fetch_t* fetch)
    {
        WasmFetchState* state = (WasmFetchState*)fetch->userData;
        HttpTransfer* transfer = state->transfer.Get();

        // The browser does not expose the failure cause; a served error status is still a response
        if (fetch->status >= 100)
        {
            transfer->status = (int)fetch->status;
            if (fetch->data && fetch->numBytes > 0)
                transfer->responseBody.assign(fetch->data, (size_t)fetch->numBytes);
        }
        else
            transfer->error = HttpError::ConnectionFailed;

        transfer->done.Store(1);

        emscripten_fetch_close(fetch);
        delete state;
    }

    // Browser fetch backend. Cookies, cache and redirects are managed by the browser itself:
    // the engine-side jar and cache see no Set-Cookie headers and 3xx statuses here
    class WasmHttpBackend: public IHttpBackend
    {
    public:
        void Perform(const SharedRef<HttpTransfer>& transfer) override
        {
            auto state = new WasmFetchState();
            state->transfer = transfer;

            for (auto& line : transfer->headerLines)
            {
                size_t colon = line.find(':');
                if (colon == std::string::npos)
                    continue;

                size_t valueBegin = line.find_first_not_of(" \t", colon + 1);
                String value = valueBegin == std::string::npos ? String() : (String)line.substr(valueBegin);

                state->headerStorage.Add(line.substr(0, colon));
                state->headerStorage.Add(value);
            }

            for (auto& header : state->headerStorage)
                state->headerPointers.push_back(header.Data());

            state->headerPointers.push_back(nullptr);

            emscripten_fetch_attr_t attr;
            emscripten_fetch_attr_init(&attr);
            strncpy(attr.requestMethod, transfer->method.Data(), sizeof(attr.requestMethod) - 1);
            attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;
            attr.timeoutMSecs = (unsigned long)(transfer->timeout * 1000.0f);
            attr.userData = state;
            attr.onsuccess = OnWasmFetchSuccess;
            attr.onerror = OnWasmFetchError;
            attr.requestHeaders = state->headerPointers.data();

            if (!transfer->body.IsEmpty())
            {
                attr.requestData = transfer->body.data();
                attr.requestDataSize = transfer->body.size();
            }

            emscripten_fetch(&attr, transfer->url.Data());
        }
    };

    IHttpBackend* CreateWasmHttpBackend()
    {
        return mnew WasmHttpBackend();
    }
}

#endif // PLATFORM_WASM
