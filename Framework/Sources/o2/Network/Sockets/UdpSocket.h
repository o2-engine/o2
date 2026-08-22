#pragma once

#include "o2/Network/Sockets/SocketPlatform.h"
#include "o2/Utils/Basic/IObject.h"
#include "o2/Utils/Coroutines/Coroutines.h"
#include "o2/Utils/Function/Function.h"
#include "o2/Utils/Types/Containers/Map.h"
#include "o2/Utils/Types/Ref.h"
#include "o2/Utils/Types/String.h"

namespace o2
{
    // Received UDP datagram: payload and sender endpoint
    struct UdpDatagram
    {
        String data;    // Payload bytes
        String address; // Sender address
        int    port = 0; // Sender port

        // Returns true when all parts are equal
        bool operator==(const UdpDatagram& other) const
        {
            return data == other.data && address == other.address && port == other.port;
        }
    };

    // -------------------------------------------------------------------------------------------
    // UDP socket for datagram exchange, the base transport for realtime gameplay. Non-blocking,
    // pumped by the network system every frame on the main thread. Optionally connected to a
    // default remote endpoint with Connect, so plain Send works. Payloads are o2::String
    // -------------------------------------------------------------------------------------------
    class UdpSocket: public IObject, public RefCounterable
    {
    public:
        Function<void(const String& data, const String& address, int port)> onDataReceived; // Called for every received datagram @SCRIPTABLE

    public:
        // Default constructor @SCRIPTABLE
        explicit UdpSocket(RefCounter* refCounter);

        // Destructor, closes the socket
        ~UdpSocket() override;

        // Opens the socket on the local port. Port 0 binds an ephemeral port, see GetLocalPort.
        // Returns false on failure @SCRIPTABLE
        bool Open(int localPort = 0);

        // Sets the default remote endpoint for Send. Blocking for non-numeric hosts.
        // Returns false on failure @SCRIPTABLE
        bool Connect(const String& address, int port);

        // Sends a datagram to the default remote endpoint set by Connect @SCRIPTABLE
        bool Send(const String& data);

        // Sends raw bytes to the default remote endpoint
        bool Send(const void* data, int size);

        // Sends a datagram to the endpoint @SCRIPTABLE
        bool SendTo(const String& address, int port, const String& data);

        // Sends raw bytes to the endpoint
        bool SendTo(const String& address, int port, const void* data, int size);

        // Coroutine receive: completes with the next received datagram, or an empty one when
        // the socket is closed. Pending awaiters consume datagrams before onDataReceived
        Coroutine<UdpDatagram> ReceiveAsync();

        // Returns the port the socket is bound to, 0 when closed @SCRIPTABLE
        int GetLocalPort() const;

        // Returns true while the socket is open @SCRIPTABLE
        bool IsOpened() const;

        // Closes the socket. Pending awaiters complete with empty datagrams @SCRIPTABLE
        void Close();

        IOBJECT(UdpSocket);

    protected:
        SocketHandle mHandle = InvalidSocketHandle; // Native socket handle
        bool         mOpened = false;               // True while the socket is open
        int          mLocalPort = 0;                // Bound port
        bool         mHasDefaultRemote = false;     // True once Connect set the default remote

        Map<String, SocketAddress> mResolvedAddresses; // SendTo resolve cache, key is "address:port"

        Vector<Function<void(const UdpDatagram&)>> mReceiveWaiters;   // One-shot receive completions
        Vector<UdpDatagram>                        mPendingDatagrams; // Datagrams received while there was no waiter and no onDataReceived subscriber

    protected:
        // Resolves the endpoint through the cache
        bool ResolveCached(const String& address, int port, SocketAddress& outAddress);

        // Adds a one-shot receive completion; fires immediately with an empty datagram when closed
        void AddReceiveWaiter(const Function<void(const UdpDatagram&)>& waiter);

        // Pumps pending datagrams. Called by the network system
        void UpdateSocket(float dt);

        friend class NetworkSystem;
    };
}
// --- META ---

CLASS_BASES_META(o2::UdpSocket)
{
    BASE_CLASS(o2::IObject);
    BASE_CLASS(o2::RefCounterable);
}
END_META;
CLASS_FIELDS_META(o2::UdpSocket)
{
    FIELD().PUBLIC().SCRIPTABLE_ATTRIBUTE().NAME(onDataReceived);
    FIELD().PROTECTED().DEFAULT_VALUE(InvalidSocketHandle).NAME(mHandle);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mOpened);
    FIELD().PROTECTED().DEFAULT_VALUE(0).NAME(mLocalPort);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mHasDefaultRemote);
    FIELD().PROTECTED().NAME(mResolvedAddresses);
    FIELD().PROTECTED().NAME(mReceiveWaiters);
    FIELD().PROTECTED().NAME(mPendingDatagrams);
}
END_META;
CLASS_METHODS_META(o2::UdpSocket)
{

    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(bool, Open, int);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(bool, Connect, const String&, int);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(bool, Send, const String&);
    FUNCTION().PUBLIC().SIGNATURE(bool, Send, const void*, int);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(bool, SendTo, const String&, int, const String&);
    FUNCTION().PUBLIC().SIGNATURE(bool, SendTo, const String&, int, const void*, int);
    FUNCTION().PUBLIC().SIGNATURE(Coroutine<UdpDatagram>, ReceiveAsync);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(int, GetLocalPort);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(bool, IsOpened);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, Close);
    FUNCTION().PROTECTED().SIGNATURE(bool, ResolveCached, const String&, int, SocketAddress&);
    FUNCTION().PROTECTED().SIGNATURE(void, AddReceiveWaiter, const Function<void(const UdpDatagram&)>&);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateSocket, float);
}
END_META;
// --- END META ---
