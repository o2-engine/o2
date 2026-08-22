#include "o2/stdafx.h"
#include "TcpMessageChannel.h"

#include "o2/Network/NetworkSystem.h"
#include "o2/Utils/Debug/Log/LogStream.h"

namespace o2
{
    // Appends the 4-byte little-endian length prefix and the payload to the buffer
    void EncodeNetMessage(String& buffer, const String& message)
    {
        UInt32 size = (UInt32)message.size();
        char prefix[4] = { (char)(size & 0xff), (char)((size >> 8) & 0xff),
                           (char)((size >> 16) & 0xff), (char)((size >> 24) & 0xff) };
        buffer.append(prefix, 4);
        buffer.append(message.data(), message.size());
    }

    TcpMessageChannel::TcpMessageChannel(RefCounter* refCounter):
        RefCounterable(refCounter)
    {}

    TcpMessageChannel::~TcpMessageChannel()
    {
        Close();
    }

    bool TcpMessageChannel::Connect(const String& host, int port)
    {
        if (mSocket)
        {
            o2Debug.LogError("TcpMessageChannel::Connect: already connected or connecting");
            return false;
        }

        mSocket = mmake<TcpSocket>();
        BindSocketEvents();

        if (!mSocket->Connect(host, port))
        {
            mSocket = nullptr;
            return false;
        }

        return true;
    }

    Coroutine<bool> TcpMessageChannel::ConnectAsync(const String& host, int port)
    {
        auto coroutine = [](Ref<TcpMessageChannel> self, String host, int port) -> Coroutine<bool>
        {
            if (!self->Connect(host, port))
                co_return false;

            struct Result: public ThreadSafeRefCounterable { bool success = false; };

            Signal done;
            auto result = MakeShared<Result>();
            self->mSocket->AddConnectWaiter([done, result](bool success)
            {
                result->success = success;
                done.Synchronize();
            });

            co_await done;
            co_return result->success;
        }(Ref(this), host, port);

        coroutine.Start(JobThread::Main);
        return coroutine;
    }

    void TcpMessageChannel::Send(const String& message)
    {
        if (!mSocket)
        {
            o2Debug.LogError("TcpMessageChannel::Send: channel is not connected");
            return;
        }

        String framed;
        EncodeNetMessage(framed, message);
        mSocket->Send(framed);
    }

    Coroutine<String> TcpMessageChannel::ReceiveAsync()
    {
        auto coroutine = [](Ref<TcpMessageChannel> self) -> Coroutine<String>
        {
            struct Result: public ThreadSafeRefCounterable { String message; };

            Signal received;
            auto result = MakeShared<Result>();
            self->AddMessageWaiter([received, result](const String& message)
            {
                result->message = message;
                received.Synchronize();
            });

            co_await received;
            co_return result->message;
        }(Ref(this));

        coroutine.Start(JobThread::Main);
        return coroutine;
    }

    void TcpMessageChannel::Close()
    {
        if (!mSocket)
            return;

        auto socket = mSocket;
        mSocket = nullptr;
        mReceiveBuffer.Clear();

        socket->Close();

        FailMessageWaiters();
    }

    bool TcpMessageChannel::IsConnecting() const
    {
        return mSocket && mSocket->IsConnecting();
    }

    bool TcpMessageChannel::IsConnected() const
    {
        return mSocket && mSocket->IsConnected();
    }

    void TcpMessageChannel::InitializeAccepted(const Ref<TcpSocket>& socket)
    {
        mSocket = socket;
        BindSocketEvents();
    }

    void TcpMessageChannel::BindSocketEvents()
    {
        mSocket->onConnected = [this](bool success)
        {
            onConnected(success);
            if (!success)
                FailMessageWaiters();
        };

        mSocket->onDataReceived = [this](const String& data) { ProcessReceivedData(data); };

        mSocket->onClosed = [this]
        {
            FailMessageWaiters();
            onClosed();
        };
    }

    void TcpMessageChannel::AddMessageWaiter(const Function<void(const String&)>& waiter)
    {
        if (!mSocket || (!mSocket->IsConnected() && !mSocket->IsConnecting()))
            waiter(String());
        else
            mMessageWaiters.Add(waiter);
    }

    void TcpMessageChannel::ProcessReceivedData(const String& data)
    {
        mReceiveBuffer.append(data.data(), data.size());

        while (mReceiveBuffer.size() >= 4)
        {
            const unsigned char* prefix = (const unsigned char*)mReceiveBuffer.data();
            UInt32 messageSize = (UInt32)prefix[0] | ((UInt32)prefix[1] << 8) |
                ((UInt32)prefix[2] << 16) | ((UInt32)prefix[3] << 24);

            if ((int)messageSize > maxMessageSize)
            {
                o2Network.GetLog()->Error("TcpMessageChannel: incoming message size " + String((int)messageSize) +
                                          " exceeds the limit, closing");
                Close();
                onClosed();
                return;
            }

            if (mReceiveBuffer.size() < 4 + (size_t)messageSize)
                break;

            String message;
            message.assign(mReceiveBuffer.data() + 4, (size_t)messageSize);
            mReceiveBuffer.erase(0, 4 + (size_t)messageSize);

            if (!mMessageWaiters.IsEmpty())
            {
                auto waiter = mMessageWaiters[0];
                mMessageWaiters.RemoveAt(0);
                waiter(message);
            }
            else
                onMessage(message);
        }
    }

    void TcpMessageChannel::FailMessageWaiters()
    {
        auto waiters = mMessageWaiters;
        mMessageWaiters.Clear();
        for (auto& waiter : waiters)
            waiter(String());
    }
}
// --- META ---

DECLARE_CLASS(o2::TcpMessageChannel, o2__TcpMessageChannel);
// --- END META ---
