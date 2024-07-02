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
        virtual ~ITCPServer();
        
        bool Start();
        void Stop();
        void WaitForClientConnection();
        
        void MessageClient(std::shared_ptr<ITCPConn> client, const TCPMsg& msg);
        void MessageAllClients(const TCPMsg &msg, std::shared_ptr<ITCPConn> pIgnoreClient = nullptr);
        
        void Update(size_t nMaxMessages = -1);
        
        virtual bool OnClientConnected(std::shared_ptr<ITCPConn> client) { return true; }
        virtual void OnClientDisconnected(std::shared_ptr<ITCPConn> client) {}
        virtual void OnMessage(std::shared_ptr<ITCPConn> client, TCPMsg& msg) {}
        
        explicit ITCPServer(uint16_t port);

    private:
        uint16_t _port;
        std::unique_ptr<TCPServerImpl> pimpl;
    };
    
}


#endif //ZMONITOR_TCPSERVER_H
