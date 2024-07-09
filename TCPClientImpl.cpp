//
// Created by Bohan Leng on 7/2/2024.
//

#include "TCPClientImpl.h"
#include "TCPConnImpl.h"
#include "LogMacros.h"

namespace TCPConn {

    /* ----- ITCPClient ----- */

    template <typename T>
    ITCPClient<T>::ITCPClient() {
        pimpl = std::make_unique<TCPClientImpl<T>>(*this);
    }

    template <typename T>
    ITCPClient<T>::~ITCPClient() = default;

    template <typename T>
    bool ITCPClient<T>::Connect(const std::string& host, const uint16_t port) {
        return pimpl->Connect(host, port);
    }

    template <typename T>
    void ITCPClient<T>::Disconnect() {
        pimpl->Disconnect();
    }

    template <typename T>
    bool ITCPClient<T>::IsConnected() {
        return pimpl->IsConnected();
    }

    template <typename T>
    void ITCPClient<T>::Send(const T& msg) {
        pimpl->Send(msg);
    }

    template <typename T>
    TCPMsgQueue<TCPMsgOwned<T>>& ITCPClient<T>::Incoming() {
        return pimpl->Incoming();
    }

    template <typename T>
    void ITCPClient<T>::Update(size_t nMaxMessages, bool bWait) {
        pimpl->Update(nMaxMessages, bWait);
    }


    /* ----- TCPClientImpl ----- */

    template <typename T>
    TCPClientImpl<T>::TCPClientImpl(ITCPClient<T>& interface)
        : _interface(interface), m_socket(m_context) {}

    template <typename T>
    TCPClientImpl<T>::~TCPClientImpl() {
        m_bIsDestroying = true;
        Disconnect();
    }

    template <typename T>
    bool TCPClientImpl<T>::Connect(const std::string& host, const uint16_t port) {
        try {
            ip::tcp::resolver resolver(m_context);
            ip::tcp::resolver::results_type endpoint = resolver.resolve(host, std::to_string(port));

            struct ITCPConn<T>::TCPContext tcp_context{m_context, ip::tcp::socket(m_context)};
            m_connection = std::make_unique<ITCPConn<T>>(ITCPConn<T>::EOwner::client, tcp_context, m_qMessagesIn);

            INFO_MSG("Connecting to %s:%d", host.c_str(), port);
            struct ITCPConn<T>::TCPEndpoint tcp_endpoint{endpoint};
            m_connection->ConnectToServer(tcp_endpoint, [this]() { _interface.OnConnected(); });

            m_thrContext = std::thread([this]() { m_context.run(); });

            return true;
        } catch (std::exception& e) {
            ERROR_MSG("Client Exception: %s", e.what());
            return false;
        }
    }

    template <typename T>
    void TCPClientImpl<T>::Disconnect() {
        if (IsConnected()) {
            m_connection->Disconnect();
        }
        m_context.stop();
        if (m_thrContext.joinable()) {
            m_thrContext.join();
        }
        m_connection.reset();
        if(!m_bIsDestroying) _interface.OnDisconnected();
    }

    template <typename T>
    bool TCPClientImpl<T>::IsConnected() {
        return m_connection ? m_connection->IsConnected() : false;
    }

    template <typename T>
    void TCPClientImpl<T>::Send(const T& msg) {
        if (IsConnected()) m_connection->Send(msg);
    }

    template <typename T>
    TCPMsgQueue<TCPMsgOwned<T>>& TCPClientImpl<T>::Incoming() {
        return m_qMessagesIn;
    }

    template <typename T>
    void TCPClientImpl<T>::Update(size_t nMaxMessages, bool bWait) {
        if (bWait) m_qMessagesIn.wait();
        size_t nMessageCount = 0;
        while (nMessageCount < nMaxMessages && !m_qMessagesIn.empty()) {
            auto msg = m_qMessagesIn.pop_front();
            _interface.OnMessage(msg.msg);
            nMessageCount++;
        }
    }

} // TCPConn