//
// Created by Bohan Leng on 7/2/2024.
//

#include "TCPConnImpl.h"
#include "LogMacros.h"
#include "TCPConn.h"

#define RAW_RECEIVE_BUFFER_SIZE 1024

namespace TCPConn {

    /* ----- ITCPConn ----- */
    
    template <typename T>
    ITCPConn<T>::ITCPConn(ITCPConn<T>::EOwner owner, struct ITCPConn::TCPContext& context, TCPMsgQueue<TCPMsgOwned<T>>& qIn)
    {
        pimpl = std::make_unique<TCPConnImpl<T>>(*this, owner, context, qIn);
    }
    
    template <typename T>
    ITCPConn<T>::~ITCPConn() = default;

    template <typename T>
    uint32_t ITCPConn<T>::GetID() const {
        return pimpl->GetID();
    }

    template <typename T>
    void ITCPConn<T>::ConnectToClient(uint32_t uid) {
        pimpl->ConnectToClient(uid);
    }

    template<typename T>
    void ITCPConn<T>::ConnectToServer(const ITCPConn::TCPEndpoint &endpoint,
                                      const std::function<void()> &OnConnectedCallback) {
        pimpl->ConnectToServer(endpoint, OnConnectedCallback);
    }
    
    template <typename T>
    void ITCPConn<T>::Disconnect() {
        pimpl->Disconnect();
    }

    template <typename T>
    bool ITCPConn<T>::IsConnected() const {
        return pimpl->IsConnected();
    }

    template <typename T>
    void ITCPConn<T>::Send(const T& msg) {
        pimpl->Send(msg);
    }


    /* ----- TCPConnImpl ----- */
    
    template <typename T>
    TCPConnImpl<T>::TCPConnImpl(ITCPConn<T>& interface, ITCPConn<T>::EOwner owner, struct ITCPConn<T>::TCPContext& context, TCPMsgQueue<TCPMsgOwned<T>>& qIn)
        : _interface(interface), m_context(context.context), m_socket(std::move(context.socket)), m_qMessagesIn(qIn)
    {
        m_nOwnerType = owner;
    }
    
    template <typename T>
    TCPConnImpl<T>::~TCPConnImpl() = default;

    template <typename T>
    uint32_t TCPConnImpl<T>::GetID() const {
        return id;
    }

    template <typename T>
    void TCPConnImpl<T>::ConnectToClient(uint32_t uid)  {
        if (m_nOwnerType == ITCPConn<T>::EOwner::server) {
            if (m_socket.is_open()) {
                id = uid;
                if constexpr (std::is_same<T, TCPMsg>::value) ReadHeader();
                else if constexpr (std::is_same<T, TCPRawMsg>::value) ReadRaw();
            }
        } else
            ERROR_MSG("Cannot connect client to client!");
    }

    template <typename T>
    void TCPConnImpl<T>::ConnectToServer(const struct ITCPConn<T>::TCPEndpoint &endpoint, const std::function<void()>& OnConnectedCallback) {
        if (m_nOwnerType == ITCPConn<T>::EOwner::client) {
            async_connect(m_socket, endpoint.endpoint,
                          [this, OnConnectedCallback](std::error_code ec, ip::tcp::endpoint endpoint) {
                              if (!ec) {
                                  INFO_MSG("[%d] Connected to server: %s", id, endpoint.address().to_string().c_str());
                                  OnConnectedCallback();
                                  if constexpr (std::is_same<T, TCPMsg>::value) ReadHeader();
                                  else if constexpr (std::is_same<T, TCPRawMsg>::value) ReadRaw();
                              } else {
                                  INFO_MSG("[%d] Connect fail: %s", id, ec.message().c_str());
                                  m_socket.close();
                              }
                          });
        } else
            ERROR_MSG("Cannot connect server to server!");
    }

    template <typename T>
    void TCPConnImpl<T>::Disconnect()  {
        if (IsConnected()) 
            post(m_context, [this]() { m_socket.close(); });
    }

    template <typename T>
    bool TCPConnImpl<T>::IsConnected() const  {
        return m_socket.is_open();
    }

    template <typename T>
    void TCPConnImpl<T>::Send(const T& msg) {
        if constexpr (std::is_same<T, TCPMsg>::value)
            post(m_context,
                 [this, msg]() {
                     bool bWritingMessage = !m_qMessagesOut.empty();
                     m_qMessagesOut.push_back(msg);
                     if (!bWritingMessage) {
                         WriteHeader();
                     }
                 });
        else if constexpr (std::is_same<T, TCPRawMsg>::value) 
            post(m_context,
                 [this, msg]() {
                     bool bWritingMessage = !m_qMessagesOut.empty();
                     m_qMessagesOut.push_back(msg);
                     if (!bWritingMessage) {
                         WriteRaw();
                     }
                 });
    }

    template <typename T>
    void TCPConnImpl<T>::ReadHeader()  {
        if constexpr (std::is_same<T, TCPMsg>::value)
            async_read(m_socket, buffer(&m_msgTemporaryIn.header, sizeof(TCPMsgHeader)),
                       [this](std::error_code ec, std::size_t length) {
                           if (!ec) {
                               if (m_msgTemporaryIn.header.size > 0) {
                                   m_msgTemporaryIn.body.resize(m_msgTemporaryIn.header.size - sizeof(TCPMsgHeader));
                                   ReadBody();
                               } else {
                                   AddToIncomingMessageQueue();
                               }
                           } else {
                               INFO_MSG("[%d] Read header fail, closing.", id);
                               m_socket.close();
                           }
                       });
        else 
            static_assert(std::is_same<T, TCPMsg>::value, "ReadHeader() is only for TCPMsg");
    }

    template <typename T>
    void TCPConnImpl<T>::ReadBody() {
        if constexpr (std::is_same<T, TCPMsg>::value) 
            async_read(m_socket, buffer(m_msgTemporaryIn.body.data(), m_msgTemporaryIn.body.size()),
                       [this](std::error_code ec, std::size_t length) {
                           if (!ec) {
                               AddToIncomingMessageQueue();
                           } else {
                               INFO_MSG("[%d] Read body fail, closing.", id);
                               m_socket.close();
                           }
                       });
        else
            static_assert(std::is_same<T, TCPMsg>::value, "ReadBody() is only for TCPMsg");
    }

    template <typename T>
    void TCPConnImpl<T>::WriteHeader() {
        if constexpr (std::is_same<T, TCPMsg>::value) 
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
        else
            static_assert(std::is_same<T, TCPMsg>::value, "ConnectToClient() is only for TCPMsg");
    }

    template <typename T>
    void TCPConnImpl<T>::WriteBody() {
        if constexpr (std::is_same<T, TCPMsg>::value)
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
        else 
            static_assert(std::is_same<T, TCPMsg>::value, "WriteBody() is only for TCPMsg");
    }

    template <typename T>
    void TCPConnImpl<T>::AddToIncomingMessageQueue() {
        if (m_nOwnerType == ITCPConn<T>::EOwner::server) {
            m_qMessagesIn.push_back({_interface.shared_from_this(), m_msgTemporaryIn});
        } else {
            m_qMessagesIn.push_back({nullptr, m_msgTemporaryIn});
        }
        if constexpr (std::is_same<T, TCPMsg>::value) ReadHeader();
        else if constexpr (std::is_same<T, TCPRawMsg>::value) ReadRaw();
    }

    template<typename T>
    void TCPConnImpl<T>::ReadRaw() {
        if constexpr (std::is_same<T, TCPRawMsg>::value) {
            m_msgTemporaryIn.body.resize(RAW_RECEIVE_BUFFER_SIZE);
            m_socket.async_receive(buffer(m_msgTemporaryIn.body.data(), RAW_RECEIVE_BUFFER_SIZE),
                                   [this](std::error_code ec, std::size_t length) {
                                       if (!ec) {
                                           m_msgTemporaryIn.body.resize(length);
                                           AddToIncomingMessageQueue();
                                       } else {
                                           INFO_MSG("[%d] Receive raw message fail, closing.", id);
                                           m_socket.close();
                                       }
                                   });
        }
        else
            static_assert(std::is_same<T, TCPRawMsg>::value, "ReadRaw() is only for TCPRawMsg");
    }

    template<typename T>
    void TCPConnImpl<T>::WriteRaw() {
        if constexpr (std::is_same<T, TCPRawMsg>::value)
            async_write(m_socket, buffer(m_qMessagesOut.front().body.data(), m_qMessagesOut.front().full_size()),
                    [this](std::error_code ec, std::size_t length) {
                        if (!ec) {
                            m_qMessagesOut.pop_front();
                            if (!m_qMessagesOut.empty()) {
                                WriteRaw();
                            }
                        } else {
                            INFO_MSG("[%d] Write raw message fail, closing.", id);
                            m_socket.close();
                        }
                    });
        else
            static_assert(std::is_same<T, TCPRawMsg>::value, "WriteRaw() is only for TCPRawMsg");
    }

    template class ITCPConn<TCPMsg>;
    template class ITCPConn<TCPRawMsg>;

} // TCPConn