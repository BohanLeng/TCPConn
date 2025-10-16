//
// Created by Bohan Leng on 19.07.2024.
//

#ifndef TCPCONN_TCPRAWMSGSENDER_H
#define TCPCONN_TCPRAWMSGSENDER_H

#include "TCPMsg.h"
#include "TCPMsgQueue.h"

namespace TCPConn {
    
    class TCPRawMsgSenderImpl;
    
    /// \brief This is a class used for interacting with third-parties
    /// using custom message formats, send as `TCPRawMsg`.
    /// For interacting with known servers derived from `ITCPServer``, use `ITCPClient`
    class TCPCONN_API ITCPRawMsgSender {
    public:
        
        enum class ERawMsgType {
            with_header,
            no_header
        };

        /// \brief Construct a TCP header-less message sender.
        ITCPRawMsgSender();
        
        /// \brief Construct a TCP header message sender.
        /// \param header_size size of the header
        /// \param length_offset offset (in byte) of the length field in the header
        /// \param length_size size (in byte) of the length field in the header
        /// \param length_include_header whether the length field includes the header size
        /// \param endian_flip whether to flip the endian of the length field 
        ITCPRawMsgSender(int header_size, int length_offset, int length_size,
                         bool length_include_header, bool endian_flip);
        
        virtual ~ITCPRawMsgSender();

        
        /// \brief Connect to a raw message recipient
        /// \param host server address or domain name
        /// \param port server port
        bool Connect(const std::string& host, uint16_t port);

        /// \brief Connect to a raw message recipient
        /// \param host server address or domain name
        /// \param port server port
        bool Connect(const char* host, uint16_t port);

        /// \brief Disconnect from the server, will be called automatically on destruction
        void Disconnect();

        /// \brief Check if the sender is connected.
        /// \return true if the socket is open
        [[nodiscard]] bool IsConnected() const;

        
        /// \brief Send a message to the server.
        /// \param msg message to send
        void Send(const TCPRawMsg& msg) const;

        /// \brief Send a raw msg to the server.
        /// \param msg message to send
        void Send(const uint8_t* raw_msg, uint32_t length) const;

        
        /// \brief Actively consume messages in the message queue.
        /// \param nMaxMessages maximum number of messages to consume, default is -1, consume all
        /// \param bWait whether to block to wait for incoming messages, must be true if used in a loop
        void Update(size_t nMaxMessages = -1, bool bWait = true);

        /// \brief Start continuous update messages.
        /// Will block the current thread. Pending signals to terminate.
        void Run();

        
        /// \brief On connected to server.
        virtual void OnConnected() {}

        /// \brief On disconnection from server.
        virtual void OnDisconnected() {}

        /// \brief On message received, must be overridden.
        /// \param msg received message
        virtual void OnMessage(TCPRawMsg& msg) = 0;

    private:
        std::unique_ptr<TCPRawMsgSenderImpl> pimpl;
    };

} // TCPConn


#endif //TCPCONN_TCPRAWMSGSENDER_H
