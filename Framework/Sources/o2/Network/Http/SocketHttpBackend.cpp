#include "o2/stdafx.h"
#include "SocketHttpBackend.h"

#include "o2/Network/Sockets/TcpSocket.h"
#include "o2/Network/Url.h"

namespace o2
{
    // Decodes a whole chunked body. Returns false when the data is incomplete or malformed
    static bool DecodeChunkedBody(const char* data, size_t size, String& out)
    {
        size_t position = 0;
        while (true)
        {
            size_t lineEnd = position;
            while (lineEnd + 1 < size && !(data[lineEnd] == '\r' && data[lineEnd + 1] == '\n'))
                lineEnd++;

            if (lineEnd + 1 >= size)
                return false;

            int chunkSize = (int)strtol(String(std::string(data + position, lineEnd - position)).Data(), nullptr, 16);
            if (chunkSize < 0)
                return false;

            position = lineEnd + 2;
            if (chunkSize == 0)
                return true;

            if (position + chunkSize + 2 > size)
                return false;

            out.append(data + position, (size_t)chunkSize);
            position += chunkSize + 2; // Skip the chunk and its trailing CRLF
        }
    }

    SocketHttpBackend::~SocketHttpBackend()
    {
        auto transfers = mTransfers;
        for (auto& transfer : transfers)
        {
            if (transfer->socket)
                transfer->socket->Close();
        }

        mTransfers.Clear();
    }

    void SocketHttpBackend::Perform(const SharedRef<HttpTransfer>& transfer)
    {
        Url url = Url::Parse(transfer->url);
        if (!url.isValid)
        {
            transfer->error = HttpError::InvalidUrl;
            transfer->done.Store(1);
            return;
        }

        if (url.scheme == "https")
        {
            transfer->error = HttpError::TlsNotSupported;
            transfer->done.Store(1);
            return;
        }

        auto state = mmake<Transfer>();
        state->request = transfer;
        state->noBody = transfer->method == "HEAD";
        mTransfers.Add(state);

        // Build the request text ahead: it is buffered by the socket and flushed after connect
        String requestText = transfer->method + " " + url.path + " HTTP/1.1\r\n";

        bool defaultPort = (url.scheme == "http" && url.port == 80);
        requestText += "Host: " + url.host + (defaultPort ? String() : (String)(":" + String(url.port))) + "\r\n";

        for (auto& line : transfer->headerLines)
            requestText += line + "\r\n";

        if (!transfer->body.IsEmpty() || transfer->method == "POST" || transfer->method == "PUT" ||
            transfer->method == "PATCH")
        {
            requestText += "Content-Length: " + String((int)transfer->body.size()) + "\r\n";
        }

        requestText += "Accept-Encoding: identity\r\n";
        requestText += "Connection: close\r\n\r\n";
        requestText += transfer->body;

        Transfer* statePtr = state.Get();

        state->socket = mmake<TcpSocket>();
        state->socket->connectTimeout = transfer->timeout;

        state->socket->onConnected = [this, statePtr](bool success)
        {
            if (!success)
                FinishError(statePtr, HttpError::ConnectionFailed);
        };

        state->socket->onDataReceived = [this, statePtr](const String& data) { OnData(statePtr, data); };
        state->socket->onClosed = [this, statePtr] { OnClosed(statePtr); };

        if (!state->socket->Connect(url.host, url.port))
        {
            FinishError(statePtr, HttpError::ConnectionFailed);
            return;
        }

        state->socket->Send(requestText);
    }

    void SocketHttpBackend::Update(float dt)
    {
        auto transfers = mTransfers;
        for (auto& transfer : transfers)
        {
            transfer->elapsed += dt;
            if (!transfer->finished && transfer->elapsed > transfer->request->timeout)
                FinishError(transfer.Get(), HttpError::Timeout);
        }
    }

    void SocketHttpBackend::OnData(Transfer* transfer, const String& data)
    {
        if (transfer->finished)
            return;

        transfer->received.append(data.data(), data.size());

        if (!transfer->headersParsed)
        {
            size_t headersEnd = transfer->received.find("\r\n\r\n");
            if (headersEnd == std::string::npos)
                return;

            if (!ParseHeaders(transfer))
            {
                FinishError(transfer, HttpError::Internal);
                return;
            }
        }

        if (transfer->noBody)
        {
            FinishSuccess(transfer);
            return;
        }

        if (transfer->contentLength >= 0 &&
            transfer->received.size() >= transfer->bodyBegin + (size_t)transfer->contentLength)
        {
            FinishSuccess(transfer);
            return;
        }

        if (transfer->chunked)
        {
            String body;
            if (DecodeChunkedBody(transfer->received.data() + transfer->bodyBegin,
                                  transfer->received.size() - transfer->bodyBegin, body))
            {
                FinishSuccess(transfer);
            }
        }
    }

    void SocketHttpBackend::OnClosed(Transfer* transfer)
    {
        if (transfer->finished)
            return;

        if (!transfer->headersParsed)
        {
            FinishError(transfer, HttpError::ConnectionClosed);
            return;
        }

        if (transfer->contentLength >= 0 &&
            transfer->received.size() < transfer->bodyBegin + (size_t)transfer->contentLength)
        {
            FinishError(transfer, HttpError::ConnectionClosed);
            return;
        }

        // Chunked or read-until-close body: whatever arrived is the body
        FinishSuccess(transfer);
    }

    void SocketHttpBackend::FinishSuccess(Transfer* transfer)
    {
        auto request = transfer->request;

        request->status = transfer->status;
        request->responseHeaderLines = transfer->headerLines;

        if (!transfer->noBody)
        {
            const char* bodyData = transfer->received.data() + transfer->bodyBegin;
            size_t bodySize = transfer->received.size() - transfer->bodyBegin;

            if (transfer->chunked)
            {
                String body;
                if (!DecodeChunkedBody(bodyData, bodySize, body))
                {
                    FinishError(transfer, HttpError::ConnectionClosed);
                    return;
                }

                request->responseBody = body;
            }
            else if (transfer->contentLength >= 0)
                request->responseBody.assign(bodyData, (size_t)transfer->contentLength);
            else
                request->responseBody.assign(bodyData, bodySize);
        }

        transfer->finished = true;
        RemoveTransfer(transfer);
        request->done.Store(1);
    }

    void SocketHttpBackend::FinishError(Transfer* transfer, HttpError error)
    {
        auto request = transfer->request;

        transfer->finished = true;
        RemoveTransfer(transfer);

        request->error = error;
        request->done.Store(1);
    }

    bool SocketHttpBackend::ParseHeaders(Transfer* transfer)
    {
        size_t headersEnd = transfer->received.find("\r\n\r\n");
        transfer->bodyBegin = headersEnd + 4;

        size_t lineBegin = 0;
        bool firstLine = true;
        while (lineBegin < headersEnd)
        {
            size_t lineEnd = transfer->received.find("\r\n", lineBegin);
            if (lineEnd == std::string::npos || lineEnd > headersEnd)
                lineEnd = headersEnd;

            String line = transfer->received.substr(lineBegin, lineEnd - lineBegin);
            lineBegin = lineEnd + 2;

            if (firstLine)
            {
                // Status line: "HTTP/1.1 200 OK"
                size_t firstSpace = line.find(' ');
                if (firstSpace == std::string::npos)
                    return false;

                transfer->status = atoi(line.Data() + firstSpace + 1);
                if (transfer->status < 100)
                    return false;

                firstLine = false;
                continue;
            }

            if (!line.IsEmpty())
                transfer->headerLines.Add(line);
        }

        transfer->headersParsed = true;

        if (transfer->status < 200 || transfer->status == 204 || transfer->status == 304)
            transfer->noBody = true;

        String contentLength;
        String transferEncoding;
        for (auto& line : transfer->headerLines)
        {
            String lower = line;
            for (size_t i = 0; i < lower.size(); i++)
            {
                if (lower[i] >= 'A' && lower[i] <= 'Z')
                    lower[i] = (char)(lower[i] - 'A' + 'a');
            }

            if (lower.compare(0, 15, "content-length:") == 0)
                contentLength = line.substr(15);
            else if (lower.compare(0, 18, "transfer-encoding:") == 0)
                transferEncoding = lower.substr(18);
        }

        if (!contentLength.IsEmpty())
            transfer->contentLength = atoi(contentLength.Data());

        if (transferEncoding.find("chunked") != std::string::npos)
        {
            transfer->chunked = true;
            transfer->contentLength = -1;
        }

        return true;
    }

    void SocketHttpBackend::RemoveTransfer(Transfer* transfer)
    {
        if (transfer->socket)
            transfer->socket->Close();

        mTransfers.RemoveAll([&](const Ref<Transfer>& other) { return other.Get() == transfer; });
    }
}
