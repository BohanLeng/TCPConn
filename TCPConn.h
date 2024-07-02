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

enum class MsgTypes;

namespace TCPConn {

    class TCPConnImpl;

    class TCPCONN_API ITCPConn : public std::enable_shared_from_this<ITCPConn> {
    public:
        enum class EOwner {
            server,
            client
        };

        struct TCPContext;
        struct TCPEndpoint;
        
        ITCPConn(EOwner owner, struct TCPContext &context, TCPMsgQueue<TCPMsgOwned> &qIn);
        virtual ~ITCPConn();

        [[nodiscard]] uint32_t GetID() const;

        void ConnectToClient(uint32_t uid = 0);
        void ConnectToServer(const struct TCPEndpoint &endpoint);
        void Disconnect();
        [[nodiscard]] bool IsConnected();

        bool Send(const TCPMsg &msg);

        void ReadHeader();
        void ReadBody();
        void WriteHeader();
        void WriteBody();

        void AddToIncomingMessageQueue();
        
    private:
        std::unique_ptr<TCPConnImpl> pimpl;
    };

} // TCPConn

#endif //ZMONITOR_TCPCONN_H
