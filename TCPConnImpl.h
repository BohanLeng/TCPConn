//
// Created by Bohan Leng on 7/1/2024.
//

#ifndef ZMONITOR_TCPCONNIMPL_H
#define ZMONITOR_TCPCONNIMPL_H

#include "TCPConn.h"
#include <boost/asio.hpp>

using namespace boost::asio;

namespace TCPConn {

    template <typename T>
    struct ITCPConn<T>::TCPContext {
        io_context &context;
        ip::tcp::socket socket;
    };

    template <typename T>
    struct ITCPConn<T>::TCPEndpoint {
        ip::tcp::resolver::results_type &endpoint;
    };

    template <typename T>
    class TCPConnImpl {
    public:
        TCPConnImpl(ITCPConn<T>& interface, ITCPConn<T>::EOwner owner, 
                    struct ITCPConn<T>::TCPContext& context, TCPMsgQueue<TCPMsgOwned<T>>& qIn);
        virtual ~TCPConnImpl();

        [[nodiscard]] uint32_t GetID() const;

        void ConnectToClient(uint32_t uid = 0);
        void ConnectToServer(const struct ITCPConn<T>::TCPEndpoint &endpoint, const std::function<void()>& OnConnectedCallback);
        void Disconnect();
        [[nodiscard]] bool IsConnected() const;

        // Server validation procedure 
        void WriteValidation();
        void ReadValidation();
        void NotifyValidation();
        // Client validation procedure
        void ReadValidation(const std::function<void()>& OnConnectedCallback);
        void WriteValidation(const std::function<void()>& OnConnectedCallback);
        void WaitForValidation(const std::function<void()>& OnConnectedCallback);
        static uint64_t CalculateValidation(uint64_t nInput);
        
        void Send(const T& msg);

    protected:

        void ReadHeader();
        void ReadBody();
        void WriteHeader();
        void WriteBody();
        void AddToIncomingMessageQueue();
        
        void ReadRaw();
        void WriteRaw();
        
        ip::tcp::socket m_socket;
        io_context& m_context;
        
        TCPMsgQueue<T> m_qMessagesOut{};
        TCPMsgQueue<TCPMsgOwned<T>>& m_qMessagesIn;
        T m_msgTemporaryIn;
        
        uint64_t m_nValidationOut = 0;
        uint64_t m_nValidationIn = 0;
        uint64_t m_nValidationCheck = 0;
        
        ITCPConn<T>::EOwner m_eOwnerType;
        uint32_t id = -1;
        
    private:
        ITCPConn<T>& _interface;
    };

} // TCPConn

#endif //ZMONITOR_TCPCONNIMPL_H
