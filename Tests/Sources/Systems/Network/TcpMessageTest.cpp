#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "Network/NetworkTestHelpers.h"
#include "o2/Network/Sockets/TcpMessageChannel.h"
#include "o2/Network/Sockets/TcpMessageServer.h"

using namespace o2;

TEST(TcpMessage, ChatScenarioWithCallbacks)
{
    auto server = mmake<TcpMessageServer>();
    ASSERT_TRUE(server->Listen(0));

    Vector<String> serverReceived;
    server->onClientMessage = [&](int clientId, const String& message)
    {
        serverReceived.Add(message);
        server->SendTo(clientId, "server got: " + message);
    };

    auto client = mmake<TcpMessageChannel>();
    bool connected = false;
    Vector<String> clientReceived;
    client->onConnected = [&](bool success) { connected = success; };
    client->onMessage = [&](const String& message) { clientReceived.Add(message); };

    ASSERT_TRUE(client->Connect("127.0.0.1", server->GetLocalPort()));
    ASSERT_TRUE(NetPumpUntil([&] { return connected; }));

    client->Send("hello");
    client->Send("world");

    ASSERT_TRUE(NetPumpUntil([&] { return clientReceived.Count() == 2; }));
    EXPECT_EQ(serverReceived, Vector<String>({ "hello", "world" }));
    EXPECT_EQ(clientReceived, Vector<String>({ "server got: hello", "server got: world" }));

    client->Close();
    server->Close();
}

TEST(TcpMessage, MessagesKeepBoundariesWhenCoalesced)
{
    auto server = mmake<TcpMessageServer>();
    ASSERT_TRUE(server->Listen(0));

    Vector<String> received;
    server->onClientMessage = [&](int, const String& message) { received.Add(message); };

    auto client = mmake<TcpMessageChannel>();
    ASSERT_TRUE(client->Connect("127.0.0.1", server->GetLocalPort()));

    // All sent before the connect finishes: they are buffered and flushed as one stream chunk
    for (int i = 0; i < 100; i++)
        client->Send("message #" + String(i));

    ASSERT_TRUE(NetPumpUntil([&] { return received.Count() == 100; }));
    for (int i = 0; i < 100; i++)
        EXPECT_EQ(received[i], "message #" + String(i));

    client->Close();
    server->Close();
}

TEST(TcpMessage, BigMessageCrossesChunks)
{
    auto server = mmake<TcpMessageServer>();
    ASSERT_TRUE(server->Listen(0));

    String received;
    server->onClientMessage = [&](int, const String& message) { received = message; };

    String bigMessage;
    bigMessage.resize(2 * 1024 * 1024);
    for (size_t i = 0; i < bigMessage.size(); i++)
        bigMessage[i] = (char)(i * 17 + 5);

    auto client = mmake<TcpMessageChannel>();
    ASSERT_TRUE(client->Connect("127.0.0.1", server->GetLocalPort()));
    client->Send(bigMessage);

    ASSERT_TRUE(NetPumpUntil([&] { return !received.IsEmpty(); }, 10.0f));
    EXPECT_TRUE(received == bigMessage);

    client->Close();
    server->Close();
}

TEST(TcpMessage, BinaryPayloadWithZeroBytes)
{
    auto server = mmake<TcpMessageServer>();
    ASSERT_TRUE(server->Listen(0));

    String received;
    server->onClientMessage = [&](int, const String& message) { received = message; };

    String binary;
    binary.assign("a\0b\0c", 5);

    auto client = mmake<TcpMessageChannel>();
    ASSERT_TRUE(client->Connect("127.0.0.1", server->GetLocalPort()));
    client->Send(binary);

    ASSERT_TRUE(NetPumpUntil([&] { return received.size() == 5; }));
    EXPECT_TRUE(received == binary);

    client->Close();
    server->Close();
}

TEST(TcpMessage, CoroutineClientConversation)
{
    auto server = mmake<TcpMessageServer>();
    ASSERT_TRUE(server->Listen(0));

    server->onClientMessage = [&](int clientId, const String& message)
    {
        server->SendTo(clientId, "re: " + message);
    };

    auto client = mmake<TcpMessageChannel>();

    auto coroutine = [](Ref<TcpMessageChannel> client, int port) -> Coroutine<String>
    {
        if (!co_await client->ConnectAsync("127.0.0.1", port))
            co_return String("connect failed");

        client->Send("first");
        String first = co_await client->ReceiveAsync();

        client->Send("second");
        String second = co_await client->ReceiveAsync();

        co_return first + "|" + second;
    }(client, server->GetLocalPort());

    coroutine.Start(JobThread::Main);

    ASSERT_TRUE(NetPumpUntil([&] { return coroutine.IsDone(); }));
    EXPECT_EQ(coroutine.GetResult(), String("re: first|re: second"));

    client->Close();
    server->Close();
}

TEST(TcpMessage, BroadcastReachesAllClients)
{
    auto server = mmake<TcpMessageServer>();
    ASSERT_TRUE(server->Listen(0));

    int connectedCount = 0;
    server->onClientConnected = [&](int) { connectedCount++; };

    Vector<Ref<TcpMessageChannel>> clients;
    Vector<String> received;
    received.Resize(3);

    for (int i = 0; i < 3; i++)
    {
        auto client = mmake<TcpMessageChannel>();
        client->onMessage = [&received, i](const String& message) { received[i] = message; };
        ASSERT_TRUE(client->Connect("127.0.0.1", server->GetLocalPort()));
        clients.Add(client);
    }

    ASSERT_TRUE(NetPumpUntil([&] { return connectedCount == 3; }));
    EXPECT_EQ(server->GetClientIds().Count(), 3);

    server->Broadcast("to everyone");

    ASSERT_TRUE(NetPumpUntil([&] { return received.Count([](const String& r) { return !r.IsEmpty(); }) == 3; }));
    for (int i = 0; i < 3; i++)
        EXPECT_EQ(received[i], String("to everyone"));

    for (auto& client : clients)
        client->Close();
    server->Close();
}

TEST(TcpMessage, DisconnectClientFiresClosedOnClient)
{
    auto server = mmake<TcpMessageServer>();
    ASSERT_TRUE(server->Listen(0));

    int connectedId = 0;
    server->onClientConnected = [&](int clientId) { connectedId = clientId; };

    auto client = mmake<TcpMessageChannel>();
    bool clientClosed = false;
    client->onClosed = [&] { clientClosed = true; };

    ASSERT_TRUE(client->Connect("127.0.0.1", server->GetLocalPort()));
    ASSERT_TRUE(NetPumpUntil([&] { return connectedId != 0; }));

    int disconnectedId = 0;
    server->onClientDisconnected = [&](int clientId) { disconnectedId = clientId; };
    server->DisconnectClient(connectedId);

    ASSERT_TRUE(NetPumpUntil([&] { return clientClosed; }));
    EXPECT_EQ(disconnectedId, connectedId);
    EXPECT_EQ(server->GetClientIds().Count(), 0);

    server->Close();
}

TEST(TcpMessage, ClientDisconnectRemovesFromServer)
{
    auto server = mmake<TcpMessageServer>();
    ASSERT_TRUE(server->Listen(0));

    int connectedCount = 0;
    int disconnectedId = 0;
    server->onClientConnected = [&](int) { connectedCount++; };
    server->onClientDisconnected = [&](int clientId) { disconnectedId = clientId; };

    auto client = mmake<TcpMessageChannel>();
    ASSERT_TRUE(client->Connect("127.0.0.1", server->GetLocalPort()));
    ASSERT_TRUE(NetPumpUntil([&] { return connectedCount == 1; }));

    client->Close();

    ASSERT_TRUE(NetPumpUntil([&] { return disconnectedId != 0; }));
    EXPECT_EQ(server->GetClientIds().Count(), 0);

    server->Close();
}
