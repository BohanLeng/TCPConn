//
// Created by Bohan Leng on 7/1/2024.
//

#ifndef ZMONITOR_TCPSERVERIMPL_H
#define ZMONITOR_TCPSERVERIMPL_H

#include "TCPServer.h"
#include <boost/asio.hpp>

using namespace boost::asio;

namespace TCPConn {
    
    class TCPServerImpl {
    public:
        TCPServerImpl(ITCPServer& interface, uint16_t port);
        virtual ~TCPServerImpl();

        bool Start();
        bool Stop();

        void WaitForClientConnection();

        void MessageClient(std::shared_ptr<ITCPConn> client, const TCPMsg& msg);
        void MessageAllClients(const TCPMsg& msg, std::shared_ptr<ITCPConn> pIgnoreClient = nullptr);

        void Update(size_t nMaxMessages = -1, bool bWait = true);

    protected:
        TCPMsgQueue<TCPMsgOwned> m_qMessagesIn;
        std::deque<std::shared_ptr<ITCPConn>> m_deqConns;
        io_context m_context;
        std::thread m_thrContext;
        ip::tcp::acceptor m_acceptor;
        uint32_t m_idCounter = 10000;
        
    private:
        ITCPServer& _interface;
    };

} // TCPConn


#endif //ZMONITOR_TCPSERVERIMPL_H
