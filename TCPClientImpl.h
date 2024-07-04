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
        TCPClientImpl(ITCPClient& interface);
        virtual ~TCPClientImpl();

        bool Connect(const std::string& host, uint16_t port);
        void Disconnect();
        bool IsConnected();

        void Send(const TCPMsg& msg);

        void Update(size_t nMaxMessages = -1, bool bWait = true);

        TCPMsgQueue<TCPMsgOwned>& Incoming();

    protected:
        io_context m_context;
        std::thread m_thrContext;
        ip::tcp::socket m_socket;
        std::unique_ptr<ITCPConn> m_connection;
        TCPMsgQueue<TCPMsgOwned> m_qMessagesIn;

    private:
        ITCPClient& _interface;
    };
    
} // TCPConn

#endif //ZMONITOR_TCPCLIENTIMPL_H
