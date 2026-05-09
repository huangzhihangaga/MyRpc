//
// Created by 小晓 on 2026/4/28.
//

#ifndef RPC_LOCKQUEUE_H
#define RPC_LOCKQUEUE_H

#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>

/**
 * @brief 异步日志队列
 * @tparam T 队列中存储的元素类型
 */
template <typename T>
class LockQueue {
public:

    /**
     * @brief 多个worker线程都会写入日志queue
     * @param data
     */
    void Push(const T& data) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_queue.push(data);
        m_condvariable.notify_one();
    }

    /**
     * @brief 一个线程读日志queue，写日志文件
     * @return 读出日志queue的队头元素
     */
    T Pop() {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_condvariable.wait(lock,[this](){return !m_queue.empty();});
        T data=m_queue.front();
        m_queue.pop();
        return data;
    }

private:
    std::queue<T> m_queue;
    std::mutex m_mutex;
    std::condition_variable m_condvariable;
};
#endif //RPC_LOCKQUEUE_H
