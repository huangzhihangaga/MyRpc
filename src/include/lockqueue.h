/**
 * @file lockqueue.h
 * @brief 线程安全队列模块
 */

#ifndef RPC_LOCKQUEUE_H
#define RPC_LOCKQUEUE_H

#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>

/**
 * @brief 线程安全的队列
 * @tparam T 队列中存储的元素类型
 */
template <typename T>
class LockQueue {
public:

    /**
     * @brief 向队列中压入一个元素
     * @param data 要压入的元素
     */
    void Push(const T& data) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_queue.push(data);
        m_condvariable.notify_one();
    }

    /**
     * @brief 从队列中共弹出一个元素
     * @return 队列头元素
     */
    T Pop() {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_condvariable.wait(lock,[this](){return !m_queue.empty();});
        T data=m_queue.front();
        m_queue.pop();
        return data;
    }

private:
    /// 底层存储的队列
    std::queue<T> m_queue;

    /// 用于保护队列操作的互斥锁
    std::mutex m_mutex;

    /// 用于线程同步的条件变量
    std::condition_variable m_condvariable;
};
#endif //RPC_LOCKQUEUE_H
