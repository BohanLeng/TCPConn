//
// Created by Bohan Leng on 7/2/2024.
//

#include "TCPClientImpl.h"
#include "TCPConnImpl.h"

namespace TCPConn {

    /* ----- ITCPClient ----- */

    ITCPClient::ITCPClient() {
        pimpl = std::make_unique<TCPClientImpl>();
    }
    
    ITCPClient::~ITCPClient() {
        pimpl->Disconnect();
    }
    
    bool ITCPClient::Connect(const std::string &host, const uint16_t port) {
        return pimpl->Connect(host, port);
    }
    
    void ITCPClient::Disconnect() {
        pimpl->Disconnect();
    }
    
    bool ITCPClient::IsConnected() {
        return pimpl->IsConnected();
    }
    
    void ITCPClient::Send(const TCPMsg &msg) {
        pimpl->Send(msg);
    }
    
    TCPMsgQueue<TCPMsgOwned> &ITCPClient::Incoming() {
        return pimpl->Incoming();
    }

    /* ----- TCPClientImpl ----- */

    TCPClientImpl::TCPClientImpl()
        : m_socket(m_context) {}

    TCPClientImpl::~TCPClientImpl() {
        Disconnect();
    }

    bool TCPClientImpl::Connect(const std::string &host, const uint16_t port) {
        try {
            ip::tcp::resolver resolver(m_context);
            ip::tcp::resolver::results_type endpoint = resolver.resolve(host, std::to_string(port));

            ITCPConn::TCPContext tcp_context{m_context, ip::tcp::socket(m_context)};
            m_connection = std::make_unique<ITCPConn>(ITCPConn::EOwner::client, tcp_context, m_qMessagesIn);

            ITCPConn::TCPEndpoint tcp_endpoint{endpoint};
            m_connection->ConnectToServer(tcp_endpoint);

            m_thrContext = std::thread([this]() { m_context.run(); });
        } catch (std::exception &e) {
            std::cerr << "Client Exception: " << e.what() << std::endl;
            return false;
        }
        return true;
    }

    void TCPClientImpl::Disconnect() {
        if (IsConnected()) {
            m_connection->Disconnect();
        }
        m_context.stop();
        if (m_thrContext.joinable()) {
            m_thrContext.join();
        }
        m_connection.release();
    }

    bool TCPClientImpl::IsConnected() {
        if (m_connection) {
            return m_connection->IsConnected();
        } else {
            return false;
        }
    }

    void TCPClientImpl::Send(const TCPMsg &msg) {
        if (IsConnected()) {
            m_connection->Send(msg);
        }
    }

    TCPMsgQueue<TCPMsgOwned> &TCPClientImpl::Incoming() {
        return m_qMessagesIn;
    }

} // TCPConn