#include "o2/stdafx.h"
#include "SocketPlatform.h"

#include <string.h>

#ifdef PLATFORM_WINDOWS
// Uses the winsock 1.1 API already pulled in by windows.h: including winsock2.h after windows.h
// clashes with the sockaddr definitions from the precompiled header
#include <windows.h>
#else
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#endif

namespace o2
{
#ifdef PLATFORM_WINDOWS
    typedef SOCKET NativeSocket;
#else
    typedef int NativeSocket;
#endif

#ifdef PLATFORM_WINDOWS
    static bool IsWouldBlock() { int e = WSAGetLastError(); return e == WSAEWOULDBLOCK; }
    static bool IsInProgress() { int e = WSAGetLastError(); return e == WSAEWOULDBLOCK || e == WSAEINPROGRESS; }
#else
    static bool IsWouldBlock() { return errno == EWOULDBLOCK || errno == EAGAIN; }
    static bool IsInProgress() { return errno == EINPROGRESS; }
#endif

    String SocketAddress::GetAddressString() const
    {
        if (!IsValid())
            return String();

        const sockaddr* addr = (const sockaddr*)data;
        if (addr->sa_family == AF_INET)
        {
            const sockaddr_in* addr4 = (const sockaddr_in*)addr;
            UInt32 ip = ntohl(addr4->sin_addr.s_addr);
            return String((int)((ip >> 24) & 0xff)) + "." + String((int)((ip >> 16) & 0xff)) + "." +
                String((int)((ip >> 8) & 0xff)) + "." + String((int)(ip & 0xff));
        }

#ifndef PLATFORM_WINDOWS
        if (addr->sa_family == AF_INET6)
        {
            char buffer[64] = { 0 };
            inet_ntop(AF_INET6, &((const sockaddr_in6*)addr)->sin6_addr, buffer, sizeof(buffer));
            return String(buffer);
        }
#endif

        return String();
    }

    int SocketAddress::GetPort() const
    {
        if (!IsValid())
            return 0;

        const sockaddr* addr = (const sockaddr*)data;
        if (addr->sa_family == AF_INET)
            return (int)ntohs(((const sockaddr_in*)addr)->sin_port);

#ifndef PLATFORM_WINDOWS
        if (addr->sa_family == AF_INET6)
            return (int)ntohs(((const sockaddr_in6*)addr)->sin6_port);
#endif

        return 0;
    }

    bool SocketAddress::operator==(const SocketAddress& other) const
    {
        return size == other.size && memcmp(data, other.data, (size_t)size) == 0;
    }

    namespace SocketPlatform
    {
        void Initialize()
        {
#ifdef PLATFORM_WINDOWS
            static bool initialized = false;
            if (!initialized)
            {
                WSADATA wsaData;
                initialized = WSAStartup(MAKEWORD(2, 2), &wsaData) == 0;
            }
#endif
        }

        void Deinitialize()
        {}

        static bool SetNonBlocking(SocketHandle handle)
        {
#ifdef PLATFORM_WINDOWS
            u_long mode = 1;
            return ioctlsocket((SOCKET)handle, FIONBIO, &mode) == 0;
#else
            int flags = fcntl((int)handle, F_GETFL, 0);
            return flags >= 0 && fcntl((int)handle, F_SETFL, flags | O_NONBLOCK) >= 0;
#endif
        }

        SocketHandle CreateSocket(bool udp, const SocketAddress& address)
        {
            int family = address.IsValid() ? ((const sockaddr*)address.data)->sa_family : AF_INET;

#ifdef PLATFORM_WINDOWS
            SOCKET handle = socket(family, udp ? SOCK_DGRAM : SOCK_STREAM, 0);
            if (handle == INVALID_SOCKET)
                return InvalidSocketHandle;
#else
            int handle = socket(family, udp ? SOCK_DGRAM : SOCK_STREAM, 0);
            if (handle < 0)
                return InvalidSocketHandle;
#endif

            if (!SetNonBlocking((SocketHandle)handle))
            {
                Close((SocketHandle)handle);
                return InvalidSocketHandle;
            }

#ifndef PLATFORM_WINDOWS
            // A crashed peer must not kill the process with SIGPIPE on send
#ifdef SO_NOSIGPIPE
            int noSigpipe = 1;
            setsockopt((int)handle, SOL_SOCKET, SO_NOSIGPIPE, &noSigpipe, sizeof(noSigpipe));
#endif
#endif

            return (SocketHandle)handle;
        }

        void Close(SocketHandle handle)
        {
            if (handle == InvalidSocketHandle)
                return;

#ifdef PLATFORM_WINDOWS
            closesocket((SOCKET)handle);
#else
            close((int)handle);
#endif
        }

        bool ResolveSocketAddress(const String& host, int port, SocketAddress& outAddress)
        {
            outAddress.size = 0;

#ifdef PLATFORM_WINDOWS
            // The winsock 1.1 API has no getaddrinfo; resolve IPv4 only
            sockaddr_in addr4;
            memset(&addr4, 0, sizeof(addr4));
            addr4.sin_family = AF_INET;
            addr4.sin_port = htons((u_short)port);

            if (host.IsEmpty())
                addr4.sin_addr.s_addr = htonl(INADDR_ANY);
            else
            {
                unsigned long ip = inet_addr(host.Data());
                if (ip == INADDR_NONE)
                {
                    hostent* entry = gethostbyname(host.Data());
                    if (!entry || !entry->h_addr_list[0])
                        return false;

                    memcpy(&addr4.sin_addr, entry->h_addr_list[0], sizeof(addr4.sin_addr));
                }
                else
                    addr4.sin_addr.s_addr = ip;
            }

            memcpy(outAddress.data, &addr4, sizeof(addr4));
            outAddress.size = sizeof(addr4);
            return true;
#else
            addrinfo hints;
            memset(&hints, 0, sizeof(hints));
            hints.ai_family = AF_UNSPEC;
            hints.ai_socktype = SOCK_STREAM;
            if (host.IsEmpty())
                hints.ai_flags = AI_PASSIVE;

            String portString((int)port);
            addrinfo* results = nullptr;
            if (getaddrinfo(host.IsEmpty() ? nullptr : host.Data(), portString.Data(), &hints, &results) != 0 || !results)
                return false;

            // Prefer IPv4, fall back to the first result
            addrinfo* chosen = results;
            for (addrinfo* it = results; it; it = it->ai_next)
            {
                if (it->ai_family == AF_INET)
                {
                    chosen = it;
                    break;
                }
            }

            if ((int)chosen->ai_addrlen <= (int)sizeof(outAddress.data))
            {
                memcpy(outAddress.data, chosen->ai_addr, chosen->ai_addrlen);
                outAddress.size = (int)chosen->ai_addrlen;
            }

            freeaddrinfo(results);
            return outAddress.IsValid();
#endif
        }

        bool ConnectSocket(SocketHandle handle, const SocketAddress& address)
        {
            int result = connect((NativeSocket)handle, (const sockaddr*)address.data, address.size);
            if (result == 0)
                return true;

            return IsInProgress() || IsWouldBlock();
        }

        int CheckConnectFinished(SocketHandle handle)
        {
            fd_set writeSet;
            fd_set errorSet;
            FD_ZERO(&writeSet);
            FD_ZERO(&errorSet);
            FD_SET((NativeSocket)handle, &writeSet);
            FD_SET((NativeSocket)handle, &errorSet);

            timeval zeroTimeout = { 0, 0 };
            int selected = select((int)handle + 1, nullptr, &writeSet, &errorSet, &zeroTimeout);
            if (selected < 0)
                return -1;

            if (selected == 0)
                return 0;

            int error = 0;
#ifdef PLATFORM_WINDOWS
            int errorSize = sizeof(error);
            getsockopt((SOCKET)handle, SOL_SOCKET, SO_ERROR, (char*)&error, &errorSize);
#else
            socklen_t errorSize = sizeof(error);
            getsockopt((int)handle, SOL_SOCKET, SO_ERROR, &error, &errorSize);
#endif

            return error == 0 ? 1 : -1;
        }

        bool BindSocket(SocketHandle handle, const SocketAddress& address)
        {
#ifndef PLATFORM_WINDOWS
            int reuse = 1;
            setsockopt((int)handle, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
#endif
            return bind((NativeSocket)handle, (const sockaddr*)address.data, address.size) == 0;
        }

        bool ListenSocket(SocketHandle handle)
        {
            return listen((NativeSocket)handle, 16) == 0;
        }

        SocketHandle AcceptSocket(SocketHandle handle, SocketAddress& outAddress)
        {
            outAddress.size = 0;

#ifdef PLATFORM_WINDOWS
            int addressSize = sizeof(outAddress.data);
            SOCKET accepted = accept((SOCKET)handle, (sockaddr*)outAddress.data, &addressSize);
            if (accepted == INVALID_SOCKET)
                return InvalidSocketHandle;
#else
            socklen_t addressSize = sizeof(outAddress.data);
            int accepted = accept((int)handle, (sockaddr*)outAddress.data, &addressSize);
            if (accepted < 0)
                return InvalidSocketHandle;
#endif

            outAddress.size = (int)addressSize;

            if (!SetNonBlocking((SocketHandle)accepted))
            {
                Close((SocketHandle)accepted);
                return InvalidSocketHandle;
            }

            return (SocketHandle)accepted;
        }

        int SendData(SocketHandle handle, const void* data, int size)
        {
            int flags = 0;
#ifdef MSG_NOSIGNAL
            flags = MSG_NOSIGNAL;
#endif
            int sent = (int)send((NativeSocket)handle, (const char*)data, size, flags);
            if (sent >= 0)
                return sent;

            return IsWouldBlock() ? 0 : -1;
        }

        int ReceiveData(SocketHandle handle, void* buffer, int size)
        {
            int received = (int)recv((NativeSocket)handle, (char*)buffer, size, 0);
            if (received > 0)
                return received;

            if (received == 0)
                return -1; // Orderly remote close

            return IsWouldBlock() ? 0 : -1;
        }

        bool SendDatagram(SocketHandle handle, const SocketAddress& address, const void* data, int size)
        {
            int sent = (int)sendto((NativeSocket)handle, (const char*)data, size, 0,
                                   (const sockaddr*)address.data, address.size);
            return sent == size;
        }

        int ReceiveDatagram(SocketHandle handle, void* buffer, int size, SocketAddress& outAddress)
        {
            outAddress.size = 0;

#ifdef PLATFORM_WINDOWS
            int addressSize = sizeof(outAddress.data);
#else
            socklen_t addressSize = sizeof(outAddress.data);
#endif
            int received = (int)recvfrom((NativeSocket)handle, (char*)buffer, size, 0,
                                         (sockaddr*)outAddress.data, &addressSize);
            if (received >= 0)
            {
                outAddress.size = (int)addressSize;
                return received;
            }

            return IsWouldBlock() ? 0 : -1;
        }

        int GetLocalPort(SocketHandle handle)
        {
            SocketAddress address;
#ifdef PLATFORM_WINDOWS
            int addressSize = sizeof(address.data);
            if (getsockname((SOCKET)handle, (sockaddr*)address.data, &addressSize) != 0)
                return 0;
#else
            socklen_t addressSize = sizeof(address.data);
            if (getsockname((int)handle, (sockaddr*)address.data, &addressSize) != 0)
                return 0;
#endif
            address.size = (int)addressSize;
            return address.GetPort();
        }

        bool ConnectDatagramSocket(SocketHandle handle, const SocketAddress& address)
        {
            return connect((NativeSocket)handle, (const sockaddr*)address.data, address.size) == 0;
        }
    }
}
