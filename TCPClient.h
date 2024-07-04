//
// Created by Bohan Leng on 7/2/2024.
//

#ifndef ZMONITOR_TCPCLIENT_H
#define ZMONITOR_TCPCLIENT_H

#include "TCPConn.h"

namespace TCPConn {
         
    class TCPClientImpl;

    class TCPCONN_API ITCPClient {
    public:
        ITCPClient();
        virtual ~ITCPClient();
        
        /// \brief Connect to a server
        /// \param host server address or domain name
        /// \param port server port
        bool Connect(const std::string& host, uint16_t port);
        
        /// \brief Disconnect from the server, will be called automatically on destruction
        void Disconnect();
        
        /// \brief Check if the client is connected
        /// \return true if the socket is open
        bool IsConnected();
        
        /// \brief Send a message to the server
        /// \param msg message to send
        void Send(const TCPMsg& msg);
        
        /// \brief Get the incoming message queue
        /// \return reference to the incoming message queue
        TCPMsgQueue<TCPMsgOwned>& Incoming();

        /// \brief On connected to server
        virtual void OnConnected() = 0;

        /// \brief On disconnection from server
        virtual void OnDisconnected() = 0;

        /// \brief On message received, must be overridden
        /// \param msg received message
        virtual void OnMessage(TCPMsg& msg) = 0;

    private:
        std::unique_ptr<TCPClientImpl> pimpl;
    };
    
} // TCPConn


#endif //ZMONITOR_TCPCLIENT_H
