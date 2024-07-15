//
// Created by Bohan Leng on 7/3/2024.
//

#include "TCPServerImpl.h"
#include "TCPConnImpl.h"
#include "LogMacros.h"
#include "TCPServer.h"


namespace TCPConn {

    /* ----- ITCPServer ----- */

    template <typename T>
    ITCPServer<T>::ITCPServer(uint16_t port) 
    {
        pimpl = std::make_unique<TCPServerImpl<T>>(*this, port);
    }

    template <typename T>
    ITCPServer<T>::~ITCPServer() = default;

    template <typename T>
    bool ITCPServer<T>::Start() {
        return pimpl->Start();
    }

    template <typename T>
    void ITCPServer<T>::Stop() {
        pimpl->Stop();
    }

    template <typename T>
    void ITCPServer<T>::MessageClient(std::shared_ptr<ITCPConn<T>> client, const T& msg) const {
        pimpl->MessageClient(client, msg);
    }

    template <typename T>
    void ITCPServer<T>::MessageAllClients(const T& msg, std::shared_ptr<ITCPConn<T>> pIgnoreClient) const {
        pimpl->MessageAllClients(msg, pIgnoreClient);
    }

    template <typename T>
    void ITCPServer<T>::Update(size_t nMaxMessages, bool bWait) {
        pimpl->Update(nMaxMessages, bWait);
    }


    /* ----- TCPServerImpl ----- */

    template <typename T>
    TCPServerImpl<T>::TCPServerImpl(ITCPServer<T>& interface, uint16_t port)
            : _interface(interface), m_acceptor(m_context, ip::tcp::endpoint(ip::tcp::v4(), port)) {
    }

    template <typename T>
    TCPServerImpl<T>::~TCPServerImpl() {
        Stop();
    }

    template <typename T>
    bool TCPServerImpl<T>::Start() {
        try {
            WaitForClientConnection();
            m_thrContext = std::thread([this]() { m_context.run(); });
        }
        catch (std::exception& e) {
            ERROR_MSG("[SERVER] Exception: %s", e.what());
            return false;
        }
        INFO_MSG("[SERVER] Started!");
        m_bServerRunning = true;
        return true;
    }

    template <typename T>
    void TCPServerImpl<T>::Stop() {
        if (!m_bServerRunning) return;
        m_bServerRunning = false;
        m_context.stop();
        if (m_thrContext.joinable()) m_thrContext.join();
        INFO_MSG("[SERVER] Stopped!");
    }

    template <typename T>
    void TCPServerImpl<T>::WaitForClientConnection() {
        m_acceptor.async_accept(
                [this](std::error_code ec, ip::tcp::socket socket) {
                    if (!ec) {
                        INFO_MSG("[SERVER] New Connection: %s", socket.remote_endpoint().address().to_string().c_str());
                        struct ITCPConn<T>::TCPContext tcp_context{ m_context, std::move(socket) };
                        auto new_conn = std::make_shared<ITCPConn<T>>(ITCPConn<T>::EOwner::server, tcp_context, m_qMessagesIn);
                        if (_interface.OnClientConnectionRequest(new_conn)) {
                            m_deqConns.push_back(std::move(new_conn));
                            m_deqConns.back()->ConnectToClient(m_idCounter++);
                            _interface.OnClientConnected(m_deqConns.back());
                            INFO_MSG("[%d] Connection approved.", m_deqConns.back()->GetID());
                        } 
                        else
                            INFO_MSG("[SERVER] Connection denied!");
                    } else {
                        ERROR_MSG("[SERVER] New connection error: %s", ec.message().c_str());
                    }
                    WaitForClientConnection();
                });
    }

    template <typename T>
    void TCPServerImpl<T>::MessageClient(std::shared_ptr<ITCPConn<T>> client, const T& msg) {
        if (client && client->IsConnected()) {
            client->Send(msg);
        } else {
            _interface.OnClientDisconnected(client);
            client.reset();
            m_deqConns.erase(
                    std::remove(m_deqConns.begin(), m_deqConns.end(), client),
                    m_deqConns.end());

        }
    }

    template <typename T>
    void TCPServerImpl<T>::MessageAllClients(const T& msg, std::shared_ptr<ITCPConn<T>> pIgnoreClient) {
        DEBUG_MSG("[SERVER] Sending message to all clients...");
        bool bInvalidClientExists = false;
        for (auto& client: m_deqConns) {
            if (client && client->IsConnected()) {
                if (client != pIgnoreClient) {
                    client->Send(msg);
                    DEBUG_MSG("[SERVER] Message sent to [%d]", client->GetID());
                }
            } else {
                _interface.OnClientDisconnected(client);
                client.reset();
                bInvalidClientExists = true;
            }
        }
        if (bInvalidClientExists) {
            m_deqConns.erase(
                    std::remove(m_deqConns.begin(), m_deqConns.end(), nullptr),
                    m_deqConns.end());
        }
    }

    template <typename T>
    void TCPServerImpl<T>::Update(size_t nMaxMessages, bool bWait) {
        if (bWait) m_qMessagesIn.wait();
        size_t nMessageCount = 0;
        while (nMessageCount < nMaxMessages && !m_qMessagesIn.empty()) {
            auto msg = m_qMessagesIn.pop_front();
            _interface.OnMessage(msg.remote, msg.msg);
            nMessageCount++;
        }
    }

    template class ITCPServer<TCPMsg>;
    template class ITCPServer<TCPRawMsg>;
    
} // TCPConn