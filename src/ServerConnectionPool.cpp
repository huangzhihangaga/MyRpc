/**
 * @file ServerConnectionPool.cpp
 * @brief 单个服务端的连接池的实现
 */

#include "ServerConnectionPool.h"
#include "Logger.h"

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <cstring>
#include <poll.h>
#include <future>

ServerConnectionPool::ServerConnectionPool(const std::string &ip, uint16_t port, int maxConnections, int idleTimeoutSeconds)
    :ip_(ip)
    ,port_(port)
    ,maxConnections_(maxConnections)
    ,idleTimeoutSeconds_(idleTimeoutSeconds){
    cleanupThread_=std::thread([this]() {
        while (running_) {
            {
                std::unique_lock<std::mutex> lock(mutex_);
                bool flag=cleanupCond_.wait_for(lock,std::chrono::seconds(30),[this]() {
                    return !running_;
                });
                if (flag) {
                    break;
                }
            }
            CleanupIdleConnections();
        }
    });
}

ServerConnectionPool::~ServerConnectionPool() {
    running_=false;
    cond_.notify_all();
    cleanupCond_.notify_all();
    if (cleanupThread_.joinable()) {
        cleanupThread_.join();
    }
    CloseAllConnections();
}

int ServerConnectionPool::CreateNewConnection() {
    int sockfd=socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd==-1) {
        LOG_ERROR("create socket failed: %d", errno);
        return -1;
    }

    int flag=1;
    setsockopt(sockfd,IPPROTO_TCP,TCP_NODELAY, &flag, sizeof(flag));
    setsockopt(sockfd, SOL_SOCKET,SO_KEEPALIVE, &flag, sizeof(flag));

    int flags= fcntl(sockfd, F_GETFL, 0);
    fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);

    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port_);
    inet_pton(AF_INET, ip_.c_str(), &serverAddr.sin_addr);

    int ret=connect(sockfd,(struct sockaddr*)&serverAddr,sizeof(serverAddr));

    if (ret==-1) {
        if (errno!=EINPROGRESS) {
            LOG_ERROR("connect to %s:%d failed: %d", ip_.c_str(), port_, errno);
            close(sockfd);
            return -1;
        }

        struct pollfd pfd;
        pfd.fd=sockfd;
        pfd.events=POLLOUT;
        int pollRet=poll(&pfd,1,3000);
        if (pollRet<=0) {
            LOG_ERROR("connect to %s:%d timeout", ip_.c_str(), port_);
            close(sockfd);
            return -1;
        }

        if (pfd.revents & (POLLERR | POLLHUP)) {
            LOG_ERROR("connect to %s:%d error", ip_.c_str(), port_);
            close(sockfd);
            return -1;
        }

        int error;
        socklen_t len=sizeof(error);
        getsockopt(sockfd,SOL_SOCKET,SO_ERROR,&error,&len);
        if (error != 0) {
            LOG_ERROR("connect to %s:%d error: %d", ip_.c_str(), port_, error);
            close(sockfd);
            return -1;
        }
    }

    fcntl(sockfd, F_SETFL, flags);
    LOG_DEBUG("created new connection to %s:%d, fd=%d", ip_.c_str(), port_, sockfd);
    return sockfd;
}

bool ServerConnectionPool::CheckConnectionAlive(int sockfd) {
    char buf[1];
    ssize_t ret=recv(sockfd,buf,1,MSG_DONTWAIT | MSG_PEEK);

    if (ret>0) {
        return true;
    }
    if (ret==0) {
        LOG_DEBUG("connection %d closed by peer", sockfd);
        return false;
    }
    if (errno==EAGAIN || errno==EWOULDBLOCK) {
        return true;
    }
    return false;
}

int ServerConnectionPool::GetConnection(int timeoutMs) {
    while (true) {
        std::unique_lock<std::mutex> lock(mutex_);

        while (!idleQueue_.empty()) {
            auto conn=idleQueue_.front();
            idleQueue_.pop();

            if (conn->isValid_ && CheckConnectionAlive(conn->sockfd_)) {
                conn->isBusy_=true;
                conn->lastUsedTime_=std::chrono::steady_clock::now();
                LOG_DEBUG("get idle connection to %s:%d, fd=%d",ip_.c_str(), port_, conn->sockfd_);
                return conn->sockfd_;
            }else {
                LOG_DEBUG("closing invalid connection to %s:%d, fd=%d",ip_.c_str(), port_, conn->sockfd_);
                close(conn->sockfd_);
                allConnections_.erase(conn->sockfd_);
            }
        }

        if (allConnections_.size()<maxConnections_) {
            lock.unlock();
            int sockfd=CreateNewConnection();

            if (sockfd!=-1) {
                lock.lock();
                if (allConnections_.size()<maxConnections_) {
                    auto conn=std::make_shared<ConnectionItem>(sockfd,ip_,port_);
                    conn->isBusy_=true;
                    allConnections_[sockfd]=conn;
                    LOG_DEBUG("created new connection to %s:%d, total=%zu",ip_.c_str(), port_, allConnections_.size());
                    return sockfd;
                }else {
                    close(sockfd);
                    continue;
                }
            }

            LOG_ERROR("failed to create connection to %s:%d", ip_.c_str(), port_);
            return -1;
        }

        LOG_DEBUG("connections reached limits (%d), wait for connection to %s:%d",maxConnections_, ip_.c_str(), port_);

        bool flag=cond_.wait_for(lock,std::chrono::milliseconds(timeoutMs),[this]() {
            return !idleQueue_.empty() || !running_;
        });

        if (!flag) {
            LOG_WARN("wait for connection to %s:%d timeout after %d ms",ip_.c_str(), port_, timeoutMs);
            return -1;
        }
        if (!running_) {
            return -1;
        }
        auto conn=idleQueue_.front();
        idleQueue_.pop();

        if (conn->isValid_ && CheckConnectionAlive(conn->sockfd_)) {
            conn->isBusy_=true;
            conn->lastUsedTime_=std::chrono::steady_clock::now();
            LOG_DEBUG("get idle connection to %s:%d, fd=%d",ip_.c_str(), port_, conn->sockfd_);
            return conn->sockfd_;
        }else {
            LOG_DEBUG("closing invalid connection to %s:%d, fd=%d",ip_.c_str(), port_, conn->sockfd_);
            close(conn->sockfd_);
            allConnections_.erase(conn->sockfd_);
        }
    }
}

void ServerConnectionPool::ReturnConnection(int sockfd, bool isValid) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it=allConnections_.find(sockfd);
    if (it==allConnections_.end()) {
        LOG_WARN("return unknown connection fd=%d", sockfd);
        close(sockfd);
        return;
    }

    auto conn = it->second;
    conn->isBusy_=false;
    conn->isValid_=isValid;
    conn->lastUsedTime_=std::chrono::steady_clock::now();

    if (!isValid) {
        LOG_WARN("close invalid connection to %s:%d, fd=%d",ip_.c_str(), port_, sockfd);
        close(sockfd);
        allConnections_.erase(sockfd);
        return;
    }

    idleQueue_.push(conn);
    cond_.notify_one();
}

void ServerConnectionPool::CleanupIdleConnections() {
    std::lock_guard<std::mutex> lock(mutex_);
    auto now=std::chrono::steady_clock::now();

    std::queue<std::shared_ptr<ConnectionItem>> newQueue;
    while (!idleQueue_.empty()) {
        auto conn=idleQueue_.front();
        idleQueue_.pop();

        auto idleTime = std::chrono::duration_cast<std::chrono::seconds>(now - conn->lastUsedTime_);
        if (idleTime.count() < idleTimeoutSeconds_ && conn->isValid_) {
            newQueue.push(conn);
        } else {
            LOG_DEBUG("closing idle connection to %s:%d, idle=%lld seconds",ip_.c_str(), port_, (long long)idleTime.count());
            close(conn->sockfd_);
            allConnections_.erase(conn->sockfd_);
        }
    }

    idleQueue_.swap(newQueue);
}

void ServerConnectionPool::CloseAllConnections() {
    std::lock_guard<std::mutex> lock(mutex_);

    for (auto& pair:allConnections_) {
        close(pair.first);
    }
    allConnections_.clear();

    while (!idleQueue_.empty()) {
        idleQueue_.pop();
    }

    cond_.notify_all();
}

size_t ServerConnectionPool::GetCurrentConnectionCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return allConnections_.size();
}

size_t ServerConnectionPool::GetIdleConnectionCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return idleQueue_.size();
}
