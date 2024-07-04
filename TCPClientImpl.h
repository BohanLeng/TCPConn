//
// Created by Bohan Leng on 7/1/2024.
//

#ifndef ZMONITOR_TCPCLIENTIMPL_H
#define ZMONITOR_TCPCLIENTIMPL_H

#include "TCPClient.h"
#include <boost/asio.hpp>
#include <thread>

using namespace boost::asio;

namespace TCPConn {

    class TCPClientImpl {
    public:
        TCPClientImpl();
        virtual ~TCPClientImpl();

        bool Connect(const std::string& host, uint16_t port);
        void Disconnect();
        bool IsConnected();

        void Send(const TCPMsg& msg);

        TCPMsgQueue<TCPMsgOwned>& Incoming();

    protected:
        io_context m_context;
        std::thread m_thrContext;
        ip::tcp::socket m_socket;
        std::unique_ptr<ITCPConn> m_connection;

    private:
        TCPMsgQueue<TCPMsgOwned> m_qMessagesIn;
    };
    
} // TCPConn

#endif //ZMONITOR_TCPCLIENTIMPL_H
