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
        uint32_t id{};
        uint32_t size = 0;
    };

    struct TCPMsg {
        TCPMsgHeader header{};
        std::vector<uint8_t> body;

        size_t size() const {
            return sizeof(TCPMsgHeader) + body.size();
        }

        friend std::ostream &operator<<(std::ostream &os, const TCPMsg &msg) {
            os << "ID: " << int(msg.header.id) << " Size: " << msg.header.size;
            return os;
        }

        template<typename DataType>
        friend TCPMsg &operator<<(TCPMsg &msg, const DataType &data) {
            static_assert(std::is_standard_layout<DataType>::value, "Data is too complex to be pushed into vector");
            size_t size = msg.body.size();
            msg.body.resize(size + sizeof(DataType));
            std::memcpy(msg.body.data() + size, &data, sizeof(DataType));
            msg.header.size = msg.size();
            return msg;
        }

        template<typename DataType>
        friend TCPMsg &operator>>(TCPMsg &msg, DataType &data) {
            static_assert(std::is_standard_layout<DataType>::value, "Data is too complex to be pushed into vector");
            size_t size = msg.body.size() - sizeof(DataType);
            std::memcpy(&data, msg.body.data() + size, sizeof(DataType));
            msg.body.resize(size);
            msg.header.size = msg.size();
            return msg;
        }

    };

    class ITCPConn;

    struct TCPMsgOwned {
        std::shared_ptr<ITCPConn> remote = nullptr;
        TCPMsg msg;

        uint32_t size() const {
            return msg.size();
        }

        friend std::ostream &operator<<(std::ostream &os, const TCPMsgOwned &msg) {
            os << msg.msg;
            return os;
        }
    };

} // TCPCon


#endif //ZMONITOR_TCPMSG_H
