#include "o2/stdafx.h"
#include "TcpListener.h"

#include "o2/Network/NetworkSystem.h"
#include "o2/Network/Sockets/TcpSocket.h"
#include "o2/Utils/Debug/Log/LogStream.h"

namespace o2
{
    FORWARD_REF_IMPL(TcpSocket);

    TcpListener::TcpListener(RefCounter* refCounter):
        RefCounterable(refCounter)
    {}

    TcpListener::~TcpListener()
    {
        Close();
    }

    bool TcpListener::Listen(int port)
    {
        if (!NetworkSystem::IsSingletonInitialzed())
        {
            o2Debug.LogError("TcpListener::Listen: network system is not initialized");
            return false;
        }

        if (mListening)
        {
            o2Network.GetLog()->Error("TcpListener::Listen: already listening");
            return false;
        }

        SocketPlatform::Initialize();

        SocketAddress address;
        if (!SocketPlatform::ResolveSocketAddress(String(), port, address))
            return false;

        mHandle = SocketPlatform::CreateSocket(false, address);
        if (mHandle == InvalidSocketHandle)
            return false;

        if (!SocketPlatform::BindSocket(mHandle, address) || !SocketPlatform::ListenSocket(mHandle))
        {
            o2Network.GetLog()->Error("TcpListener::Listen: failed to bind port " + String(port));
            SocketPlatform::Close(mHandle);
            mHandle = InvalidSocketHandle;
            return false;
        }

        mLocalPort = SocketPlatform::GetLocalPort(mHandle);
        mListening = true;
        o2Network.RegisterTcpListener(Ref(this));

        return true;
    }

    Coroutine<Ref<TcpSocket>> TcpListener::AcceptAsync()
    {
        auto coroutine = [](Ref<TcpListener> self) -> Coroutine<Ref<TcpSocket>>
        {
            struct Result: public ThreadSafeRefCounterable { Ref<TcpSocket> socket; };

            Signal accepted;
            auto result = MakeShared<Result>();
            self->AddAcceptWaiter([accepted, result](const Ref<TcpSocket>& socket)
            {
                result->socket = socket;
                accepted.Synchronize();
            });

            co_await accepted;
            co_return result->socket;
        }(Ref(this));

        coroutine.Start(JobThread::Main);
        return coroutine;
    }

    void TcpListener::AddAcceptWaiter(const Function<void(const Ref<TcpSocket>&)>& waiter)
    {
        if (!mListening)
            waiter(nullptr);
        else
            mAcceptWaiters.Add(waiter);
    }

    int TcpListener::GetLocalPort() const
    {
        return mLocalPort;
    }

    bool TcpListener::IsListening() const
    {
        return mListening;
    }

    void TcpListener::Close()
    {
        if (!mListening)
            return;

        SocketPlatform::Close(mHandle);
        mHandle = InvalidSocketHandle;
        mListening = false;
        mLocalPort = 0;

        auto waiters = mAcceptWaiters;
        mAcceptWaiters.Clear();

        if (NetworkSystem::IsSingletonInitialzed())
            o2Network.UnregisterTcpListener(this);

        for (auto& waiter : waiters)
            waiter(nullptr);
    }

    void TcpListener::UpdateSocket(float dt)
    {
        while (mListening)
        {
            SocketAddress remoteAddress;
            SocketHandle accepted = SocketPlatform::AcceptSocket(mHandle, remoteAddress);
            if (accepted == InvalidSocketHandle)
                break;

            auto socket = mmake<TcpSocket>();
            socket->InitializeAccepted(accepted, remoteAddress);
            o2Network.RegisterTcpSocket(socket);

            if (!mAcceptWaiters.IsEmpty())
            {
                auto waiter = mAcceptWaiters[0];
                mAcceptWaiters.RemoveAt(0);
                waiter(socket);
            }
            else
                onAccepted(socket);
        }
    }
}
