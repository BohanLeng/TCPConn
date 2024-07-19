//
// Created by Bohan Leng on 7/2/2024.
//

#ifndef ZMONITOR_TCPCLIENT_H
#define ZMONITOR_TCPCLIENT_H

#include "TCPConn.h"

namespace TCPConn {

    template <typename T>
    class TCPClientImpl;

    template <typename T>
    class TCPCONN_API ITCPClient {
    public:
        ITCPClient();
        virtual ~ITCPClient();
        
        /// \brief Connect to a server.
        /// \param host server address or domain name
        /// \param port server port
        bool Connect(const std::string& host, uint16_t port);
        
        /// \brief Disconnect from the server, will be called automatically on destruction.
        void Disconnect();
        
        /// \brief Check if the client is connected.
        /// \return true if the socket is open
        [[nodiscard]] bool IsConnected() const;
        
        /// \brief Send a message to the server.
        /// \param msg message to send
        void Send(const T& msg) const;
        
        /// \brief Get the incoming message queue.
        /// \return reference to the incoming message queue
        TCPMsgQueue<TCPMsgOwned<T>>& Incoming() const;
        
        /// \brief Actively consume messages in the message queue.
        /// \param nMaxMessages maximum number of messages to consume, default is -1, consume all
        /// \param bWait whether to block to wait for incoming messages, must be true if used in a loop
        void Update(size_t nMaxMessages = -1, bool bWait = true);

        /// \brief Start continuous update messages.
        /// Will block the current thread. Pending signals to terminate.
        void Run();

        /// \brief On connected to server.
        virtual void OnConnected() {}

        /// \brief On disconnection from server.
        virtual void OnDisconnected() {}

        /// \brief On message received, must be overridden.
        /// \param msg received message
        virtual void OnMessage(T& msg) = 0;

    private:
        std::unique_ptr<TCPClientImpl<T>> pimpl;
    };
    
} // TCPConn


#endif //ZMONITOR_TCPCLIENT_H
