#include "o2/stdafx.h"
#include "TcpMessageServer.h"

#include "o2/Network/NetworkSystem.h"
#include "o2/Utils/Debug/Log/LogStream.h"

namespace o2
{
    void EncodeNetMessage(String& buffer, const String& message);

    TcpMessageServer::TcpMessageServer(RefCounter* refCounter):
        RefCounterable(refCounter)
    {}

    TcpMessageServer::~TcpMessageServer()
    {
        Close();
    }

    bool TcpMessageServer::Listen(int port)
    {
        if (mListener)
        {
            o2Debug.LogError("TcpMessageServer::Listen: already listening");
            return false;
        }

        mListener = mmake<TcpListener>();
        mListener->onAccepted = [this](const Ref<TcpSocket>& socket) { OnAccepted(socket); };

        if (!mListener->Listen(port))
        {
            mListener = nullptr;
            return false;
        }

        return true;
    }

    int TcpMessageServer::GetLocalPort() const
    {
        return mListener ? mListener->GetLocalPort() : 0;
    }

    bool TcpMessageServer::IsListening() const
    {
        return mListener && mListener->IsListening();
    }

    bool TcpMessageServer::SendTo(int clientId, const String& message)
    {
        auto client = FindClient(clientId);
        if (!client)
            return false;

        String framed;
        EncodeNetMessage(framed, message);
        client->socket->Send(framed);
        return true;
    }

    void TcpMessageServer::Broadcast(const String& message)
    {
        String framed;
        EncodeNetMessage(framed, message);

        auto clients = mClients;
        for (auto& client : clients)
            client.socket->Send(framed);
    }

    void TcpMessageServer::DisconnectClient(int clientId)
    {
        auto client = FindClient(clientId);
        if (!client)
            return;

        auto socket = client->socket;
        mClients.RemoveAll([&](const Client& other) { return other.id == clientId; });
        socket->Close();

        onClientDisconnected(clientId);
    }

    Vector<int> TcpMessageServer::GetClientIds() const
    {
        Vector<int> ids;
        for (auto& client : mClients)
            ids.Add(client.id);

        return ids;
    }

    String TcpMessageServer::GetClientAddress(int clientId) const
    {
        auto client = FindClient(clientId);
        return client ? client->socket->GetRemoteAddress() : String();
    }

    void TcpMessageServer::Close()
    {
        if (!mListener)
            return;

        auto listener = mListener;
        mListener = nullptr;

        auto clients = mClients;
        mClients.Clear();

        listener->Close();
        for (auto& client : clients)
            client.socket->Close();
    }

    void TcpMessageServer::OnAccepted(const Ref<TcpSocket>& socket)
    {
        Client client;
        client.id = mNextClientId++;
        client.socket = socket;
        mClients.Add(client);

        int clientId = client.id;
        socket->onDataReceived = [this, clientId](const String& data) { ProcessClientData(clientId, data); };
        socket->onClosed = [this, clientId] { OnClientClosed(clientId); };

        onClientConnected(clientId);
    }

    void TcpMessageServer::ProcessClientData(int clientId, const String& data)
    {
        auto client = FindClient(clientId);
        if (!client)
            return;

        client->receiveBuffer.append(data.data(), data.size());

        while (true)
        {
            client = FindClient(clientId);
            if (!client || client->receiveBuffer.size() < 4)
                break;

            const unsigned char* prefix = (const unsigned char*)client->receiveBuffer.data();
            UInt32 messageSize = (UInt32)prefix[0] | ((UInt32)prefix[1] << 8) |
                ((UInt32)prefix[2] << 16) | ((UInt32)prefix[3] << 24);

            if ((int)messageSize > maxMessageSize)
            {
                o2Network.GetLog()->Error("TcpMessageServer: incoming message size " + String((int)messageSize) +
                                          " from client " + String(clientId) + " exceeds the limit, disconnecting");
                DisconnectClient(clientId);
                return;
            }

            if (client->receiveBuffer.size() < 4 + (size_t)messageSize)
                break;

            String message;
            message.assign(client->receiveBuffer.data() + 4, (size_t)messageSize);
            client->receiveBuffer.erase(0, 4 + (size_t)messageSize);

            onClientMessage(clientId, message);
        }
    }

    void TcpMessageServer::OnClientClosed(int clientId)
    {
        if (!FindClient(clientId))
            return;

        mClients.RemoveAll([&](const Client& other) { return other.id == clientId; });
        onClientDisconnected(clientId);
    }

    TcpMessageServer::Client* TcpMessageServer::FindClient(int clientId)
    {
        for (auto& client : mClients)
        {
            if (client.id == clientId)
                return &client;
        }

        return nullptr;
    }

    const TcpMessageServer::Client* TcpMessageServer::FindClient(int clientId) const
    {
        for (auto& client : mClients)
        {
            if (client.id == clientId)
                return &client;
        }

        return nullptr;
    }
}
// --- META ---

DECLARE_CLASS(o2::TcpMessageServer, o2__TcpMessageServer);
// --- END META ---
