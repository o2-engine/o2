#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "Network/NetworkTestHelpers.h"
#include "o2/Network/Sockets/UdpSocket.h"

using namespace o2;

TEST(UdpSocket, SendToAndReceiveWithCallback)
{
    auto receiver = mmake<UdpSocket>();
    ASSERT_TRUE(receiver->Open());
    ASSERT_GT(receiver->GetLocalPort(), 0);

    String receivedData;
    String receivedAddress;
    int receivedPort = 0;
    receiver->onDataReceived = [&](const String& data, const String& address, int port)
    {
        receivedData = data;
        receivedAddress = address;
        receivedPort = port;
    };

    auto sender = mmake<UdpSocket>();
    ASSERT_TRUE(sender->Open());
    ASSERT_TRUE(sender->SendTo("127.0.0.1", receiver->GetLocalPort(), "datagram payload"));

    ASSERT_TRUE(NetPumpUntil([&] { return !receivedData.IsEmpty(); }));
    EXPECT_EQ(receivedData, String("datagram payload"));
    EXPECT_EQ(receivedAddress, String("127.0.0.1"));
    EXPECT_EQ(receivedPort, sender->GetLocalPort());
}

TEST(UdpSocket, ConnectSetsDefaultRemote)
{
    auto receiver = mmake<UdpSocket>();
    ASSERT_TRUE(receiver->Open());

    String receivedData;
    receiver->onDataReceived = [&](const String& data, const String&, int) { receivedData = data; };

    auto sender = mmake<UdpSocket>();
    ASSERT_TRUE(sender->Connect("127.0.0.1", receiver->GetLocalPort()));
    ASSERT_TRUE(sender->Send("connected send"));

    ASSERT_TRUE(NetPumpUntil([&] { return !receivedData.IsEmpty(); }));
    EXPECT_EQ(receivedData, String("connected send"));
}

TEST(UdpSocket, SendWithoutRemoteFails)
{
    auto socket = mmake<UdpSocket>();
    ASSERT_TRUE(socket->Open());
    EXPECT_FALSE(socket->Send("no remote"));
}

TEST(UdpSocket, ReceiveAsyncCoroutine)
{
    auto receiver = mmake<UdpSocket>();
    ASSERT_TRUE(receiver->Open());

    auto coroutine = [](Ref<UdpSocket> receiver) -> Coroutine<UdpDatagram>
    {
        UdpDatagram datagram = co_await receiver->ReceiveAsync();
        co_return datagram;
    }(receiver);

    coroutine.Start(JobThread::Main);

    auto sender = mmake<UdpSocket>();
    ASSERT_TRUE(sender->Open());
    ASSERT_TRUE(sender->SendTo("127.0.0.1", receiver->GetLocalPort(), "async datagram"));

    ASSERT_TRUE(NetPumpUntil([&] { return coroutine.IsDone(); }));

    UdpDatagram datagram = coroutine.GetResult();
    EXPECT_EQ(datagram.data, String("async datagram"));
    EXPECT_EQ(datagram.address, String("127.0.0.1"));
    EXPECT_EQ(datagram.port, sender->GetLocalPort());
}

TEST(UdpSocket, CloseCompletesPendingReceiveWithEmptyDatagram)
{
    auto receiver = mmake<UdpSocket>();
    ASSERT_TRUE(receiver->Open());

    auto coroutine = receiver->ReceiveAsync();
    receiver->Close();

    ASSERT_TRUE(NetPumpUntil([&] { return coroutine.IsDone(); }));
    EXPECT_TRUE(coroutine.GetResult().data.IsEmpty());
    EXPECT_FALSE(receiver->IsOpened());
}

TEST(UdpSocket, TwoWayExchange)
{
    auto first = mmake<UdpSocket>();
    auto second = mmake<UdpSocket>();
    ASSERT_TRUE(first->Open());
    ASSERT_TRUE(second->Open());

    String firstReceived;
    String secondReceived;
    first->onDataReceived = [&](const String& data, const String& address, int port)
    {
        firstReceived = data;
        first->SendTo(address, port, "pong");
    };
    second->onDataReceived = [&](const String& data, const String&, int) { secondReceived = data; };

    ASSERT_TRUE(second->SendTo("127.0.0.1", first->GetLocalPort(), "ping"));

    ASSERT_TRUE(NetPumpUntil([&] { return !secondReceived.IsEmpty(); }));
    EXPECT_EQ(firstReceived, String("ping"));
    EXPECT_EQ(secondReceived, String("pong"));
}

// Regression: a datagram arriving before any awaiter exists must be buffered, not dropped
TEST(UdpSocket, ReceiveAsyncAfterDatagramAlreadyArrived)
{
    auto receiver = mmake<UdpSocket>();
    ASSERT_TRUE(receiver->Open()); // No onDataReceived subscriber and no awaiter yet

    auto sender = mmake<UdpSocket>();
    ASSERT_TRUE(sender->Open());
    ASSERT_TRUE(sender->SendTo("127.0.0.1", receiver->GetLocalPort(), "early datagram"));

    NetPumpFrames(10);

    auto coroutine = receiver->ReceiveAsync();

    ASSERT_TRUE(NetPumpUntil([&] { return coroutine.IsDone(); }));
    EXPECT_EQ(coroutine.GetResult().data, String("early datagram"));
}
