#include "o2/stdafx.h"
#include "TcpSocket.h"

#include "o2/Network/NetworkSystem.h"
#include "o2/Utils/Debug/Log/LogStream.h"
#include "o2/Utils/Jobs/JobSystem.h"

namespace o2
{
    // Shared holder for a coroutine awaited result
    template<typename _type>
    struct AsyncResult: public ThreadSafeRefCounterable
    {
        _type value = _type();
    };

    TcpSocket::TcpSocket(RefCounter* refCounter):
        RefCounterable(refCounter)
    {}

    TcpSocket::~TcpSocket()
    {
        if (mState != State::Idle && mState != State::Closed)
            CloseInternal(false);
    }

    bool TcpSocket::Connect(const String& host, int port)
    {
        if (!NetworkSystem::IsSingletonInitialzed())
        {
            o2Debug.LogError("TcpSocket::Connect: network system is not initialized");
            return false;
        }

        if (mState != State::Idle)
        {
            o2Network.GetLog()->Error("TcpSocket::Connect: socket is not idle");
            return false;
        }

        SocketPlatform::Initialize();

        mResolveState = MakeShared<ResolveState>();
        mResolveState->host = host;
        mResolveState->port = port;

        auto resolveState = mResolveState;
        o2Jobs.Schedule([resolveState]
        {
            resolveState->success = SocketPlatform::ResolveSocketAddress(resolveState->host, resolveState->port,
                                                                        resolveState->address);
            resolveState->done.Store(1);
        }, JobPriority::Normal, JobThread::Any);

        mState = State::Resolving;
        mConnectingTime = 0.0f;
        o2Network.RegisterTcpSocket(Ref(this));

        return true;
    }

    Coroutine<bool> TcpSocket::ConnectAsync(const String& host, int port)
    {
        // The waiter is registered before the coroutine ever runs, so a result arriving while the
        // coroutine is still starting cannot be missed
        Signal done;
        auto result = MakeShared<AsyncResult<bool>>();
        auto waiter = [done, result](bool success) { result->value = success; done.Synchronize(); };

        if (!Connect(host, port))
            waiter(false);
        else
            AddConnectWaiter(waiter);

        auto coroutine = [](Signal done, SharedRef<AsyncResult<bool>> result) -> Coroutine<bool>
        {
            co_await done;
            co_return result->value;
        }(done, result);

        coroutine.Start(JobThread::Main);
        return coroutine;
    }

    void TcpSocket::Send(const String& data)
    {
        Send(data.data(), (int)data.size());
    }

    void TcpSocket::Send(const void* data, int size)
    {
        if (size <= 0)
            return;

        if (mState == State::Idle || mState == State::Closed)
        {
            o2Network.GetLog()->Error("TcpSocket::Send: socket is not connected");
            return;
        }

        mSendBuffer.append((const char*)data, (size_t)size);

        if (mState == State::Connected && !FlushSendBuffer())
            CloseInternal(true);
    }

    Coroutine<String> TcpSocket::ReceiveAsync()
    {
        Signal received;
        auto result = MakeShared<AsyncResult<String>>();
        AddReceiveWaiter([received, result](const String& data) { result->value = data; received.Synchronize(); });

        auto coroutine = [](Signal received, SharedRef<AsyncResult<String>> result) -> Coroutine<String>
        {
            co_await received;
            co_return result->value;
        }(received, result);

        coroutine.Start(JobThread::Main);
        return coroutine;
    }

    void TcpSocket::Close()
    {
        if (mState != State::Idle && mState != State::Closed)
            CloseInternal(false);
    }

    bool TcpSocket::IsConnecting() const
    {
        return mState == State::Resolving || mState == State::Connecting;
    }

    bool TcpSocket::IsConnected() const
    {
        return mState == State::Connected;
    }

    String TcpSocket::GetRemoteAddress() const
    {
        return mRemoteAddress.GetAddressString();
    }

    int TcpSocket::GetRemotePort() const
    {
        return mRemoteAddress.GetPort();
    }

    void TcpSocket::InitializeAccepted(SocketHandle handle, const SocketAddress& remoteAddress)
    {
        mHandle = handle;
        mRemoteAddress = remoteAddress;
        mState = State::Connected;
    }

    void TcpSocket::AddConnectWaiter(const Function<void(bool)>& waiter)
    {
        if (mState == State::Connected)
            waiter(true);
        else if (mState == State::Idle || mState == State::Closed)
            waiter(false);
        else
            mConnectWaiters.Add(waiter);
    }

    void TcpSocket::AddReceiveWaiter(const Function<void(const String&)>& waiter)
    {
        if (!mPendingReceivedData.IsEmpty())
        {
            String data = mPendingReceivedData;
            mPendingReceivedData.Clear();
            waiter(data);
        }
        else if (mState == State::Idle || mState == State::Closed)
            waiter(String());
        else
            mReceiveWaiters.Add(waiter);
    }

    void TcpSocket::UpdateSocket(float dt)
    {
        if (mState == State::Resolving)
        {
            if (mResolveState->done.Load() != 0)
            {
                if (!mResolveState->success)
                {
                    o2Network.GetLog()->Error("TcpSocket: failed to resolve host '" + mResolveState->host + "'");
                    FinishConnect(false);
                    return;
                }

                mRemoteAddress = mResolveState->address;
                mResolveState = nullptr;

                mHandle = SocketPlatform::CreateSocket(false, mRemoteAddress);
                if (mHandle == InvalidSocketHandle || !SocketPlatform::ConnectSocket(mHandle, mRemoteAddress))
                {
                    FinishConnect(false);
                    return;
                }

                mState = State::Connecting;
            }
            else
            {
                mConnectingTime += dt;
                if (mConnectingTime > connectTimeout)
                    FinishConnect(false);
            }

            return;
        }

        if (mState == State::Connecting)
        {
            int finished = SocketPlatform::CheckConnectFinished(mHandle);
            if (finished > 0)
                FinishConnect(true);
            else if (finished < 0)
                FinishConnect(false);
            else
            {
                mConnectingTime += dt;
                if (mConnectingTime > connectTimeout)
                    FinishConnect(false);
            }

            return;
        }

        if (mState == State::Connected)
        {
            if (!FlushSendBuffer())
            {
                CloseInternal(true);
                return;
            }

            char buffer[65536];
            while (mState == State::Connected)
            {
                int received = SocketPlatform::ReceiveData(mHandle, buffer, sizeof(buffer));
                if (received == 0)
                    break;

                if (received < 0)
                {
                    CloseInternal(true);
                    return;
                }

                String data;
                data.assign(buffer, (size_t)received);

                if (!mReceiveWaiters.IsEmpty())
                {
                    auto waiter = mReceiveWaiters[0];
                    mReceiveWaiters.RemoveAt(0);
                    waiter(data);
                }
                else if (!onDataReceived.IsEmpty())
                    onDataReceived(data);
                else
                    mPendingReceivedData.append(data.data(), data.size());
            }
        }
    }

    void TcpSocket::FinishConnect(bool success)
    {
        if (!success)
        {
            if (mHandle != InvalidSocketHandle)
            {
                SocketPlatform::Close(mHandle);
                mHandle = InvalidSocketHandle;
            }

            mResolveState = nullptr;
            mState = State::Closed;

            if (NetworkSystem::IsSingletonInitialzed())
                o2Network.UnregisterTcpSocket(this);
        }
        else
            mState = State::Connected;

        auto waiters = mConnectWaiters;
        mConnectWaiters.Clear();

        onConnected(success);
        for (auto& waiter : waiters)
            waiter(success);

        if (!success)
        {
            auto receiveWaiters = mReceiveWaiters;
            mReceiveWaiters.Clear();
            for (auto& waiter : receiveWaiters)
                waiter(String());
        }

        if (success && mState == State::Connected && !mSendBuffer.IsEmpty())
        {
            if (!FlushSendBuffer())
                CloseInternal(true);
        }
    }

    void TcpSocket::CloseInternal(bool remote)
    {
        if (mHandle != InvalidSocketHandle)
        {
            SocketPlatform::Close(mHandle);
            mHandle = InvalidSocketHandle;
        }

        mResolveState = nullptr;
        mState = State::Closed;
        mSendBuffer.clear();


        auto connectWaiters = mConnectWaiters;
        mConnectWaiters.Clear();

        auto receiveWaiters = mReceiveWaiters;
        mReceiveWaiters.Clear();

        if (NetworkSystem::IsSingletonInitialzed())
            o2Network.UnregisterTcpSocket(this);

        for (auto& waiter : connectWaiters)
            waiter(false);

        for (auto& waiter : receiveWaiters)
            waiter(String());

        if (remote)
            onClosed();
    }

    bool TcpSocket::FlushSendBuffer()
    {
        while (!mSendBuffer.IsEmpty())
        {
            int sent = SocketPlatform::SendData(mHandle, mSendBuffer.data(), (int)mSendBuffer.size());
            if (sent < 0)
                return false;

            if (sent == 0)
                break;

            mSendBuffer.erase(0, (size_t)sent);
        }

        return true;
    }
}
