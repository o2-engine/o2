#include "o2/stdafx.h"
#include "TestHttpServer.h"

#include "o2/Utils/Bitmap/Bitmap.h"
#include "o2/Utils/FileSystem/FileSystem.h"
#include "o2/Utils/FileSystem/File.h"

namespace o2
{
    TestHttpServer::TestHttpServer(RefCounter* refCounter):
        RefCounterable(refCounter)
    {}

    TestHttpServer::~TestHttpServer()
    {
        Stop();
    }

    bool TestHttpServer::Start()
    {
        mListener = mmake<TcpListener>();
        mListener->onAccepted = [this](const Ref<TcpSocket>& socket)
        {
            auto connection = mmake<Connection>();
            connection->socket = socket;
            mConnections.Add(connection);

            Connection* connectionPtr = connection.Get();
            socket->onDataReceived = [this, connectionPtr](const String& data)
            {
                connectionPtr->buffer.append(data.data(), data.size());
                ProcessConnection(connectionPtr);
            };

            socket->onClosed = [this, connectionPtr]
            {
                mConnections.RemoveAll([&](const Ref<Connection>& other) { return other.Get() == connectionPtr; });
            };
        };

        return mListener->Listen(0);
    }

    int TestHttpServer::GetPort() const
    {
        return mListener ? mListener->GetLocalPort() : 0;
    }

    String TestHttpServer::GetBaseUrl() const
    {
        return "http://127.0.0.1:" + String(GetPort());
    }

    void TestHttpServer::Stop()
    {
        if (mListener)
            mListener->Close();

        auto connections = mConnections;
        mConnections.Clear();
        for (auto& connection : connections)
            connection->socket->Close();

        mListener = nullptr;
    }

    String TestHttpServer::GetTestImagePng()
    {
        // A small saved-and-read-back PNG: Bitmap has no in-memory encoder
        static String cached;
        if (!cached.IsEmpty())
            return cached;

        Bitmap bitmap(PixelFormat::R8G8B8A8, Vec2I(16, 16));
        bitmap.Fill(Color4(255, 64, 32, 255));

        String tempPath = "TestHttpServerImage.png";
        bitmap.Save(tempPath, Bitmap::ImageType::Png);

        InFile file(tempPath);
        if (file.IsOpened())
        {
            UInt size = file.GetDataSize();
            cached.resize((size_t)size);
            file.ReadData(&cached[0], size);
            file.Close();
        }

        o2FileSystem.FileDelete(tempPath);
        return cached;
    }

    void TestHttpServer::ProcessConnection(Connection* connection)
    {
        while (true)
        {
            size_t headersEnd = connection->buffer.find("\r\n\r\n");
            if (headersEnd == std::string::npos)
                return;

            // Request line
            size_t lineEnd = connection->buffer.find("\r\n");
            String requestLine = connection->buffer.substr(0, lineEnd);

            size_t methodEnd = requestLine.find(' ');
            size_t pathEnd = requestLine.find(' ', methodEnd + 1);
            if (methodEnd == std::string::npos || pathEnd == std::string::npos)
            {
                connection->socket->Close();
                return;
            }

            String method = requestLine.substr(0, methodEnd);
            String path = requestLine.substr(methodEnd + 1, pathEnd - methodEnd - 1);

            // Header lines
            Vector<String> headerLines;
            size_t lineBegin = lineEnd + 2;
            while (lineBegin < headersEnd)
            {
                size_t end = connection->buffer.find("\r\n", lineBegin);
                if (end == std::string::npos || end > headersEnd)
                    end = headersEnd;

                headerLines.Add(connection->buffer.substr(lineBegin, end - lineBegin));
                lineBegin = end + 2;
            }

            int contentLength = 0;
            String contentLengthValue = FindHeader(headerLines, "Content-Length");
            if (!contentLengthValue.IsEmpty())
                contentLength = atoi(contentLengthValue.Data());

            size_t requestEnd = headersEnd + 4 + (size_t)contentLength;
            if (connection->buffer.size() < requestEnd)
                return;

            String body = connection->buffer.substr(headersEnd + 4, (size_t)contentLength);
            connection->buffer.erase(0, requestEnd);

            String response = HandleRequest(method, path, headerLines, body);
            if (!response.IsEmpty())
                connection->socket->Send(response);
        }
    }

    String TestHttpServer::HandleRequest(const String& method, const String& path,
                                         const Vector<String>& headerLines, const String& body)
    {
        lastMethod = method;

        String cookieHeader = FindHeader(headerLines, "Cookie");
        lastCookieHeader = cookieHeader.IsEmpty() ? String("-") : cookieHeader;

        if (path == "/echo")
        {
            echoHits++;

            Vector<String> extra;
            String testHeader = FindHeader(headerLines, "X-Test");
            if (!testHeader.IsEmpty())
                extra.Add("X-Echo-Header: " + testHeader);

            extra.Add("Content-Type: text/plain");

            String responseBody = method + "|" + body;
            bool headOnly = method == "HEAD";
            if (headOnly)
            {
                // HEAD: full headers, no body bytes
                String response = "HTTP/1.1 200 OK\r\nContent-Length: " + String((int)responseBody.size()) + "\r\n";
                for (auto& header : extra)
                    response += header + "\r\n";
                return response + "\r\n";
            }

            return MakeResponse(200, "OK", extra, responseBody);
        }

        if (path == "/json")
        {
            Vector<String> extra;
            extra.Add("Content-Type: application/json");
            return MakeResponse(200, "OK", extra, "{\"name\":\"o2\",\"value\":42,\"items\":[1,2,3]}");
        }

        if (path == "/image")
        {
            Vector<String> extra;
            extra.Add("Content-Type: image/png");
            return MakeResponse(200, "OK", extra, GetTestImagePng());
        }

        if (path == "/status/404")
            return MakeResponse(404, "Not Found", {}, "not found");

        if (path == "/redirect")
        {
            Vector<String> extra;
            extra.Add("Location: /echo");
            return MakeResponse(302, "Found", extra, "");
        }

        if (path == "/redirect-loop")
        {
            Vector<String> extra;
            extra.Add("Location: /redirect-loop");
            return MakeResponse(302, "Found", extra, "");
        }

        if (path == "/slow")
            return String(); // Never respond, for timeout tests

        if (path == "/chunked")
        {
            return "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nTransfer-Encoding: chunked\r\n\r\n"
                   "5\r\nhello\r\n6\r\n world\r\n0\r\n\r\n";
        }

        if (path == "/cookies/set")
        {
            Vector<String> extra;
            extra.Add("Set-Cookie: session=abc123; Path=/");
            extra.Add("Set-Cookie: theme=dark; Path=/; Max-Age=3600");
            return MakeResponse(200, "OK", extra, "cookies set");
        }

        if (path == "/cookies/show")
            return MakeResponse(200, "OK", {}, lastCookieHeader);

        if (path == "/cache")
        {
            String noneMatch = FindHeader(headerLines, "If-None-Match");
            if (!noneMatch.IsEmpty() && noneMatch == cacheEtag)
            {
                cacheHits++;
                Vector<String> extra;
                extra.Add("ETag: " + cacheEtag);
                return "HTTP/1.1 304 Not Modified\r\nETag: " + cacheEtag + "\r\nContent-Length: 0\r\n\r\n";
            }

            cacheHits++;
            Vector<String> extra;
            extra.Add("Cache-Control: max-age=60");
            extra.Add("ETag: " + cacheEtag);
            return MakeResponse(200, "OK", extra, "cache hit " + String(cacheHits));
        }

        if (path == "/cache-revalidate")
        {
            String noneMatch = FindHeader(headerLines, "If-None-Match");
            if (!noneMatch.IsEmpty() && noneMatch == cacheEtag)
            {
                cacheHits++;
                return "HTTP/1.1 304 Not Modified\r\nETag: " + cacheEtag + "\r\nContent-Length: 0\r\n\r\n";
            }

            cacheHits++;
            Vector<String> extra;
            extra.Add("Cache-Control: max-age=0");
            extra.Add("ETag: " + cacheEtag);
            return MakeResponse(200, "OK", extra, "revalidated content");
        }

        if (path == "/big")
        {
            String big;
            big.resize(1024 * 1024);
            for (size_t i = 0; i < big.size(); i++)
                big[i] = (char)('a' + (i % 23));

            return MakeResponse(200, "OK", {}, big);
        }

        return MakeResponse(404, "Not Found", {}, "unknown path");
    }

    String TestHttpServer::FindHeader(const Vector<String>& headerLines, const String& name)
    {
        for (auto& line : headerLines)
        {
            if (line.size() <= name.size() + 1 || line[name.size()] != ':')
                continue;

            bool equal = true;
            for (size_t i = 0; i < name.size() && equal; i++)
            {
                char a = line[i], b = name[i];
                if (a >= 'A' && a <= 'Z') a = (char)(a - 'A' + 'a');
                if (b >= 'A' && b <= 'Z') b = (char)(b - 'A' + 'a');
                equal = a == b;
            }

            if (!equal)
                continue;

            size_t valueBegin = line.find_first_not_of(" \t", name.size() + 1);
            return valueBegin == std::string::npos ? String() : (String)line.substr(valueBegin);
        }

        return String();
    }

    String TestHttpServer::MakeResponse(int status, const String& statusText, const Vector<String>& extraHeaders,
                                        const String& body)
    {
        String response = "HTTP/1.1 " + String(status) + " " + statusText + "\r\n";
        response += "Content-Length: " + String((int)body.size()) + "\r\n";

        for (auto& header : extraHeaders)
            response += header + "\r\n";

        response += "\r\n";
        response.append(body.data(), body.size());
        return response;
    }
}
