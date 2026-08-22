#pragma once

#include "o2/Network/Sockets/TcpSocket.h"
#include "o2/Utils/Basic/IObject.h"

namespace o2
{
    // -------------------------------------------------------------------------------------------
    // Message-oriented TCP connection for chats, lobbies and other whole-message exchanges. Wraps
    // a TcpSocket and splits the stream into length-prefixed messages, so every onMessage carries
    // exactly one sent message. Messages are o2::String, which is binary-safe. The framing is a
    // 4-byte little-endian payload size, compatible with TcpMessageServer
    // -------------------------------------------------------------------------------------------
    class TcpMessageChannel: public IObject, public RefCounterable
    {
    public:
        Function<void(bool success)>          onConnected; // Called once when the connect attempt finishes @SCRIPTABLE
        Function<void(const String& message)> onMessage;   // Called for every received whole message @SCRIPTABLE
        Function<void()>                      onClosed;    // Called when the connection is closed remotely or by error @SCRIPTABLE

        int maxMessageSize = 16*1024*1024; // Incoming message size limit; a bigger prefix closes the connection

    public:
        // Default constructor @SCRIPTABLE
        explicit TcpMessageChannel(RefCounter* refCounter);

        // Destructor, closes the connection
        ~TcpMessageChannel() override;

        // Begins an asynchronous connect to the host and port. The result is reported through
        // onConnected and pending ConnectAsync awaiters. Returns false on immediate failure @SCRIPTABLE
        bool Connect(const String& host, int port);

        // Coroutine connect: starts the connect and completes with its result
        Coroutine<bool> ConnectAsync(const String& host, int port);

        // Sends one message. Buffered while connecting @SCRIPTABLE
        void Send(const String& message);

        // Coroutine receive: completes with the next whole message, or an empty string when the
        // connection is closed. Pending awaiters consume messages before onMessage
        Coroutine<String> ReceiveAsync();

        // Closes the connection @SCRIPTABLE
        void Close();

        // Returns true while the connect attempt is in progress @SCRIPTABLE
        bool IsConnecting() const;

        // Returns true when connected @SCRIPTABLE
        bool IsConnected() const;

        IOBJECT(TcpMessageChannel);

    protected:
        Ref<TcpSocket> mSocket;        // Underlying stream socket
        String         mReceiveBuffer; // Stream bytes not yet split into messages

        Vector<Function<void(const String&)>> mMessageWaiters; // One-shot message completions

    protected:
        // Wraps an already connected socket accepted by a server
        void InitializeAccepted(const Ref<TcpSocket>& socket);

        // Subscribes to the socket events
        void BindSocketEvents();

        // Adds a one-shot message completion; fires immediately with an empty message when closed
        void AddMessageWaiter(const Function<void(const String&)>& waiter);

        // Splits buffered stream bytes into whole messages and dispatches them
        void ProcessReceivedData(const String& data);

        // Completes all pending waiters with empty messages
        void FailMessageWaiters();

        friend class TcpMessageServer;
    };
}
// --- META ---

CLASS_BASES_META(o2::TcpMessageChannel)
{
    BASE_CLASS(o2::IObject);
    BASE_CLASS(o2::RefCounterable);
}
END_META;
CLASS_FIELDS_META(o2::TcpMessageChannel)
{
    FIELD().PUBLIC().SCRIPTABLE_ATTRIBUTE().NAME(onConnected);
    FIELD().PUBLIC().SCRIPTABLE_ATTRIBUTE().NAME(onMessage);
    FIELD().PUBLIC().SCRIPTABLE_ATTRIBUTE().NAME(onClosed);
    FIELD().PUBLIC().DEFAULT_VALUE(16*1024*1024).NAME(maxMessageSize);
    FIELD().PROTECTED().NAME(mSocket);
    FIELD().PROTECTED().NAME(mReceiveBuffer);
    FIELD().PROTECTED().NAME(mMessageWaiters);
}
END_META;
CLASS_METHODS_META(o2::TcpMessageChannel)
{

    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(bool, Connect, const String&, int);
    FUNCTION().PUBLIC().SIGNATURE(Coroutine<bool>, ConnectAsync, const String&, int);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, Send, const String&);
    FUNCTION().PUBLIC().SIGNATURE(Coroutine<String>, ReceiveAsync);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, Close);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(bool, IsConnecting);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(bool, IsConnected);
    FUNCTION().PROTECTED().SIGNATURE(void, InitializeAccepted, const Ref<TcpSocket>&);
    FUNCTION().PROTECTED().SIGNATURE(void, BindSocketEvents);
    FUNCTION().PROTECTED().SIGNATURE(void, AddMessageWaiter, const Function<void(const String&)>&);
    FUNCTION().PROTECTED().SIGNATURE(void, ProcessReceivedData, const String&);
    FUNCTION().PROTECTED().SIGNATURE(void, FailMessageWaiters);
}
END_META;
// --- END META ---
