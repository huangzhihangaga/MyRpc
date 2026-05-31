/**
 * @file ServerConnectionPool.h
 * @brief 单个服务端的连接池
 */

#ifndef RPC_SERVERCONNECTIONPOOL_H
#define RPC_SERVERCONNECTIONPOOL_H

#include <string>
#include <unordered_map>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <chrono>
#include <atomic>
#include <memory>
#include <vector>
#include <queue>
#include <future>
#include <list>

/**
 * @brief 单个服务端的连接池
 * @details 管理到特定(ip:port)的所有tcp连接，支持：
 *          容量控制
 *          等待队列
 *          空闲回收，定期关闭长时间未使用的连接
 *          健康检查，获取连接时验证连接有效性
 */
class ServerConnectionPool {
public:
    /**
     * @brief 构造函数
     * @param ip 目标服务端IP地址
     * @param port 目标服务端端口
     * @param maxConnections 最大连接数，默认10
     * @param idleTimeoutSeconds 空闲连接超时时间，默认60秒
     */
    ServerConnectionPool(const std::string& ip,uint16_t port,int maxConnections=10,int idleTimeoutSeconds=60);

    /**
     * @brief 析构函数，关闭所有连接并停止清理线程
     */
    ~ServerConnectionPool();

    /**
     * @brief 获取一个连接
     * @param timeoutMs 等待超时时间，默认3000毫秒
     * @return socket 文件描述符，-1表示失败或超时
     * @details 获取策略：
     *          1.优先从空闲队列获取有效连接
     *          2.如果无空闲连接并且未达到最大连接数，创建新连接
     *          3.如果已经达到最大连接数，阻塞等待其他线程归还
     */
    int GetConnection(int timeoutMs=3000);

    /**
     * @brief 归还连接
     * @param sockfd 要归还的文件描述符
     * @param isValid 连接是否仍然有效，false时关闭连接
     * @details 归还后连接进入空闲队列，并唤醒等待队列中的线程
     */
    void ReturnConnection(int sockfd,bool isValid=true);

    /**
     * @brief 关闭所有连接
     * @details 唤醒所有等待的线程
     */
    void CloseAllConnections();

    /**
     * @brief 获取当前连接总数
     * @return 连接整数
     */
    size_t GetCurrentConnectionCount() const;

    /**
     * @brief 获取当前空闲连接数
     * @return 空闲连接数
     */
    size_t GetIdleConnectionCount() const;
private:
    /**
     * @struc ConnectionItem
     * @brief 连接项结构体，存储单个连接的信息
     */
    struct ConnectionItem {
        /// 文件描述符
        int sockfd_;

        /// 服务端地址
        std::string ip_;

        /// 服务端端口
        uint16_t port_;

        /// 最后使用时间
        std::chrono::steady_clock::time_point lastUsedTime_;

        /// 连接是否有效
        bool isValid_;

        /// 连接是否正在使用
        bool isBusy_;

        /**
         * @brief 构造函数
         * @param fd 文件描述符
         * @param ip 服务端地址
         * @param port 服务端端口
         */
        ConnectionItem(int fd,const std::string& ip,uint16_t port)
            :sockfd_(fd)
            ,ip_(ip)
            ,port_(port)
            ,lastUsedTime_(std::chrono::steady_clock::now())
            ,isValid_(true)
            ,isBusy_(false){}
    };

    /**
     * @brief 创建新的tcp连接
     * @return 文件描述符，-1表示失败
     * @details 非阻塞连接，3秒超时
     */
    int CreateNewConnection();

    /**
     * @brief 检查连接是否存活
     * @param sockfd 文件描述符
     * @return true表示连接有效，false表示连接无效
     */
    bool CheckConnectionAlive(int sockfd);

    /**
     * @brief 清理空闲太久的连接
     * @details 由清理线程定期调用，关闭空闲超过idleTimeoutSeconds_的连接
     */
    void CleanupIdleConnections();

    /// 服务端地址
    std::string ip_;

    /// 服务端端口
    uint16_t port_;

    /// 最大连接数
    int maxConnections_;

    /// 空闲超时时间
    int idleTimeoutSeconds_;

    /// 所有连接 文件描述符:连接
    std::unordered_map<int,std::shared_ptr<ConnectionItem>> allConnections_;

    /// 空闲连接队列
    std::queue<std::shared_ptr<ConnectionItem>> idleQueue_;

    /// 保护共享数据的互斥锁
    mutable std::mutex mutex_;

    /// 等待连接条件变量
    std::condition_variable cond_;

    /// cleanupThread_等待超时或停止的条件变量
    std::condition_variable cleanupCond_;

    /// 允许标志
    std::atomic<bool> running_{true};

    /// 空闲连接清理线程
    std::thread cleanupThread_;
};
#endif //RPC_SERVERCONNECTIONPOOL_H
