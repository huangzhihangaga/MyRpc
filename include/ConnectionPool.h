/**
 * @file ConnectionPool.h
 * @brief 全局连接池管理器
 */

#ifndef RPC_CONNECTIONPOOL_H
#define RPC_CONNECTIONPOOL_H

#include <mutex>
#include <condition_variable>
#include <unordered_map>
#include <queue>
#include <memory>
#include <atomic>
#include <chrono>
#include <string>
#include <thread>
#include <future>

#include "ServerConnectionPool.h"

/**
 * @brief 全局连接池管理器，单例模式
 * @details 管理到不同服务端的连接池，每个(ip:port)对应一个ServerConnectionPool
 *          功能：
 *          1.按服务端地址管理独立的连接池
 *          2.提供统一的获取/归还连接接口
 *          3.支持连接复用
 */
class ConnectionPool {
public:
    /**
     * @brief 删除拷贝构造
     */
    ConnectionPool(const ConnectionPool&)=delete;

    /**
     * @brief 删除拷贝赋值
     * @return 已删除
     */
    ConnectionPool& operator=(const ConnectionPool&)=delete;

    /**
     * @brief 获取单例实例
     * @return ConnectionPool的单例引用
     */
    static ConnectionPool& GetInstance();

    /**
     * @brief 从连接池获取一个连接
     * @param ip 服务端地址
     * @param port 服务端端口
     * @param timeoutMs 获取连接的超时时间，默认3000毫秒
     * @return socket文件描述符，-1表示失败或超时
     * @details 执行流程：
     *          1.查找或创建对应服务端的连接池
     *          2.从连接池中共获取空闲连接
     *          3.如果无空闲连接并且未达上限，创建新连接
     *          4.如果已达上限，阻塞等待其他线程归还连接
     */
    int GetConnection(const std::string& ip,uint16_t port,int timeoutMs=3000);

    /**
     * @brief 归还连接到连接池
     * @param ip 服务端地址
     * @param port 服务端端口
     * @param sockfd 要归还的文件描述符
     * @param isValid 连接是否有效，false时会关闭连接
     */
    void ReturnConnection(const std::string& ip,uint16_t port,int sockfd,bool isValid=true);

    /**
     * @brief 关闭所有连接池中的所有连接
     */
    void CloseAll();
private:
    /**
     * @brief 私有构造函数
     */
    ConnectionPool()=default;

    /**
     * @brief 私有析构函数
     */
    ~ConnectionPool()=default;

    /**
     * @brief 获取或创建指定服务端的连接池
     * @param ip 服务端地址
     * @param port 服务端端口
     * @return ServerConnectionPool的shared_ptr
     */
    std::shared_ptr<ServerConnectionPool> GetOrCreatePool(const std::string& ip,uint16_t port);

    /// 映射 ip:port -> 连接池的shared_ptr
    std::unordered_map<std::string,std::shared_ptr<ServerConnectionPool>> pools_;

    /// 保护pools_的互斥锁
    mutable std::mutex mutex_;

    /// 每个服务端的最大连接数，默认为10
    static constexpr int MaxConnections=10;

    /// 空闲连接超时时间，默认60秒
    static constexpr int IdleTimeout=60;
};

#endif //RPC_CONNECTIONPOOL_H
