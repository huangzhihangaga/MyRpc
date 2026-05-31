/**
 * @file ConnectionPool.cpp
 * @brief 全局连接池管理器实现
 */

#include "ConnectionPool.h"
#include "Logger.h"
#include <unistd.h>

ConnectionPool &ConnectionPool::GetInstance() {
    static ConnectionPool instance;
    return instance;
}

std::shared_ptr<ServerConnectionPool> ConnectionPool::GetOrCreatePool(const std::string &ip, uint16_t port) {
    std::string key=ip+":"+std::to_string(port);

    std::lock_guard<std::mutex> lock(mutex_);
    auto it = pools_.find(key);
    if (it != pools_.end()) {
        return it->second;
    }

    auto pool=std::make_shared<ServerConnectionPool>(ip, port,MaxConnections,IdleTimeout);
    pools_[key]=pool;
    return pool;
}

int ConnectionPool::GetConnection(const std::string &ip, uint16_t port, int timeoutMs) {
    auto pool=GetOrCreatePool(ip, port);
    return pool->GetConnection(timeoutMs);
}

void ConnectionPool::ReturnConnection(const std::string &ip, uint16_t port, int sockfd, bool isValid) {
    std::string key = ip + ":" + std::to_string(port);

    std::lock_guard<std::mutex> lock(mutex_);
    auto it = pools_.find(key);
    if (it != pools_.end()) {
        it->second->ReturnConnection(sockfd, isValid);
    } else {
        LOG_WARN("return connection to unknown pool: %s", key.c_str());
        close(sockfd);
    }
}

void ConnectionPool::CloseAll() {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& pair : pools_) {
        pair.second->CloseAllConnections();
    }
    pools_.clear();
}


