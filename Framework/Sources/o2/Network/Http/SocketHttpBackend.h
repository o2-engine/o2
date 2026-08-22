#pragma once

#include "o2/Network/Http/HttpBackend.h"
#include "o2/Utils/Types/Ref.h"

namespace o2
{
    FORWARD_CLASS_REF(TcpSocket);

    // -------------------------------------------------------------------------------------------
    // Portable HTTP/1.1 backend over the engine's own TCP sockets. Supports plain http only —
    // an https transfer fails with TlsNotSupported. Used on platforms without a native HTTP
    // facility and for tests; runs entirely on the main thread pump
    // -------------------------------------------------------------------------------------------
    class SocketHttpBackend: public IHttpBackend
    {
    public:
        // Destructor, closes active transfers
        ~SocketHttpBackend() override;

        // Begins the transfer over a TCP socket
        void Perform(const SharedRef<HttpTransfer>& transfer) override;

        // Advances transfer timeouts
        void Update(float dt) override;

    protected:
        // Active transfer state, driven by the socket callbacks
        struct Transfer: public RefCounterable
        {
            SharedRef<HttpTransfer> request; // The transfer being performed

            Ref<TcpSocket> socket;   // Connection to the server
            String         received; // Accumulated response bytes

            bool   headersParsed = false; // True once the status line and headers are parsed
            int    status = 0;            // Parsed status code
            Vector<String> headerLines;   // Parsed response header lines
            size_t bodyBegin = 0;         // Offset of the body in received
            int    contentLength = -1;    // Content-Length, -1 when absent
            bool   chunked = false;       // Transfer-Encoding: chunked
            bool   noBody = false;        // The response has no body (HEAD, 1xx/204/304)

            float elapsed = 0.0f;  // Transfer time, for the timeout
            bool  finished = false; // True once the result was delivered
        };

        Vector<Ref<Transfer>> mTransfers; // Active transfers

    protected:
        // Handles received bytes: parses headers and completes when the body is whole
        void OnData(Transfer* transfer, const String& data);

        // Handles the connection close: completes with the remaining body or an error
        void OnClosed(Transfer* transfer);

        // Completes the transfer with the parsed response
        void FinishSuccess(Transfer* transfer);

        // Completes the transfer with the error
        void FinishError(Transfer* transfer, HttpError error);

        // Parses the status line and headers once they are whole. Returns false on malformed data
        bool ParseHeaders(Transfer* transfer);

        // Removes the transfer from the active list and closes its socket
        void RemoveTransfer(Transfer* transfer);
    };
}
