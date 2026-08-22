#pragma once

#include "o2/Network/Sockets/SocketPlatform.h"
#include "o2/Utils/Coroutines/Coroutines.h"
#include "o2/Utils/Function/Function.h"
#include "o2/Utils/Types/Ref.h"
#include "o2/Utils/Types/String.h"

namespace o2
{
    FORWARD_CLASS_REF(TcpSocket);

    // -------------------------------------------------------------------------------------------
    // TCP listening socket. Accepts incoming connections as TcpSocket instances, pumped by the
    // network system every frame on the main thread
    // -------------------------------------------------------------------------------------------
    class TcpListener: public RefCounterable
    {
    public:
        Function<void(const Ref<TcpSocket>&)> onAccepted; // Called for every accepted connection

    public:
        // Default constructor
        explicit TcpListener(RefCounter* refCounter);

        // Destructor, closes the listener
        ~TcpListener() override;

        // Starts listening on the port. Port 0 binds an ephemeral port, see GetLocalPort.
        // Returns false on failure
        bool Listen(int port);

        // Coroutine accept: completes with the next accepted connection, or null when the
        // listener is closed. Pending awaiters consume connections before onAccepted
        Coroutine<Ref<TcpSocket>> AcceptAsync();

        // Returns the port the listener is bound to, 0 when not listening
        int GetLocalPort() const;

        // Returns true while listening
        bool IsListening() const;

        // Closes the listener. Pending awaiters complete with null
        void Close();

    protected:
        SocketHandle mHandle = InvalidSocketHandle; // Native socket handle
        bool         mListening = false;            // True while listening
        int          mLocalPort = 0;                // Bound port

        Vector<Function<void(const Ref<TcpSocket>&)>> mAcceptWaiters; // One-shot accept completions

    protected:
        // Adds a one-shot accept completion; fires immediately with null when closed
        void AddAcceptWaiter(const Function<void(const Ref<TcpSocket>&)>& waiter);

        // Pumps pending accepts. Called by the network system
        void UpdateSocket(float dt);

        friend class NetworkSystem;
    };
}
