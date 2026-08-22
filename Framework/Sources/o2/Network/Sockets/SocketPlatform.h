#pragma once

#include "o2/Utils/Types/CommonTypes.h"
#include "o2/Utils/Types/String.h"

namespace o2
{
    // Native socket handle, wide enough for POSIX int and WinSock SOCKET
    typedef intptr_t SocketHandle;

    constexpr SocketHandle InvalidSocketHandle = -1;

    // Resolved socket address, opaque sockaddr storage. Filled by ResolveSocketAddress
    struct SocketAddress
    {
        UInt8 data[128]; // sockaddr storage bytes
        int   size = 0;  // Used sockaddr size, 0 when not resolved

        // Returns true when the address was resolved
        bool IsValid() const { return size > 0; }

        // Returns printable address string, empty when invalid
        String GetAddressString() const;

        // Returns port in host byte order, 0 when invalid
        int GetPort() const;

        // Returns true when the addresses are byte-equal
        bool operator==(const SocketAddress& other) const;
    };

    // Internal platform layer under the socket classes: creation, non-blocking mode, readiness
    // checks and address resolution. Not a public engine API
    namespace SocketPlatform
    {
        // Initializes the platform socket layer (WSAStartup on Windows). Safe to call repeatedly
        void Initialize();

        // Deinitializes the platform socket layer
        void Deinitialize();

        // Creates a non-blocking TCP or UDP socket for the address family, InvalidSocketHandle on failure
        SocketHandle CreateSocket(bool udp, const SocketAddress& address);

        // Closes the socket handle
        void Close(SocketHandle handle);

        // Resolves host name or numeric address and port. Blocking for non-numeric hosts.
        // When host is empty resolves the any-address for binding. Returns false on failure
        bool ResolveSocketAddress(const String& host, int port, SocketAddress& outAddress);

        // Begins a non-blocking connect. Returns false on immediate failure
        bool ConnectSocket(SocketHandle handle, const SocketAddress& address);

        // Returns 1 when the non-blocking connect finished successfully, 0 when still in progress,
        // -1 when it failed
        int CheckConnectFinished(SocketHandle handle);

        // Binds the socket to the address. Returns false on failure
        bool BindSocket(SocketHandle handle, const SocketAddress& address);

        // Starts listening on a bound socket. Returns false on failure
        bool ListenSocket(SocketHandle handle);

        // Accepts a pending connection, InvalidSocketHandle when none pending. The accepted
        // socket is switched to non-blocking mode
        SocketHandle AcceptSocket(SocketHandle handle, SocketAddress& outAddress);

        // Sends bytes. Returns sent count, 0 when the socket buffer is full, -1 on error
        int SendData(SocketHandle handle, const void* data, int size);

        // Receives bytes. Returns received count, 0 when nothing pending, -1 on error or
        // orderly remote close
        int ReceiveData(SocketHandle handle, void* buffer, int size);

        // Sends a datagram to the address. Returns false on error
        bool SendDatagram(SocketHandle handle, const SocketAddress& address, const void* data, int size);

        // Receives a datagram. Returns its size, 0 when nothing pending, -1 on error
        int ReceiveDatagram(SocketHandle handle, void* buffer, int size, SocketAddress& outAddress);

        // Returns the local port the socket is bound to, 0 on failure
        int GetLocalPort(SocketHandle handle);

        // Sets the default remote address of a UDP socket, so plain SendData works. Returns false on failure
        bool ConnectDatagramSocket(SocketHandle handle, const SocketAddress& address);
    }
}
