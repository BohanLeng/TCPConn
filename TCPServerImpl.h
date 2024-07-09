//
// Created by Bohan Leng on 7/1/2024.
//

#ifndef ZMONITOR_TCPSERVERIMPL_H
#define ZMONITOR_TCPSERVERIMPL_H

#include "TCPServer.h"
#include <boost/asio.hpp>

using namespace boost::asio;

namespace TCPConn {

    template <typename T>
    class TCPServerImpl {
    public:
        TCPServerImpl(ITCPServer<T>& interface, uint16_t port);
        virtual ~TCPServerImpl();

        bool Start();
        void Stop();

        void WaitForClientConnection();

        void MessageClient(std::shared_ptr<ITCPConn<T>> client, const T& msg);
        void MessageAllClients(const T& msg, std::shared_ptr<ITCPConn<T>> pIgnoreClient = nullptr);

        void Update(size_t nMaxMessages = -1, bool bWait = true);

    protected:
        TCPMsgQueue<TCPMsgOwned<T>> m_qMessagesIn;
        std::deque<std::shared_ptr<ITCPConn<T>>> m_deqConns;
        io_context m_context;
        std::thread m_thrContext;
        ip::tcp::acceptor m_acceptor;
        uint32_t m_idCounter = 10000;
        
    private:
        ITCPServer<T>& _interface;
    };

} // TCPConn


#endif //ZMONITOR_TCPSERVERIMPL_H
