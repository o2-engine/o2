#include "o2/stdafx.h"
#include "UdpSocket.h"

#include "o2/Network/NetworkSystem.h"
#include "o2/Utils/Debug/Log/LogStream.h"

namespace o2
{
    UdpSocket::UdpSocket(RefCounter* refCounter):
        RefCounterable(refCounter)
    {}

    UdpSocket::~UdpSocket()
    {
        Close();
    }

    bool UdpSocket::Open(int localPort /*= 0*/)
    {
        if (!NetworkSystem::IsSingletonInitialzed())
        {
            o2Debug.LogError("UdpSocket::Open: network system is not initialized");
            return false;
        }

        if (mOpened)
        {
            o2Network.GetLog()->Error("UdpSocket::Open: already opened");
            return false;
        }

        SocketPlatform::Initialize();

        SocketAddress address;
        if (!SocketPlatform::ResolveSocketAddress(String(), localPort, address))
            return false;

        mHandle = SocketPlatform::CreateSocket(true, address);
        if (mHandle == InvalidSocketHandle)
            return false;

        if (!SocketPlatform::BindSocket(mHandle, address))
        {
            o2Network.GetLog()->Error("UdpSocket::Open: failed to bind port " + String(localPort));
            SocketPlatform::Close(mHandle);
            mHandle = InvalidSocketHandle;
            return false;
        }

        mLocalPort = SocketPlatform::GetLocalPort(mHandle);
        mOpened = true;
        o2Network.RegisterUdpSocket(Ref(this));

        return true;
    }

    bool UdpSocket::Connect(const String& address, int port)
    {
        if (!mOpened && !Open())
            return false;

        SocketAddress remoteAddress;
        if (!ResolveCached(address, port, remoteAddress))
            return false;

        if (!SocketPlatform::ConnectDatagramSocket(mHandle, remoteAddress))
            return false;

        mHasDefaultRemote = true;
        return true;
    }

    bool UdpSocket::Send(const String& data)
    {
        return Send(data.data(), (int)data.size());
    }

    bool UdpSocket::Send(const void* data, int size)
    {
        if (!mOpened || !mHasDefaultRemote)
        {
            o2Network.GetLog()->Error("UdpSocket::Send: no default remote endpoint, use Connect or SendTo");
            return false;
        }

        return SocketPlatform::SendData(mHandle, data, size) == size;
    }

    bool UdpSocket::SendTo(const String& address, int port, const String& data)
    {
        return SendTo(address, port, data.data(), (int)data.size());
    }

    bool UdpSocket::SendTo(const String& address, int port, const void* data, int size)
    {
        if (!mOpened && !Open())
            return false;

        SocketAddress remoteAddress;
        if (!ResolveCached(address, port, remoteAddress))
            return false;

        return SocketPlatform::SendDatagram(mHandle, remoteAddress, data, size);
    }

    Coroutine<UdpDatagram> UdpSocket::ReceiveAsync()
    {
        auto coroutine = [](Ref<UdpSocket> self) -> Coroutine<UdpDatagram>
        {
            struct Result: public ThreadSafeRefCounterable { UdpDatagram datagram; };

            Signal received;
            auto result = MakeShared<Result>();
            self->AddReceiveWaiter([received, result](const UdpDatagram& datagram)
            {
                result->datagram = datagram;
                received.Synchronize();
            });

            co_await received;
            co_return result->datagram;
        }(Ref(this));

        coroutine.Start(JobThread::Main);
        return coroutine;
    }

    int UdpSocket::GetLocalPort() const
    {
        return mLocalPort;
    }

    bool UdpSocket::IsOpened() const
    {
        return mOpened;
    }

    void UdpSocket::Close()
    {
        if (!mOpened)
            return;

        SocketPlatform::Close(mHandle);
        mHandle = InvalidSocketHandle;
        mOpened = false;
        mLocalPort = 0;
        mHasDefaultRemote = false;
        mResolvedAddresses.Clear();

        auto waiters = mReceiveWaiters;
        mReceiveWaiters.Clear();

        if (NetworkSystem::IsSingletonInitialzed())
            o2Network.UnregisterUdpSocket(this);

        for (auto& waiter : waiters)
            waiter(UdpDatagram());
    }

    bool UdpSocket::ResolveCached(const String& address, int port, SocketAddress& outAddress)
    {
        String key = address + ":" + String(port);
        if (mResolvedAddresses.TryGetValue(key, outAddress))
            return true;

        if (!SocketPlatform::ResolveSocketAddress(address, port, outAddress))
        {
            o2Network.GetLog()->Error("UdpSocket: failed to resolve host '" + address + "'");
            return false;
        }

        mResolvedAddresses.Add(key, outAddress);
        return true;
    }

    void UdpSocket::AddReceiveWaiter(const Function<void(const UdpDatagram&)>& waiter)
    {
        if (!mOpened)
            waiter(UdpDatagram());
        else
            mReceiveWaiters.Add(waiter);
    }

    void UdpSocket::UpdateSocket(float dt)
    {
        char buffer[65536];
        while (mOpened)
        {
            SocketAddress fromAddress;
            int received = SocketPlatform::ReceiveDatagram(mHandle, buffer, sizeof(buffer), fromAddress);
            if (received == 0)
                break;

            if (received < 0)
                break; // A failed datagram (e.g. ICMP unreachable) must not close the socket

            UdpDatagram datagram;
            datagram.data.assign(buffer, (size_t)received);
            datagram.address = fromAddress.GetAddressString();
            datagram.port = fromAddress.GetPort();

            if (!mReceiveWaiters.IsEmpty())
            {
                auto waiter = mReceiveWaiters[0];
                mReceiveWaiters.RemoveAt(0);
                waiter(datagram);
            }
            else
                onDataReceived(datagram.data, datagram.address, datagram.port);
        }
    }
}
// --- META ---

DECLARE_CLASS(o2::UdpSocket, o2__UdpSocket);
// --- END META ---
