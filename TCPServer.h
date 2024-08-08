//
// Created by Bohan Leng on 7/2/2024.
//

#ifndef ZMONITOR_TCPSERVER_H
#define ZMONITOR_TCPSERVER_H

#include "TCPConn.h"

namespace TCPConn {

    template <typename T>
    class TCPServerImpl;

    template <typename T>
    class TCPCONN_API ITCPServer {
    public:

        /// \brief Construct a new ITCPServer.
        /// \param port 
        explicit ITCPServer(uint16_t port);
        virtual ~ITCPServer();
        
        
        /// \brief Start the server.
        /// \return true if server started successfully
        bool Start();
        
        /// \brief Stop the server.
        void Stop();
        
        
        /// \brief Message a client.
        /// \param client socket pointer to the client
        /// \param msg message to send
        void MessageClient(std::shared_ptr<ITCPConn<T>> client, const T& msg) const;
        
        /// \brief Message all clients.
        /// \param msg message to send
        /// \param pIgnoreClient socket pointer to the client to ignore
        void MessageAllClients(const T& msg, std::shared_ptr<ITCPConn<T>> pIgnoreClient = nullptr) const;
        
        
        /// \brief Actively consume messages in the message queue.
        /// \param nMaxMessages maximum number of messages to consume, default is -1, consume all
        /// \param bWait whether to block to wait for incoming messages, must be true if used in a loop
        void Update(bool bWait, size_t nMaxMessages = -1);
        
        /// \brief Start continuous update messages.
        /// Will block the current thread. Pending signals to terminate.
        void Run();
        
        
        /// \brief On new connection request, pending approval to establish connection.
        /// \param client newly created socket pointer on server for this connection
        /// \return true to accept the connection, false to deny
        virtual bool OnClientConnectionRequest(std::shared_ptr<ITCPConn<T>> client) { return true; }
        
        /// \brief On client connection established, pending validation.
        /// \param client socket pointer to the connected client
        virtual void OnClientConnected(std::shared_ptr<ITCPConn<T>> client) {}
        
        /// \brief On client disconnection, will be called on sending attempt to a already disconnected client.
        /// \param client socket pointer to the disconnected client
        virtual void OnClientDisconnected(std::shared_ptr<ITCPConn<T>> client) {}
        
        /// \brief On message received, must be overridden.
        /// \param client socket pointer to the client that sent the message
        /// \param msg received message
        virtual void OnMessage(std::shared_ptr<ITCPConn<T>> client, T& msg) = 0;

    private:
        std::unique_ptr<TCPServerImpl<T>> pimpl;
    };
    
}


#endif //ZMONITOR_TCPSERVER_H
