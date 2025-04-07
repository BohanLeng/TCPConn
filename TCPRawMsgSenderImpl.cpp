//
// Created by Bohan Leng on 19.07.2024.
//

#include "TCPRawMsgSenderImpl.h"
#include "LogMacros.h"
#include "TCPRawMsgSender.h"
#include <csignal>

#define RAW_RECEIVE_BUFFER_SIZE 1024

namespace TCPConn {

    /* ----- ITCPRawMsgSender ----- */

    ITCPRawMsgSender::ITCPRawMsgSender() {
        pimpl = std::make_unique<TCPRawMsgSenderImpl>(*this, ERawMsgType::no_header);
    }

    ITCPRawMsgSender::ITCPRawMsgSender(int header_size, int length_offset, int length_size,
                                       bool length_include_header, bool endian_flip) {
        if (length_size != 1 && length_size != 2 && length_size != 4)
            throw std::invalid_argument("Length size must be 1, 2, or 4!");
        if (header_size <= 0 || length_offset <= 0 || length_offset + length_size > header_size)
            throw std::invalid_argument("Header properties are invalid!");
        pimpl = std::make_unique<TCPRawMsgSenderImpl>(*this, ERawMsgType::with_header, header_size, 
                                                      length_offset, length_size, length_include_header, endian_flip);
    }

    ITCPRawMsgSender::~ITCPRawMsgSender() = default;

    bool ITCPRawMsgSender::Connect(const std::string &host, uint16_t port) {
        return pimpl->Connect(host, port);
    }

    bool ITCPRawMsgSender::Connect(const char *host, uint16_t port) {
        return pimpl->Connect(std::string(host), port);
    }

    void ITCPRawMsgSender::Disconnect() {
        pimpl->Disconnect();
    }

    bool ITCPRawMsgSender::IsConnected() const {
        return pimpl->IsConnected();
    }

    void ITCPRawMsgSender::Send(const TCPRawMsg &msg) const {
        pimpl->Send(msg);
    }

    void ITCPRawMsgSender::Send(const uint8_t *raw_msg, uint32_t length) const {
        TCPRawMsg msg;
        msg.body.assign(raw_msg, raw_msg + length);
        pimpl->Send(msg);
    }

    void ITCPRawMsgSender::Update(size_t nMaxMessages, bool bWait) {
        pimpl->Update(nMaxMessages, bWait);
    }

    void ITCPRawMsgSender::Run() {
        pimpl->Run();
    }


    /* ----- TCPRawMsgSenderImpl ----- */

    std::atomic<bool> TCPRawMsgSenderImpl::m_bShuttingDown = false;
    
    TCPRawMsgSenderImpl::TCPRawMsgSenderImpl(ITCPRawMsgSender &interface, ITCPRawMsgSender::ERawMsgType msg_type,
                                             int header_size, int length_offset, int length_size, 
                                             bool length_include_header, bool endian_flip) 
        : _interface(interface), m_socket(m_context), m_eMsgType(msg_type),
          m_nHeaderSize(header_size), m_nLengthOffset(length_offset), m_nLengthSize(length_size),
          m_bEndianFlip(endian_flip), m_bLengthIncludeHeader(length_include_header) {}

    TCPRawMsgSenderImpl::~TCPRawMsgSenderImpl() {
        m_bIsDestroying = true;
        Disconnect();
    }

    bool TCPRawMsgSenderImpl::Connect(const std::string &host, uint16_t port) {
        if (IsConnected()) {
            ERROR_MSG("Already connected.");
            return false;
        }
        try {
            ip::tcp::resolver resolver(m_context);
            ip::tcp::resolver::results_type endpoint = resolver.resolve(host, std::to_string(port));
            async_connect(m_socket, endpoint, 
                    [this](std::error_code ec, ip::tcp::endpoint endpoint) {
                        if (!ec) {
                            INFO_MSG("Connected to: {}", endpoint.address().to_string());
                            auto async_call = std::async(std::launch::async, [this]() { _interface.OnConnected(); });
                            if (m_eMsgType == ITCPRawMsgSender::ERawMsgType::no_header) ReadRaw();
                            else ReadHeader();
                        } else {
                            INFO_MSG("Connect fail: {}", ec.message());
                            m_socket.close();
                        }
                    });
            m_thrContext = std::thread([this]() { m_context.run(); });
            return true;
        } catch (std::exception& e) {
            ERROR_MSG("Client Exception: {}", e.what());
            return false;
        }
    }

    void TCPRawMsgSenderImpl::Disconnect() {
        if (IsConnected())
            post(m_context, [this]() { m_socket.close(); });
        m_qMessagesIn.exit_wait();
        m_context.stop();
        if (m_thrContext.joinable()) {
            m_thrContext.join();
        }
        if(!m_bIsDestroying) {
            INFO_MSG("Disconnected.");
            _interface.OnDisconnected();
        }
    }

    bool TCPRawMsgSenderImpl::IsConnected() const {
        return m_socket.is_open();
    }

    void TCPRawMsgSenderImpl::Send(const TCPRawMsg &msg) {
        post(m_context,
             [this, msg]() {
                 bool bWritingMessage = !m_qMessagesOut.empty();
                 m_qMessagesOut.push_back(msg);
                 if (!bWritingMessage) {
                     WriteRaw();
                 }
             });
    }

    void TCPRawMsgSenderImpl::Update(size_t nMaxMessages, bool bWait) {
        if (bWait) m_qMessagesIn.wait();
        size_t nMessageCount = 0;
        while (nMessageCount < nMaxMessages && !m_qMessagesIn.empty()) {
            auto msg = m_qMessagesIn.pop_front();
            _interface.OnMessage(msg);
            nMessageCount++;
        }
    }

    void TCPRawMsgSenderImpl::Run() {
        INFO_MSG("Client consuming messages...");
        std::signal(SIGINT, [](int) {
            if (m_bShuttingDown) abort();
            m_bShuttingDown = true;
        });

        auto thr_shutdown_detect = std::thread([this]() {
            while (!m_bShuttingDown) {
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
            INFO_MSG("\nSignal received, exiting running...");
            m_qMessagesIn.exit_wait();
        });

        while (!m_bShuttingDown) Update();
        thr_shutdown_detect.join();
        INFO_MSG("Running exited.");
    }

    void TCPRawMsgSenderImpl::ReadRaw() {
        m_msgTemporaryIn.body.resize(RAW_RECEIVE_BUFFER_SIZE);
        m_socket.async_receive(buffer(m_msgTemporaryIn.body.data(), RAW_RECEIVE_BUFFER_SIZE),
                [this](std::error_code ec, std::size_t length) {
                    if (!ec) {
                        m_msgTemporaryIn.body.resize(length);
                        AddToIncomingMessageQueue();
                    } else {
                        INFO_MSG("Receive raw message fail, closing connection.");
                        m_socket.close();
                    }
                });
    }

    void TCPRawMsgSenderImpl::ReadHeader() {
        m_msgTemporaryIn.body.resize(m_nHeaderSize);
        async_read(m_socket, buffer(m_msgTemporaryIn.body.data(), m_nHeaderSize),
                   [this](std::error_code ec, std::size_t length) {
                       if (!ec) {
                           auto msg_full_length = CalculateMsgFullLength(m_msgTemporaryIn);
                           if (msg_full_length > m_nHeaderSize) {
                               m_msgTemporaryIn.body.resize(msg_full_length);
                               ReadBody();
                           } else {
                               AddToIncomingMessageQueue();
                           }
                       } else {
                               INFO_MSG("Read header fail, closing connection.");
                           m_socket.close();
                       }
                   });
    }

    void TCPRawMsgSenderImpl::ReadBody() {
        async_read(m_socket, buffer(m_msgTemporaryIn.body.data() + m_nHeaderSize, CalculateMsgBodyLength(m_msgTemporaryIn)),
                   [this](std::error_code ec, std::size_t length) {
                       if (!ec) {
                           AddToIncomingMessageQueue();
                       } else {
                               INFO_MSG("Read body fail, closing connection.");
                           m_socket.close();
                       }
                   });
    }

    void TCPRawMsgSenderImpl::WriteRaw() {
        async_write(m_socket, buffer(m_qMessagesOut.front().body.data(), m_qMessagesOut.front().full_size()),
                    [this](std::error_code ec, std::size_t length) {
                        if (!ec) {
                            m_qMessagesOut.pop_front();
                            if (!m_qMessagesOut.empty()) {
                                WriteRaw();
                            }
                        } else {
                            INFO_MSG("Write raw message fail, closing connection.");
                            m_socket.close();
                        }
                    });
    }

    void TCPRawMsgSenderImpl::AddToIncomingMessageQueue() {
        m_qMessagesIn.push_back(m_msgTemporaryIn);
        if (m_eMsgType == ITCPRawMsgSender::ERawMsgType::no_header) ReadRaw();
        else ReadHeader();
    }

    int TCPRawMsgSenderImpl::CalculateMsgBodyLength(const TCPRawMsg &header) const {
        auto data = header.body.data();
        if (m_nLengthSize == 1)
            return data[m_nLengthOffset];
        else if (m_nLengthSize == 2) {
            if (m_bEndianFlip) {
                return (data[m_nLengthOffset] << 8) | data[m_nLengthOffset + 1];
            } else {
                return (data[m_nLengthOffset]) | (data[m_nLengthOffset + 1] << 8);
            }
        } else if (m_nLengthSize == 4) {
            if (m_bEndianFlip) {
                return (data[m_nLengthOffset] << 24) | (data[m_nLengthOffset + 1] << 16) |
                       (data[m_nLengthOffset + 2] << 8) | data[m_nLengthOffset + 3];
            } else {
                return (data[m_nLengthOffset]) | (data[m_nLengthOffset + 1] << 8) |
                       (data[m_nLengthOffset + 2] << 16) | (data[m_nLengthOffset + 3] << 24);
            }
        } else {
            ERROR_MSG("Invalid length size: {}", m_nLengthSize);
            return 0;
        }
    }

    int TCPRawMsgSenderImpl::CalculateMsgFullLength(const TCPRawMsg &header) const {
        return CalculateMsgBodyLength(header) + m_nHeaderSize * int(!m_bLengthIncludeHeader);
    }

} // TCPConn