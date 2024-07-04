//
// Created by Bohan Leng on 7/2/2024.
//

#include "TCPConnImpl.h"
#include "LogMacros.h"

namespace TCPConn {

    /* ----- ITCPConn ----- */
    
    ITCPConn::ITCPConn(EOwner owner, struct TCPContext& context, TCPMsgQueue<TCPMsgOwned>& qIn)
    {
        pimpl = std::make_unique<TCPConnImpl>(*this, owner, context, qIn);
    }
    
    ITCPConn::~ITCPConn() = default;
    
    uint32_t ITCPConn::GetID() const {
        return pimpl->GetID();
    }
    
    void ITCPConn::ConnectToClient(uint32_t uid) {
        pimpl->ConnectToClient(uid);
    }
    
    void ITCPConn::ConnectToServer(const struct TCPEndpoint& endpoint) {
        pimpl->ConnectToServer(endpoint);
    }
    
    void ITCPConn::Disconnect() {
        pimpl->Disconnect();
    }
    
    bool ITCPConn::IsConnected() const {
        return pimpl->IsConnected();
    }
    
    void ITCPConn::Send(const TCPMsg& msg) {
        pimpl->Send(msg);
    }

    
    /* ----- TCPConnImpl ----- */
    
    TCPConnImpl::TCPConnImpl(ITCPConn& interface, ITCPConn::EOwner owner, ITCPConn::TCPContext& context, TCPMsgQueue<TCPMsgOwned>& qIn)
        : _interface(interface), m_context(context.context), m_socket(std::move(context.socket)), m_qMessagesIn(qIn)
    {
        m_nOwnerType = owner;
    }
    
    TCPConnImpl::~TCPConnImpl() = default;

    uint32_t TCPConnImpl::GetID() const {
        return id;
    }

    void TCPConnImpl::ConnectToClient(uint32_t uid)  {
        if (m_nOwnerType == ITCPConn::EOwner::server) {
            if (m_socket.is_open()) {
                id = uid;
                ReadHeader();
            }
        } else
            ERROR_MSG("Cannot connect client to client...");
    }

    void TCPConnImpl::ConnectToServer(const struct ITCPConn::TCPEndpoint& endpoint)  {
        if (m_nOwnerType == ITCPConn::EOwner::client) {
            async_connect(m_socket, endpoint.endpoint,
                          [this](std::error_code ec, ip::tcp::endpoint endpoint) {
                              if (!ec) {
                                  ReadHeader();
                              } else {
                                  INFO_MSG("[%d] Connect fail.", id);
                                  m_socket.close();
                              }
                          });
        } else
            ERROR_MSG("Cannot connect server to server...");
    }

    void TCPConnImpl::Disconnect()  {
        if (IsConnected()) 
            post(m_context, [this]() { m_socket.close(); });
    }

    bool TCPConnImpl::IsConnected() const  {
        return m_socket.is_open();
    }

    void TCPConnImpl::ReadHeader()  {
        async_read(m_socket, buffer(&m_msgTemporaryIn.header, sizeof(TCPMsgHeader)),
                   [this](std::error_code ec, std::size_t length) {
                       if (!ec) {
                           if (m_msgTemporaryIn.header.size > 0) {
                               m_msgTemporaryIn.body.resize(m_msgTemporaryIn.header.size);
                               ReadBody();
                           } else {
                               AddToIncomingMessageQueue();
                           }
                       } else {
                           INFO_MSG("[%d] Read header fail, closing.", id);
                           m_socket.close();
                       }
                   });
    }

    void TCPConnImpl::ReadBody() {
        async_read(m_socket, buffer(m_msgTemporaryIn.body.data(), m_msgTemporaryIn.body.size()),
                   [this](std::error_code ec, std::size_t length) {
                       if (!ec) {
                           AddToIncomingMessageQueue();
                       } else {
                           INFO_MSG("[%d] Read body fail, closing.", id);
                           m_socket.close();
                       }
                   });
    }

    void TCPConnImpl::WriteHeader() {
        async_write(m_socket, buffer(&m_qMessagesOut.front().header, sizeof(TCPMsgHeader)),
                    [this](std::error_code ec, std::size_t length) {
                        if (!ec) {
                            if (m_qMessagesOut.front().body.size() > 0) {
                                WriteBody();
                            } else {
                                m_qMessagesOut.pop_front();
                                if (!m_qMessagesOut.empty()) {
                                    WriteHeader();
                                }
                            }
                        } else {
                            INFO_MSG("[%d] Write header fail, closing.", id);
                            m_socket.close();
                        }
                    });
    }

    void TCPConnImpl::WriteBody() {
        async_write(m_socket, buffer(m_qMessagesOut.front().body.data(), m_qMessagesOut.front().body.size()),
                    [this](std::error_code ec, std::size_t length) {
                        if (!ec) {
                            m_qMessagesOut.pop_front();
                            if (!m_qMessagesOut.empty()) {
                                WriteHeader();
                            }
                        } else {
                            INFO_MSG("[%d] Write body fail, closing.", id);
                            m_socket.close();
                        }
                    });
    }

    void TCPConnImpl::AddToIncomingMessageQueue() {
        if (m_nOwnerType == ITCPConn::EOwner::server) {
            m_qMessagesIn.push_back({_interface.shared_from_this(), m_msgTemporaryIn});
        } else {
            m_qMessagesIn.push_back({nullptr, m_msgTemporaryIn});
        }
        ReadHeader();
    }

    void TCPConnImpl::Send(const TCPMsg& msg) {
        post(m_context,
             [this, msg]() {
                 bool bWritingMessage = !m_qMessagesOut.empty();
                 m_qMessagesOut.push_back(msg);
                 if (!bWritingMessage) {
                     WriteHeader();
                 }
             });
    }

} // TCPConn