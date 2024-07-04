//
// Created by Bohan Leng on 7/1/2024.
//

#ifndef ZMONITOR_TCPMSGQUEUE_H
#define ZMONITOR_TCPMSGQUEUE_H

#include <deque>
#include <mutex>

namespace TCPConn {

    template<typename T>
    class TCPMsgQueue {
    public:
        TCPMsgQueue() = default;
        TCPMsgQueue(const TCPMsgQueue<T>&) = delete;
        virtual ~TCPMsgQueue() { clear(); }

    public:
        const T& front() {
            std::scoped_lock lock(m_mutex);
            return m_queue.front();
        }

        const T& back() {
            std::scoped_lock lock(m_mutex);
            return m_queue.back();
        }

        void push_back(const T& item) {
            std::scoped_lock lock(m_mutex);
            m_queue.emplace_back(std::move(item));
            std::unique_lock<std::mutex> ul(m_mtxBlocking);
            m_cvBlocking.notify_one();
        }

        void push_front(const T& item) {
            std::scoped_lock lock(m_mutex);
            m_queue.emplace_front(std::move(item));
            std::unique_lock<std::mutex> ul(m_mtxBlocking);
            m_cvBlocking.notify_one();
        }

        bool empty() {
            std::scoped_lock lock(m_mutex);
            return m_queue.empty();
        }

        size_t count() {
            std::scoped_lock lock(m_mutex);
            return m_queue.size();
        }

        void clear() {
            std::scoped_lock lock(m_mutex);
            m_queue.clear();
        }

        T pop_front() {
            std::scoped_lock lock(m_mutex);
            auto item = std::move(m_queue.front());
            m_queue.pop_front();
            return item;
        }

        T pop_back() {
            std::scoped_lock lock(m_mutex);
            auto item = std::move(m_queue.back());
            m_queue.pop_back();
            return item;
        }
        
        void wait() {
            while (empty()) {
                std::unique_lock<std::mutex> ul(m_mtxBlocking);
                m_cvBlocking.wait(ul);
            }
        }

    protected:
        std::mutex m_mutex;
        std::deque<T> m_queue;
        std::condition_variable m_cvBlocking;
        std::mutex m_mtxBlocking;
    };

} // TCPConn

#endif //ZMONITOR_TCPMSGQUEUE_H
