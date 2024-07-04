//
// Created by Bohan Leng on 7/2/2024.
//

#include "TCPClientImpl.h"
#include "TCPConnImpl.h"
#include "LogMacros.h"

namespace TCPConn {

    /* ----- ITCPClient ----- */

    ITCPClient::ITCPClient() {
        pimpl = std::make_unique<TCPClientImpl>(*this);
    }
    
    ITCPClient::~ITCPClient() = default;
    
    bool ITCPClient::Connect(const std::string& host, const uint16_t port) {
        return pimpl->Connect(host, port);
    }
    
    void ITCPClient::Disconnect() {
        pimpl->Disconnect();
    }
    
    bool ITCPClient::IsConnected() {
        return pimpl->IsConnected();
    }
    
    void ITCPClient::Send(const TCPMsg& msg) {
        pimpl->Send(msg);
    }
    
    TCPMsgQueue<TCPMsgOwned>& ITCPClient::Incoming() {
        return pimpl->Incoming();
    }

    
    /* ----- TCPClientImpl ----- */

    TCPClientImpl::TCPClientImpl(ITCPClient& interface)
        : _interface(interface), m_socket(m_context) {}

    TCPClientImpl::~TCPClientImpl() {
        Disconnect();
    }

    bool TCPClientImpl::Connect(const std::string& host, const uint16_t port) {
        try {
            ip::tcp::resolver resolver(m_context);
            ip::tcp::resolver::results_type endpoint = resolver.resolve(host, std::to_string(port));

            ITCPConn::TCPContext tcp_context{m_context, ip::tcp::socket(m_context)};
            m_connection = std::make_unique<ITCPConn>(ITCPConn::EOwner::client, tcp_context, m_qMessagesIn);

            ITCPConn::TCPEndpoint tcp_endpoint{endpoint};
            m_connection->ConnectToServer(tcp_endpoint);

            m_thrContext = std::thread([this]() { m_context.run(); });
            _interface.OnConnected();
            return true;
        } catch (std::exception& e) {
            ERROR_MSG("Client Exception: %s", e.what());
            return false;
        }
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
        _interface.OnDisconnected();
    }

    bool TCPClientImpl::IsConnected() {
        return m_connection ? m_connection->IsConnected() : false;
    }

    void TCPClientImpl::Send(const TCPMsg& msg) {
        if (IsConnected()) m_connection->Send(msg);
    }

    TCPMsgQueue<TCPMsgOwned>& TCPClientImpl::Incoming() {
        return m_qMessagesIn;
    }

    void TCPClientImpl::Update(size_t nMaxMessages, bool bWait) {
        if (bWait) m_qMessagesIn.wait();
        size_t nMessageCount = 0;
        while (nMessageCount < nMaxMessages && !m_qMessagesIn.empty()) {
            auto msg = m_qMessagesIn.pop_front();
            _interface.OnMessage(msg.msg);
            nMessageCount++;
        }
    }

} // TCPConn