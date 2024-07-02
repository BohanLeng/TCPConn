//
// Created by Bohan Leng on 7/3/2024.
//

#include "TCPServerImpl.h"
#include "TCPConnImpl.h"
#include "TCPServer.h"


namespace TCPConn {

    /* ----- ITCPServer ----- */

    ITCPServer::ITCPServer(uint16_t port) : _port(port)
    {
        pimpl = std::make_unique<TCPServerImpl>(*this, _port);
    }
    
    ITCPServer::~ITCPServer() {
        pimpl->Stop();
    }
    
    bool ITCPServer::Start() {
        return pimpl->Start();
    }
    
    void ITCPServer::Stop() {
        pimpl->Stop();
    }
    
    void ITCPServer::WaitForClientConnection() {
        pimpl->WaitForClientConnection();
    }

    void ITCPServer::MessageClient(std::shared_ptr<ITCPConn> client, const TCPMsg &msg) {
        pimpl->MessageClient(client, msg);
    }

    void ITCPServer::MessageAllClients(const TCPMsg &msg, std::shared_ptr<ITCPConn> pIgnoreClient) {
        pimpl->MessageAllClients(msg, pIgnoreClient);
    }

    void ITCPServer::Update(size_t nMaxMessages) {
        pimpl->Update(nMaxMessages);
    }

    /* ----- TCPServerImpl ----- */

    TCPServerImpl::TCPServerImpl(ITCPServer &interface, uint16_t port)
            : _interface(interface), m_acceptor(m_context, ip::tcp::endpoint(ip::tcp::v4(), port))
    {
        
    }
    
    TCPServerImpl::~TCPServerImpl() {
        Stop();
    }

    bool TCPServerImpl::Start() {
        try {
            WaitForClientConnection();

            m_thrContext = std::thread([this]() { m_context.run(); });
        }
        catch (std::exception &e) {
            std::cerr << "[SERVER] Exception: " << e.what() << std::endl;
            return false;
        }

        std::cout << "[SERVER] Started!" << std::endl;
        return true;
    }

    bool TCPServerImpl::Stop() {
        m_context.stop();
        if (m_thrContext.joinable()) {
            m_thrContext.join();
        }
        std::cout << "[SERVER] Stopped!" << std::endl;
        return true;
    }

    void TCPServerImpl::WaitForClientConnection() {
        m_acceptor.async_accept(
                [this](std::error_code ec, ip::tcp::socket socket) {
                    if (!ec) {
                        std::cout << "[SERVER] New Connection: " << socket.remote_endpoint() << std::endl;
                        struct ITCPConn::TCPContext tcp_context{m_context, std::move(socket)};
                        auto new_conn = std::make_shared<ITCPConn>(ITCPConn::EOwner::server, tcp_context, m_qMessagesIn);
                        if (_interface.OnClientConnected(new_conn)) {
                            m_deqConns.push_back(std::move(new_conn));
                            m_deqConns.back()->ConnectToClient(m_idCounter++);
                            std::cout << '[' << m_deqConns.back()->GetID() << "] Connection Approved" << std::endl;
                        } 
                        else
                            std::cout << "[SERVER] Interface Expired" << std::endl;
                    } else {
                        std::cout << "[SERVER] New Connection Error: " << ec.message() << std::endl;
                    }
                    WaitForClientConnection();

                });
    }

    void TCPServerImpl::MessageClient(std::shared_ptr<ITCPConn> client, const TCPMsg &msg) {
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

    void TCPServerImpl::MessageAllClients(const TCPMsg &msg, std::shared_ptr<ITCPConn> pIgnoreClient) {
        bool bInvalidClientExists = false;
        for (auto &client: m_deqConns) {
            if (client && client->IsConnected()) {
                if (client != pIgnoreClient) {
                    client->Send(msg);
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

    void TCPServerImpl::Update(size_t nMaxMessages) {
        size_t nMessageCount = 0;
        while (nMessageCount < nMaxMessages && !m_qMessagesIn.empty()) {
            auto msg = m_qMessagesIn.pop_front();
            _interface.OnMessage(msg.remote, msg.msg);
            nMessageCount++;
        }
    }



} // TCPConn