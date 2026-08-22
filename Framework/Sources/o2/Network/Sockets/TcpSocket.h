#pragma once

#include "o2/Network/Sockets/SocketPlatform.h"
#include "o2/Utils/Coroutines/Coroutines.h"
#include "o2/Utils/Function/Function.h"
#include "o2/Utils/Types/Ref.h"
#include "o2/Utils/Types/String.h"

namespace o2
{
    // -------------------------------------------------------------------------------------------
    // Asynchronous TCP connection. Non-blocking, pumped by the network system every frame on the
    // main thread; all callbacks are invoked on the main thread. Payloads are o2::String, which is
    // binary-safe. Connect and receive are available both as events and as awaitable coroutines
    // -------------------------------------------------------------------------------------------
    class TcpSocket: public RefCounterable
    {
    public:
        Function<void(bool success)>       onConnected;    // Called once when the connect attempt finishes
        Function<void(const String& data)> onDataReceived; // Called with every received data chunk
        Function<void()>                   onClosed;       // Called when the connection is closed remotely or by error

        float connectTimeout = 30.0f; // Connect attempt timeout in seconds

    public:
        // Default constructor
        explicit TcpSocket(RefCounter* refCounter);

        // Destructor, closes the socket
        ~TcpSocket() override;

        // Begins an asynchronous connect to the host and port. The result is reported through
        // onConnected and pending ConnectAsync awaiters. Returns false on immediate failure
        bool Connect(const String& host, int port);

        // Coroutine connect: starts the connect and completes with its result
        Coroutine<bool> ConnectAsync(const String& host, int port);

        // Sends data. Buffered while connecting; not allowed before Connect or after close
        void Send(const String& data);

        // Sends raw bytes, same buffering rules
        void Send(const void* data, int size);

        // Coroutine receive: completes with the next received data chunk, or an empty string
        // when the connection is closed. Pending awaiters consume data before onDataReceived
        Coroutine<String> ReceiveAsync();

        // Closes the socket. Pending awaiters complete with failure/empty results; onClosed is
        // not called for a local close
        void Close();

        // Returns true while the connect attempt is in progress
        bool IsConnecting() const;

        // Returns true when connected
        bool IsConnected() const;

        // Returns the remote address, empty when not connected
        String GetRemoteAddress() const;

        // Returns the remote port, 0 when not connected
        int GetRemotePort() const;

    protected:
        // Connection state
        enum class State { Idle, Resolving, Connecting, Connected, Closed };

        // Host resolve result, filled on a worker job
        struct ResolveState: public ThreadSafeRefCounterable
        {
            String        host;    // Host to resolve
            int           port;    // Port to resolve
            SocketAddress address; // Resolved address
            Atomic<int>   done{ 0 };   // 1 once the job finished
            bool          success = false; // Resolve result
        };

        State        mState = State::Idle;           // Connection state
        SocketHandle mHandle = InvalidSocketHandle;  // Native socket handle

        SharedRef<ResolveState> mResolveState; // Pending host resolve, valid in Resolving state
        SocketAddress           mRemoteAddress; // Resolved remote address

        float mConnectingTime = 0.0f; // Time spent in the connect attempt, for the timeout

        String mSendBuffer; // Bytes not yet accepted by the socket

        Vector<Function<void(bool)>>          mConnectWaiters; // One-shot connect completions
        Vector<Function<void(const String&)>> mReceiveWaiters; // One-shot receive completions

    protected:
        // Initializes an already connected socket accepted by a listener
        void InitializeAccepted(SocketHandle handle, const SocketAddress& remoteAddress);

        // Adds a one-shot connect completion; fires immediately when already resolved
        void AddConnectWaiter(const Function<void(bool)>& waiter);

        // Adds a one-shot receive completion; fires immediately with empty data when closed
        void AddReceiveWaiter(const Function<void(const String&)>& waiter);

        // Pumps the socket state: resolve, connect, send and receive. Called by the network system
        void UpdateSocket(float dt);

        // Finishes the connect attempt with the result, firing event and waiters
        void FinishConnect(bool success);

        // Closes the handle and completes all pending waiters. Fires onClosed when remote is true
        void CloseInternal(bool remote);

        // Sends as much of the send buffer as the socket accepts. Returns false on error
        bool FlushSendBuffer();

        friend class NetworkSystem;
        friend class TcpListener;
        friend class TcpMessageChannel;
    };
}
