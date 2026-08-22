#pragma once

#include "o2/Network/Sockets/TcpListener.h"
#include "o2/Network/Sockets/TcpSocket.h"
#include "o2/Utils/Types/Containers/Vector.h"
#include "o2/Utils/Types/Ref.h"
#include "o2/Utils/Types/String.h"

namespace o2
{
    // ------------------------------------------------------------------------------------------
    // Minimal HTTP/1.1 test server over the engine's own TCP sockets, pumped by the same network
    // pump as the client under test. Serves the fixed endpoints the HTTP tests exercise
    // ------------------------------------------------------------------------------------------
    class TestHttpServer: public RefCounterable
    {
    public:
        int    cacheHits = 0;         // /cache endpoint hit counter, 304 revalidations included
        int    echoHits = 0;          // /echo endpoint hit counter
        String lastCookieHeader;      // Cookie header of the last request, "-" when absent
        String lastMethod;            // Method of the last request
        String cacheEtag = "\"v1\"";  // ETag served by /cache

        // Default constructor
        explicit TestHttpServer(RefCounter* refCounter);

        // Destructor, stops the server
        ~TestHttpServer() override;

        // Starts listening on an ephemeral port
        bool Start();

        // Returns the bound port
        int GetPort() const;

        // Returns the base URL, e.g. "http://127.0.0.1:12345"
        String GetBaseUrl() const;

        // Stops the server and drops all connections
        void Stop();

        // Returns PNG bytes of a small test image
        static String GetTestImagePng();

    protected:
        // One accepted connection with its stream buffer
        struct Connection: public RefCounterable
        {
            Ref<TcpSocket> socket; // Client connection
            String         buffer; // Received bytes not yet parsed
        };

        Ref<TcpListener>        mListener;    // Listening socket
        Vector<Ref<Connection>> mConnections; // Live connections

    protected:
        // Parses whole requests from the connection buffer and responds
        void ProcessConnection(Connection* connection);

        // Builds the response for one parsed request
        String HandleRequest(const String& method, const String& path, const Vector<String>& headerLines,
                             const String& body);

        // Returns the value of the header from raw lines, case-insensitive, empty when absent
        static String FindHeader(const Vector<String>& headerLines, const String& name);

        // Builds a whole response with Content-Length
        static String MakeResponse(int status, const String& statusText, const Vector<String>& extraHeaders,
                                   const String& body);
    };
}
