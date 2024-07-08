//
// Created by Bohan Leng on 7/2/2024.
//

#ifndef ZMONITOR_TCPCONN_H
#define ZMONITOR_TCPCONN_H

#if defined _WIN32
#   if defined TCPCONN_STATIC
#       define TCPCONN_API
#   elif defined TCPCONN_DLL
#       define TCPCONN_API __declspec(dllexport)
#   else
#       define TCPCONN_API __declspec(dllimport)
#   endif
#elif defined __APPLE__ || defined __linux__
#   if defined TCPCONN_STATIC
#       define TCPCONN_API
#   elif defined TCPCONN_DLL
#       define TCPCONN_API __attribute__((visibility("default")))
#   else
#       define TCPCONN_API
#   endif
#else
#   define TCPCONN_API
#endif

#include "TCPMsg.h"
#include "TCPMsgQueue.h"
#include <functional>

enum class MsgTypes;

namespace TCPConn {

    class TCPConnImpl;

    class TCPCONN_API ITCPConn : public std::enable_shared_from_this<ITCPConn> {
    public:
        
        /// \brief Creator of the connection 
        enum class EOwner {
            server,
            client
        };

        /// \brief Wrapper for asio tcp context reference and socket
        struct TCPContext;
        
        /// \brief Wrapper for asio tcp endpoint
        struct TCPEndpoint;
        
        
        /// \brief Constructor
        /// \param owner owner of the connection, either server or client
        /// \param context asio tcp context reference and socket
        /// \param qIn reference to the incoming message queue of owner
        ITCPConn(EOwner owner, struct TCPContext& context, TCPMsgQueue<TCPMsgOwned>& qIn);
        virtual ~ITCPConn();

        /// \brief Get the connection ID managed by server
        /// \return connection ID
        [[nodiscard]] uint32_t GetID() const;

        
        /// \brief For server to call, connect to a client and start receiving messages
        /// \param uid client ID to assign to this connection
        void ConnectToClient(uint32_t uid = 0);
        
        /// \brief For client to call, connect to a server
        /// \param endpoint server endpoint to connect
        void ConnectToServer(const struct TCPEndpoint &endpoint, std::function<void()> OnConnectedCallback);
        
        /// \brief Disconnect the connection
        void Disconnect();
        
        /// \brief Check if the connection is open
        /// \return true if the connection is open
        [[nodiscard]] bool IsConnected() const;

        
        /// \brief Send a message to the other end
        /// \param msg message to send
        void Send(const TCPMsg& msg);
        
    private:
        std::unique_ptr<TCPConnImpl> pimpl;
    };

} // TCPConn

#endif //ZMONITOR_TCPCONN_H
