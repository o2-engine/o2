#include "o2/stdafx.h"

#if IS_SCRIPTING_SUPPORTED

#include <gtest/gtest.h>

#include "Network/NetworkTestHelpers.h"
#include "Network/TestHttpServer.h"
#include "o2/Network/Sockets/TcpMessageServer.h"
#include "o2/Network/Sockets/UdpSocket.h"
#include "o2/Scripts/ScriptEngine.h"

using namespace o2;

namespace
{
    ScriptValue EvalChecked(const String& code)
    {
        ScriptValue result = o2Scripts.Eval(code);
        EXPECT_NE(result.GetValueType(), ScriptValue::ValueType::Error) << result.GetError().Data();
        return result;
    }

    ScriptValue Global(const char* name)
    {
        return o2Scripts.GetGlobal().GetProperty(name);
    }
}

TEST(NetworkScript, HttpGetWithCallback)
{
    auto server = mmake<TestHttpServer>();
    ASSERT_TRUE(server->Start());

    EvalChecked("var netScript_status = 0; var netScript_body = ''; var netScript_success = false;");
    EvalChecked("o2.Http.Get('" + server->GetBaseUrl() + "/json', function(response) {"
                "    netScript_status = response.status;"
                "    netScript_body = response.body;"
                "    netScript_success = response.IsSuccess();"
                "});");

    ASSERT_TRUE(NetPumpUntil([] { return Global("netScript_status").ToNumber() == 200.0f; }));
    EXPECT_TRUE((bool)Global("netScript_success"));
    EXPECT_TRUE(Global("netScript_body").ToString().find("\"name\"") != std::string::npos);
}

TEST(NetworkScript, HttpResponseJsonParsedInJs)
{
    auto server = mmake<TestHttpServer>();
    ASSERT_TRUE(server->Start());

    EvalChecked("var netScript_name = ''; var netScript_value = 0; var netScript_items = 0;");
    EvalChecked("o2.Http.Get('" + server->GetBaseUrl() + "/json', function(response) {"
                "    var json = response.GetJson();"
                "    netScript_name = json.name;"
                "    netScript_value = json.value;"
                "    netScript_items = json.items.length;"
                "});");

    ASSERT_TRUE(NetPumpUntil([] { return Global("netScript_value").ToNumber() == 42.0f; }));
    EXPECT_EQ(Global("netScript_name").ToString(), String("o2"));
    EXPECT_EQ((int)Global("netScript_items").ToNumber(), 3);
}

TEST(NetworkScript, HttpPostAndConfiguredRequest)
{
    auto server = mmake<TestHttpServer>();
    ASSERT_TRUE(server->Start());

    EvalChecked("var netScript_postBody = '';");
    EvalChecked("o2.Http.Post('" + server->GetBaseUrl() + "/echo', 'js data', function(response) {"
                "    netScript_postBody = response.body;"
                "});");

    ASSERT_TRUE(NetPumpUntil([] { return !Global("netScript_postBody").ToString().IsEmpty(); }));
    EXPECT_EQ(Global("netScript_postBody").ToString(), String("POST|js data"));

    // A configured request object built in JS
    EvalChecked("var netScript_headerEcho = '';");
    EvalChecked("var netScript_request = new o2.HttpRequest();"
                "netScript_request.url = '" + server->GetBaseUrl() + "/echo';"
                "netScript_request.method = 'Put';"
                "netScript_request.body = 'put payload';"
                "o2.Http.Send(netScript_request, function(response) {"
                "    netScript_headerEcho = response.body;"
                "});");

    ASSERT_TRUE(NetPumpUntil([] { return !Global("netScript_headerEcho").ToString().IsEmpty(); }));
    EXPECT_EQ(Global("netScript_headerEcho").ToString(), String("PUT|put payload"));
}

TEST(NetworkScript, TcpMessageChannelChat)
{
    auto server = mmake<TcpMessageServer>();
    ASSERT_TRUE(server->Listen(0));

    server->onClientMessage = [&](int clientId, const String& message)
    {
        server->SendTo(clientId, "server echo: " + message);
    };

    EvalChecked("var netScript_chatMessage = ''; var netScript_chatConnected = false;");
    EvalChecked("var netScript_channel = new o2.TcpMessageChannel();"
                "netScript_channel.onConnected = function(success) { netScript_chatConnected = success; };"
                "netScript_channel.onMessage = function(message) { netScript_chatMessage = message; };"
                "netScript_channel.Connect('127.0.0.1', " + String(server->GetLocalPort()) + ");"
                "netScript_channel.Send('hello from js');");

    ASSERT_TRUE(NetPumpUntil([] { return !Global("netScript_chatMessage").ToString().IsEmpty(); }));
    EXPECT_TRUE((bool)Global("netScript_chatConnected"));
    EXPECT_EQ(Global("netScript_chatMessage").ToString(), String("server echo: hello from js"));

    EvalChecked("netScript_channel.Close();");
}

TEST(NetworkScript, UdpSocketExchange)
{
    // C++ side echoes datagrams back to the sender
    auto echoPeer = mmake<UdpSocket>();
    ASSERT_TRUE(echoPeer->Open());
    echoPeer->onDataReceived = [&](const String& data, const String& address, int port)
    {
        echoPeer->SendTo(address, port, "pong: " + data);
    };

    EvalChecked("var netScript_udpMessage = '';");
    EvalChecked("var netScript_udp = new o2.UdpSocket();"
                "netScript_udp.Open(0);"
                "netScript_udp.onDataReceived = function(data, address, port) { netScript_udpMessage = data; };"
                "netScript_udp.SendTo('127.0.0.1', " + String(echoPeer->GetLocalPort()) + ", 'ping');");

    ASSERT_TRUE(NetPumpUntil([] { return !Global("netScript_udpMessage").ToString().IsEmpty(); }));
    EXPECT_EQ(Global("netScript_udpMessage").ToString(), String("pong: ping"));

    EvalChecked("netScript_udp.Close();");
}

#endif // IS_SCRIPTING_SUPPORTED
