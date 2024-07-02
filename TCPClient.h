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
        
        bool Connect(const std::string& host, uint16_t port);
        
        void Disconnect();
        
        bool IsConnected();
        
        void Send(const TCPMsg& msg);
        
        TCPMsgQueue<TCPMsgOwned>& Incoming();
        
    private:
        std::unique_ptr<TCPClientImpl> pimpl;
    };
    
} // TCPConn


#endif //ZMONITOR_TCPCLIENT_H
