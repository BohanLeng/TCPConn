//
// Created by Bohan Leng on 7/1/2024.
//

#ifndef ZMONITOR_TCPCONNIMPL_H
#define ZMONITOR_TCPCONNIMPL_H

#include "TCPConn.h"
#include <boost/asio.hpp>

using namespace boost::asio;

namespace TCPConn {

    struct ITCPConn::TCPContext {
        io_context &context;
        ip::tcp::socket socket;
    };
    
    struct ITCPConn::TCPEndpoint {
        ip::tcp::resolver::results_type &endpoint;
    };

    class TCPConnImpl {
    public:
        TCPConnImpl(ITCPConn& interface, ITCPConn::EOwner owner, 
                    struct ITCPConn::TCPContext& context, TCPMsgQueue<TCPMsgOwned>& qIn);
        virtual ~TCPConnImpl();

        [[nodiscard]] uint32_t GetID() const;

        void ConnectToClient(uint32_t uid = 0);
        void ConnectToServer(const struct ITCPConn::TCPEndpoint& endpoint);
        void Disconnect();
        [[nodiscard]] bool IsConnected() const;
        
        void Send(const TCPMsg &msg);

    protected:

        void ReadHeader();
        void ReadBody();
        void WriteHeader();
        void WriteBody();
        void AddToIncomingMessageQueue();
        
        ip::tcp::socket m_socket;
        io_context& m_context;
        TCPMsgQueue<TCPMsg> m_qMessagesOut;
        TCPMsgQueue<TCPMsgOwned>& m_qMessagesIn;
        TCPMsg m_msgTemporaryIn;
        ITCPConn::EOwner m_nOwnerType;
        uint32_t id = -1;
        
    private:
        ITCPConn& _interface;
    };

} // TCPConn

#endif //ZMONITOR_TCPCONNIMPL_H
