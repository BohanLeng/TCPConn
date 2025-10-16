//
// Created by Bohan Leng on 7/1/2024.
//

#ifndef TCPCONN_TCPMSG_H
#define TCPCONN_TCPMSG_H

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

#include <cstdint>
#include <vector>
#include <iostream>
#include <cstring>
#include <sstream>
#include <memory>
#include <utility>
#include <iomanip>

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
        
        // Input generic data
        template <typename T>
        friend TCPMsg& operator << (TCPMsg& msg, const T& data) {
            static_assert(std::is_standard_layout<T>::value, 
                    "Data layout is not standard. Input structure content separately.");
            size_t size = msg.body.size();
            msg.body.resize(size + sizeof(T));
            std::memcpy(msg.body.data() + size, &data, sizeof(T));
            msg.header.size = msg.full_size();
            return msg;
        }

        // Input vector
        template <typename T>
        friend TCPMsg& operator << (TCPMsg& msg, const std::vector<T>& data) {
            static_assert(std::is_standard_layout<T>::value,
                          "Vector data layout is not standard.");
            size_t size = msg.body.size();
            msg.body.resize(size + sizeof(T) * data.size());
            std::memcpy(msg.body.data() + size, data.data(), sizeof(T) * data.size());
            msg.header.size = msg.full_size();
            return msg;
        }
        
        // Output generic data
        template <typename T>
        friend TCPMsg& operator >> (TCPMsg& msg, T& data) {
            static_assert(std::is_standard_layout<T>::value, 
                    "Data layout is not standard. Output structure content separately.");
            size_t size = msg.body.size() - sizeof(T);
            std::memcpy(&data, msg.body.data() + size, sizeof(T));
            msg.body.resize(size);
            msg.header.size = msg.full_size();
            return msg;
        }
    
        // Output string
        friend TCPMsg& operator >> (TCPMsg& msg, std::string& data) {
            size_t size = msg.body.size() - data.length();
            std::memcpy(data.data(), msg.body.data() + size, data.length());
            msg.body.resize(size);
            msg.header.size = msg.full_size();
            return msg;
        }
        
        // Output vector (assert msg contains solely this vector)
        template <typename T>
        friend TCPMsg& operator >> (TCPMsg& msg, std::vector<T>& data) {
            static_assert(std::is_standard_layout<T>::value,
                          "Vector data layout is not standard.");
            data.resize(msg.body.size() / sizeof(T));
            std::memcpy(data.data(), msg.body.data(), msg.body.size());
            msg.body.resize(0);
            msg.header.size = msg.full_size();
            return msg;
        }

        std::string formatted() const {
            std::ostringstream oss;
            oss << "Message type: " << header.type << ", size: " << header.size << ", raw data: " << std::endl;
            oss << std::uppercase << std::hex;
            for (const auto &byte: body) {
                oss << std::setw(2) << std::setfill('0') << static_cast<int>(byte) << ' ';
            }
            oss << std::nouppercase << std::dec << '\n';
            return oss.str();
        }

        friend std::ostream& operator << (std::ostream& os, const TCPMsg& msg) {
            os << msg.formatted();
            return os;
        }
    };

    struct TCPRawMsg {
        std::vector<uint8_t> body;
            
        [[nodiscard]] size_t full_size() const {
            return body.size();
        }

        template <typename T>
        friend TCPRawMsg& operator << (TCPRawMsg& msg, const T& data) {
            static_assert(std::is_standard_layout<T>::value, 
                    "Data layout is not standard. Input structure content separately.");
            size_t size = msg.body.size();
            msg.body.resize(size + sizeof(T));
            std::memcpy(msg.body.data() + size, &data, sizeof(T));
            return msg;
        }

        // Input string
        friend TCPRawMsg& operator << (TCPRawMsg& msg, const std::string& data) {
            msg.body.insert(msg.body.end(), data.begin(), data.end());
            return msg;
        }

        template <typename T>
        friend TCPRawMsg& operator >> (TCPRawMsg& msg, T& data) {
            static_assert(std::is_standard_layout<T>::value, 
                    "Data layout is not standard.");
            size_t size = msg.body.size() - sizeof(T);
            std::memcpy(&data, msg.body.data() + size, sizeof(T));
            msg.body.resize(size);
            return msg;
        }
        
        friend TCPRawMsg& operator >> (TCPRawMsg& msg, std::string& data) {
            size_t size = msg.body.size() - data.length();
            std::memcpy(data.data(), msg.body.data() + size, data.length());
            msg.body.resize(size);
            return msg;
        }

        std::string formatted() const {
            std::ostringstream oss;
            oss << "Raw data (size " << full_size() << "): \n";
            oss << std::uppercase << std::hex;
            for (const auto &byte: body) {
                oss << std::setw(2) << std::setfill('0') << static_cast<int>(byte) << ' ';
            }
            oss << std::nouppercase << std::dec << '\n';
            return oss.str();
        }
        
        friend std::ostream& operator << (std::ostream& os, const TCPRawMsg& msg) {
            os << msg.formatted();
            return os;
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

#endif //TCPCONN_TCPMSG_H
