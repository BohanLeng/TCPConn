//
// Created by Bohan Leng on 19.07.2024.
//

#ifndef ZMONITOR_TCPRAWMSGSENDERIMPL_H
#define ZMONITOR_TCPRAWMSGSENDERIMPL_H

#include "TCPRawMsgSender.h"
#include <boost/asio.hpp>
#include <thread>

using namespace boost::asio;

namespace TCPConn {
    
    class TCPRawMsgSenderImpl {
    public:
        explicit TCPRawMsgSenderImpl(ITCPRawMsgSender& interface, ITCPRawMsgSender::ERawMsgType msg_type,
                                     int header_size = 0, int length_offset = 0, int length_size = 0, 
                                     bool length_include_header = false, bool endian_flip = false);
        virtual ~TCPRawMsgSenderImpl();
        
        bool Connect(const std::string& host, uint16_t port);
        void Disconnect();
        [[nodiscard]] bool IsConnected() const;
        
        void Send(const TCPRawMsg& msg);

        void Update(size_t nMaxMessages = -1, bool bWait = true);
        void Run();
        
    protected:

        void ReadRaw();
        void ReadHeader();
        void ReadBody();
        void WriteRaw();
        void AddToIncomingMessageQueue();

        io_context m_context;
        ip::tcp::socket m_socket;
        std::thread m_thrContext;
        TCPMsgQueue<TCPRawMsg> m_qMessagesOut{};
        TCPMsgQueue<TCPRawMsg> m_qMessagesIn{};
        TCPRawMsg m_msgTemporaryIn;
        ITCPRawMsgSender::ERawMsgType m_eMsgType;
        
        int m_nHeaderSize = 0;
        int m_nLengthOffset = 0;
        int m_nLengthSize = 0;
        bool m_bLengthIncludeHeader = false;
        bool m_bEndianFlip = false;
        bool m_bIsDestroying{};
        static std::atomic<bool> m_bShuttingDown;

        [[nodiscard]] int CalculateMsgBodyLength(const TCPRawMsg& header) const;
        [[nodiscard]] int CalculateMsgFullLength(const TCPRawMsg& header) const;
        
    private:
        ITCPRawMsgSender& _interface;
    };
    

} // TCPConn

#endif //ZMONITOR_TCPRAWMSGSENDERIMPL_H
