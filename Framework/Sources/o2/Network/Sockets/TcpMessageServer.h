#pragma once

#include "o2/Network/Sockets/TcpListener.h"
#include "o2/Network/Sockets/TcpSocket.h"
#include "o2/Utils/Basic/IObject.h"

namespace o2
{
    // -------------------------------------------------------------------------------------------
    // Message-oriented TCP server for chats, lobbies and other whole-message exchanges. Accepts
    // clients, assigns them numeric ids and splits their streams into length-prefixed messages
    // with the same framing as TcpMessageChannel. Messages are o2::String, which is binary-safe
    // -------------------------------------------------------------------------------------------
    class TcpMessageServer: public IObject, public RefCounterable
    {
    public:
        Function<void(int clientId)>                        onClientConnected;    // Called when a client connects @SCRIPTABLE
        Function<void(int clientId, const String& message)> onClientMessage;      // Called for every received whole message @SCRIPTABLE
        Function<void(int clientId)>                        onClientDisconnected; // Called when a client disconnects @SCRIPTABLE

        int maxMessageSize = 16*1024*1024; // Incoming message size limit; a bigger prefix disconnects the client

    public:
        // Default constructor @SCRIPTABLE
        explicit TcpMessageServer(RefCounter* refCounter);

        // Destructor, closes the server
        ~TcpMessageServer() override;

        // Starts listening on the port. Port 0 binds an ephemeral port, see GetLocalPort.
        // Returns false on failure @SCRIPTABLE
        bool Listen(int port);

        // Returns the port the server is bound to, 0 when closed @SCRIPTABLE
        int GetLocalPort() const;

        // Returns true while listening @SCRIPTABLE
        bool IsListening() const;

        // Sends one message to the client. Returns false when the client is unknown @SCRIPTABLE
        bool SendTo(int clientId, const String& message);

        // Sends one message to every connected client @SCRIPTABLE
        void Broadcast(const String& message);

        // Disconnects the client @SCRIPTABLE
        void DisconnectClient(int clientId);

        // Returns ids of the connected clients @SCRIPTABLE
        Vector<int> GetClientIds() const;

        // Returns the remote address of the client, empty when unknown @SCRIPTABLE
        String GetClientAddress(int clientId) const;

        // Closes the server and disconnects all clients @SCRIPTABLE
        void Close();

        IOBJECT(TcpMessageServer);

    protected:
        // Connected client entry
        struct Client
        {
            int            id = 0;       // Client id
            Ref<TcpSocket> socket;       // Client connection
            String         receiveBuffer; // Stream bytes not yet split into messages

            // Returns true when ids match
            bool operator==(const Client& other) const { return id == other.id; }
        };

        Ref<TcpListener> mListener;       // Listening socket
        Vector<Client>   mClients;        // Connected clients
        int              mNextClientId = 1; // Id for the next accepted client

    protected:
        // Accepts the connection: assigns an id and subscribes to the socket events
        void OnAccepted(const Ref<TcpSocket>& socket);

        // Splits the client's buffered stream bytes into whole messages and dispatches them
        void ProcessClientData(int clientId, const String& data);

        // Removes the client and fires onClientDisconnected
        void OnClientClosed(int clientId);

        // Returns the client entry by id, null when unknown
        Client* FindClient(int clientId);

        // Returns the client entry by id, null when unknown
        const Client* FindClient(int clientId) const;
    };
}
// --- META ---

CLASS_BASES_META(o2::TcpMessageServer)
{
    BASE_CLASS(o2::IObject);
    BASE_CLASS(o2::RefCounterable);
}
END_META;
CLASS_FIELDS_META(o2::TcpMessageServer)
{
    FIELD().PUBLIC().SCRIPTABLE_ATTRIBUTE().NAME(onClientConnected);
    FIELD().PUBLIC().SCRIPTABLE_ATTRIBUTE().NAME(onClientMessage);
    FIELD().PUBLIC().SCRIPTABLE_ATTRIBUTE().NAME(onClientDisconnected);
    FIELD().PUBLIC().DEFAULT_VALUE(16*1024*1024).NAME(maxMessageSize);
    FIELD().PROTECTED().NAME(mListener);
    FIELD().PROTECTED().NAME(mClients);
    FIELD().PROTECTED().DEFAULT_VALUE(1).NAME(mNextClientId);
}
END_META;
CLASS_METHODS_META(o2::TcpMessageServer)
{

    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(bool, Listen, int);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(int, GetLocalPort);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(bool, IsListening);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(bool, SendTo, int, const String&);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, Broadcast, const String&);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, DisconnectClient, int);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(Vector<int>, GetClientIds);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(String, GetClientAddress, int);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, Close);
    FUNCTION().PROTECTED().SIGNATURE(void, OnAccepted, const Ref<TcpSocket>&);
    FUNCTION().PROTECTED().SIGNATURE(void, ProcessClientData, int, const String&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnClientClosed, int);
    FUNCTION().PROTECTED().SIGNATURE(Client*, FindClient, int);
    FUNCTION().PROTECTED().SIGNATURE(const Client*, FindClient, int);
}
END_META;
// --- END META ---
