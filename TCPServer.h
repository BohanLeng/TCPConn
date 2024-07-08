//
// Created by Bohan Leng on 7/2/2024.
//

#ifndef ZMONITOR_TCPSERVER_H
#define ZMONITOR_TCPSERVER_H

#include "TCPConn.h"

namespace TCPConn {
    
    class TCPServerImpl;
    
    class TCPCONN_API ITCPServer {
    public:

        /// \brief Construct a new ITCPServer object, must override `OnMessage()`;
        /// \param port 
        explicit ITCPServer(uint16_t port);
        virtual ~ITCPServer();
        
        
        /// \brief Start the server
        /// \return true if server started successfully
        bool Start();
        
        /// \brief Stop the server
        void Stop();
        
        
        /// \brief Message a client 
        void MessageClient(std::shared_ptr<ITCPConn> client, const TCPMsg& msg);
        /// \brief Message all clients
        void MessageAllClients(const TCPMsg& msg, std::shared_ptr<ITCPConn> pIgnoreClient = nullptr);
        
        
        /// \brief Actively consume messages in the message queue
        /// \param nMaxMessages maximum number of messages to consume, default is -1, consume all
        void Update(size_t nMaxMessages = -1, bool bWait = true);
        
        
        /// \brief On new connection request, pending approval to establish connection
        /// \param client newly created socket pointer on server for this connection
        /// \return true to accept the connection, false to deny
        virtual bool OnClientConnectionRequest(std::shared_ptr<ITCPConn> client) = 0;
        
        /// \brief On client connection established
        /// \param client socket pointer to the connected client
        virtual void OnClientConnected(std::shared_ptr<ITCPConn> client) = 0;
        
        /// \brief On client disconnection, will be called on sending attempt to a already disconnected client
        /// \param client socket pointer to the disconnected client
        virtual void OnClientDisconnected(std::shared_ptr<ITCPConn> client) = 0;
        
        /// \brief On message received, must be overridden
        /// \param client socket pointer to the client that sent the message
        /// \param msg received message
        virtual void OnMessage(std::shared_ptr<ITCPConn> client, TCPMsg& msg) = 0;

    private:
        std::unique_ptr<TCPServerImpl> pimpl;
    };
    
}


#endif //ZMONITOR_TCPSERVER_H
