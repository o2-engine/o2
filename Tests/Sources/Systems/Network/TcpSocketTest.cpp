#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "Network/NetworkTestHelpers.h"
#include "o2/Network/Sockets/TcpListener.h"
#include "o2/Network/Sockets/TcpSocket.h"

using namespace o2;

TEST(TcpSocket, ListenAcceptEchoWithCallbacks)
{
    auto listener = mmake<TcpListener>();
    ASSERT_TRUE(listener->Listen(0));
    ASSERT_GT(listener->GetLocalPort(), 0);

    Ref<TcpSocket> serverSide;
    listener->onAccepted = [&](const Ref<TcpSocket>& socket)
    {
        serverSide = socket;
        socket->onDataReceived = [socket](const String& data) { socket->Send("echo:" + data); };
    };

    auto client = mmake<TcpSocket>();
    bool connected = false;
    String received;
    client->onConnected = [&](bool success) { connected = success; };
    client->onDataReceived = [&](const String& data) { received += data; };

    ASSERT_TRUE(client->Connect("127.0.0.1", listener->GetLocalPort()));
    ASSERT_TRUE(NetPumpUntil([&] { return connected; }));
    EXPECT_TRUE(client->IsConnected());

    client->Send("hello");
    ASSERT_TRUE(NetPumpUntil([&] { return received == "echo:hello"; }));

    EXPECT_TRUE(serverSide->IsConnected());
    EXPECT_EQ(serverSide->GetRemoteAddress(), String("127.0.0.1"));
    EXPECT_EQ(client->GetRemotePort(), listener->GetLocalPort());

    client->Close();
    listener->Close();
}

TEST(TcpSocket, ConnectAndReceiveWithCoroutines)
{
    auto listener = mmake<TcpListener>();
    ASSERT_TRUE(listener->Listen(0));

    listener->onAccepted = [](const Ref<TcpSocket>& socket)
    {
        socket->onDataReceived = [socket](const String& data) { socket->Send("echo:" + data); };
    };

    auto client = mmake<TcpSocket>();

    auto coroutine = [](Ref<TcpSocket> client, int port) -> Coroutine<String>
    {
        if (!co_await client->ConnectAsync("127.0.0.1", port))
            co_return String("connect failed");

        client->Send("ping");
        String response = co_await client->ReceiveAsync();
        co_return response;
    }(client, listener->GetLocalPort());

    coroutine.Start(JobThread::Main);

    ASSERT_TRUE(NetPumpUntil([&] { return coroutine.IsDone(); }));
    EXPECT_EQ(coroutine.GetResult(), String("echo:ping"));

    client->Close();
    listener->Close();
}

TEST(TcpSocket, AcceptAsyncCoroutine)
{
    auto listener = mmake<TcpListener>();
    ASSERT_TRUE(listener->Listen(0));

    auto coroutine = [](Ref<TcpListener> listener) -> Coroutine<String>
    {
        Ref<TcpSocket> accepted = co_await listener->AcceptAsync();
        if (!accepted)
            co_return String("accept failed");

        String data = co_await accepted->ReceiveAsync();
        co_return data;
    }(listener);

    coroutine.Start(JobThread::Main);

    auto client = mmake<TcpSocket>();
    ASSERT_TRUE(client->Connect("127.0.0.1", listener->GetLocalPort()));
    client->Send("from client");

    ASSERT_TRUE(NetPumpUntil([&] { return coroutine.IsDone(); }));
    EXPECT_EQ(coroutine.GetResult(), String("from client"));

    client->Close();
    listener->Close();
}

TEST(TcpSocket, ConnectToClosedPortFails)
{
    // Bind an ephemeral port and close it right away, so the connect target is a closed port
    int closedPort = 0;
    {
        auto listener = mmake<TcpListener>();
        ASSERT_TRUE(listener->Listen(0));
        closedPort = listener->GetLocalPort();
        listener->Close();
    }

    auto client = mmake<TcpSocket>();
    bool finished = false;
    bool connected = true;
    client->onConnected = [&](bool success) { connected = success; finished = true; };

    ASSERT_TRUE(client->Connect("127.0.0.1", closedPort));
    ASSERT_TRUE(NetPumpUntil([&] { return finished; }));
    EXPECT_FALSE(connected);
    EXPECT_FALSE(client->IsConnected());
}

TEST(TcpSocket, ConnectAsyncToClosedPortReturnsFalse)
{
    int closedPort = 0;
    {
        auto listener = mmake<TcpListener>();
        ASSERT_TRUE(listener->Listen(0));
        closedPort = listener->GetLocalPort();
        listener->Close();
    }

    auto client = mmake<TcpSocket>();
    auto coroutine = client->ConnectAsync("127.0.0.1", closedPort);

    ASSERT_TRUE(NetPumpUntil([&] { return coroutine.IsDone(); }));
    EXPECT_FALSE(coroutine.GetResult());
}

TEST(TcpSocket, ResolveFailureFailsConnect)
{
    auto client = mmake<TcpSocket>();
    bool finished = false;
    bool connected = true;
    client->onConnected = [&](bool success) { connected = success; finished = true; };

    ASSERT_TRUE(client->Connect("no-such-host.invalid", 80));
    ASSERT_TRUE(NetPumpUntil([&] { return finished; }, 15.0f));
    EXPECT_FALSE(connected);
}

TEST(TcpSocket, BigTransfer)
{
    auto listener = mmake<TcpListener>();
    ASSERT_TRUE(listener->Listen(0));

    String receivedByServer;
    Ref<TcpSocket> serverSide;
    listener->onAccepted = [&](const Ref<TcpSocket>& socket)
    {
        serverSide = socket;
        socket->onDataReceived = [&](const String& data) { receivedByServer += data; };
    };

    String bigData;
    bigData.resize(1024 * 1024);
    for (size_t i = 0; i < bigData.size(); i++)
        bigData[i] = (char)(i * 31 + i / 1024);

    auto client = mmake<TcpSocket>();
    ASSERT_TRUE(client->Connect("127.0.0.1", listener->GetLocalPort()));
    client->Send(bigData);

    ASSERT_TRUE(NetPumpUntil([&] { return receivedByServer.size() == bigData.size(); }, 10.0f));
    EXPECT_TRUE(receivedByServer == bigData);

    client->Close();
    listener->Close();
}

TEST(TcpSocket, RemoteCloseFiresOnClosed)
{
    auto listener = mmake<TcpListener>();
    ASSERT_TRUE(listener->Listen(0));

    Ref<TcpSocket> serverSide;
    listener->onAccepted = [&](const Ref<TcpSocket>& socket) { serverSide = socket; };

    auto client = mmake<TcpSocket>();
    bool connected = false;
    bool closed = false;
    client->onConnected = [&](bool success) { connected = success; };
    client->onClosed = [&] { closed = true; };

    ASSERT_TRUE(client->Connect("127.0.0.1", listener->GetLocalPort()));
    ASSERT_TRUE(NetPumpUntil([&] { return connected && serverSide; }));

    serverSide->Close();
    ASSERT_TRUE(NetPumpUntil([&] { return closed; }));
    EXPECT_FALSE(client->IsConnected());

    listener->Close();
}

TEST(TcpSocket, CloseCompletesPendingReceiveWithEmptyResult)
{
    auto listener = mmake<TcpListener>();
    ASSERT_TRUE(listener->Listen(0));

    auto client = mmake<TcpSocket>();
    bool connected = false;
    client->onConnected = [&](bool success) { connected = success; };
    ASSERT_TRUE(client->Connect("127.0.0.1", listener->GetLocalPort()));
    ASSERT_TRUE(NetPumpUntil([&] { return connected; }));

    auto coroutine = client->ReceiveAsync();
    client->Close();

    ASSERT_TRUE(NetPumpUntil([&] { return coroutine.IsDone(); }));
    EXPECT_TRUE(coroutine.GetResult().IsEmpty());

    listener->Close();
}

// Regression: the CI race where the connection and its data arrived before any awaiter existed.
// Both must be buffered and served to awaiters created later
TEST(TcpSocket, AcceptAsyncAfterConnectionAlreadyAccepted)
{
    auto listener = mmake<TcpListener>();
    ASSERT_TRUE(listener->Listen(0)); // No onAccepted subscriber and no awaiter yet

    auto client = mmake<TcpSocket>();
    ASSERT_TRUE(client->Connect("127.0.0.1", listener->GetLocalPort()));
    client->Send("early data");

    // Let the listener accept the connection and receive the data with nobody consuming them
    ASSERT_TRUE(NetPumpUntil([&] { return client->IsConnected(); }));
    NetPumpFrames(10);

    auto coroutine = [](Ref<TcpListener> listener) -> Coroutine<String>
    {
        Ref<TcpSocket> accepted = co_await listener->AcceptAsync();
        if (!accepted)
            co_return String("accept failed");

        String data = co_await accepted->ReceiveAsync();
        co_return data;
    }(listener);

    coroutine.Start(JobThread::Main);

    ASSERT_TRUE(NetPumpUntil([&] { return coroutine.IsDone(); }));
    EXPECT_EQ(coroutine.GetResult(), String("early data"));

    client->Close();
    listener->Close();
}
