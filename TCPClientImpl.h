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

    template <typename T>
    class TCPClientImpl {
    public:
        explicit TCPClientImpl(ITCPClient<T>& interface);
        virtual ~TCPClientImpl();

        bool Connect(const std::string& host, uint16_t port);
        void Disconnect();
        bool IsConnected();

        void Send(const T& msg);

        void Update(size_t nMaxMessages = -1, bool bWait = true);

        TCPMsgQueue<TCPMsgOwned<T>>& Incoming();

    protected:
        io_context m_context;
        std::thread m_thrContext;
        ip::tcp::socket m_socket;
        std::unique_ptr<ITCPConn<T>> m_connection;
        TCPMsgQueue<TCPMsgOwned<T>> m_qMessagesIn;
        bool m_bIsDestroying;

    private:
        ITCPClient<T>& _interface;
    };
    
} // TCPConn

#endif //ZMONITOR_TCPCLIENTIMPL_H
