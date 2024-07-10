//
// Created by Bohan Leng on 7/1/2024.
//

#ifndef ZMONITOR_TCPMSG_H
#define ZMONITOR_TCPMSG_H

#include <cstdint>
#include <vector>
#include <iostream>

namespace TCPConn {
    
    struct TCPMsgHeader {
        uint32_t type;
        uint32_t size{};
    };

    struct TCPMsg {
        TCPMsgHeader header{};
        std::vector<uint8_t> body;

        [[nodiscard]] size_t full_size() const {
            return sizeof(TCPMsgHeader) + body.size();
        }
        
        friend std::ostream& operator << (std::ostream& os, const TCPMsg& msg) {
            os << "Message type: " << msg.header.type << ", size: " << msg.header.size;
            return os;
        }

        template <typename T>
        friend TCPMsg& operator << (TCPMsg& msg, const T& data) {
            static_assert(std::is_standard_layout<T>::value, 
                    "Data is too complex to be pushed into vector");
            size_t size = msg.body.size();
            msg.body.resize(size + sizeof(T));
            std::memcpy(msg.body.data() + size, &data, sizeof(T));
            msg.header.size = msg.full_size();
            return msg;
        }

        template <typename T>
        friend TCPMsg& operator >> (TCPMsg& msg, T& data) {
            static_assert(std::is_standard_layout<T>::value, 
                    "Data is too complex to be pushed into vector");
            size_t size = msg.body.size() - sizeof(T);
            std::memcpy(&data, msg.body.data() + size, sizeof(T));
            msg.body.resize(size);
            msg.header.size = msg.full_size();
            return msg;
        }
    };

    struct TCPRawMsg {
        std::vector<uint8_t> body;
            
        [[nodiscard]] size_t full_size() const {
            return body.size();
        }
        
        friend std::ostream& operator << (std::ostream& os, const TCPRawMsg& msg) {
            os << "Raw data (size " << std::dec << msg.full_size() << "): \n";
            // print hexadecimal value of the raw byte message:
            for (const auto& byte : msg.body) {
                os << std::hex << static_cast<int>(byte) << ' ';
            }
            os << '\n';
            return os;
        }

        template <typename T>
        friend TCPRawMsg& operator << (TCPRawMsg& msg, const T& data) {
            static_assert(std::is_standard_layout<T>::value, 
                    "Data is too complex to be pushed into vector");
            size_t size = msg.body.size();
            msg.body.resize(size + sizeof(T));
            std::memcpy(msg.body.data() + size, &data, sizeof(T));
            return msg;
        }

        template <typename T>
        friend TCPRawMsg& operator >> (TCPRawMsg& msg, T& data) {
            static_assert(std::is_standard_layout<T>::value, 
                    "Data is too complex to be pushed into vector");
            size_t size = msg.body.size() - sizeof(T);
            std::memcpy(&data, msg.body.data() + size, sizeof(T));
            msg.body.resize(size);
            return msg;
        }
    };

    template <typename T>
    class ITCPConn;

    template <typename T>
    struct TCPMsgOwned {
        std::shared_ptr<ITCPConn<T>> remote = nullptr;
        T msg;

        [[nodiscard]] uint32_t full_size() const {
            return msg.full_size();
        }
    };

} // TCPCon

#endif //ZMONITOR_TCPMSG_H
