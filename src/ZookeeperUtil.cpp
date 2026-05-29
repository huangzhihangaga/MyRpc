/**
 * @file ZookeeperUtil.cpp
 * @brief ZkClient客户端接口实现
 */

#include "MprpcApplication.h"
#include "ZookeeperUtil.h"
#include "Logger.h"

/**
 * @brief 全局的watcher回调函数
 * @param zh zookeeper客户端句柄
 * @param type 事件类型，如会话事件、数据变化事件等
 * @param state 连接状态，如已连接、已断开
 * @param path 节点路径
 * @param watcherCtx 用户传递的上下文指针，对应zookeeper_init中的参数context
 * @details 作为ZkClient的友元函数，因为zookeeper_init是异步的，通过条变量确保初始化完成
 * @related ZkClient
 */
void globalWatcher(zhandle_t *zh, int type,
                   int state, const char *path,void *watcherCtx) {
    // 判断事件类型是否是会话类型
    if (type==ZOO_SESSION_EVENT) {
        // 判断连接状态是否位已连接
        if (state==ZOO_CONNECTED_STATE) {
            ZkClient* client=static_cast<ZkClient*>(watcherCtx);
            {
                std::lock_guard<std::mutex> lock(client->mutex_);
                client->connected_=true;
            }
            client->cond_.notify_one();
        }
    }
}

ZkClient::ZkClient()
    :zhandle_(nullptr)
    ,connected_(false)
    ,mutex_()
    ,cond_(){

}

ZkClient &ZkClient::GetInstance() {
    static ZkClient instance;
    return instance;
}

ZkClient::~ZkClient() {
    if (zhandle_!=nullptr) {
        zookeeper_close(zhandle_);
    }
}

void ZkClient::Start() {
    std::string host=MprpcApplication::GetConfig().Load("zookeeperip");
    std::string port=MprpcApplication::GetConfig().Load("zookeeperport");
    std::string connstr=host+":"+port;

    // zookeeper_mt多线程版本中
    // zookeeper的API客户端提供了三个线程
    // 主线程负责初始化和业务触发 1.发起请求 2.等待同步结果 3.接收会话状态变化等关键事件通知
    // IO线程负责网络通信和协议处理 1.使用poll机制 2.处理握手、心跳和同步请求的响应 3.转交异步请求响应和Watch事件
    // 完成线程负责回调事件的分发执行 1.处理异步API的业务回调函数 2.执行Watcher机制的回调
    zhandle_=zookeeper_init(connstr.c_str(),globalWatcher,30000,nullptr,this,0);
    if (zhandle_==nullptr) {
        LOG_ERROR("zookeeper_init error! connstr:%s", connstr.c_str());
        exit(-1);
    }

    std::unique_lock<std::mutex> lock(mutex_);
    cond_.wait(lock,[this](){return connected_;});

    LOG_INFO("zookeeper_init success! host:%s", connstr.c_str());
}

void ZkClient::Create(const char *path, const char *data, int datalen, int state) {
    char buffer[128]={0};
    int bufferSize=sizeof(buffer);
    int flag=zoo_exists(zhandle_,path,0,nullptr);
    if (flag==ZNONODE) {
        // 节点不存在
        flag=zoo_create(zhandle_,path,data,datalen,&ZOO_OPEN_ACL_UNSAFE,state,buffer,bufferSize);
        if (flag==ZOK) {
            LOG_INFO("znode create success: %s", path);
        }else {
            LOG_ERROR("znode create error: %s, flag:%d", path, flag);
            exit(-1);
        }
    }
}

std::string ZkClient::GetData(const char *path) {
    std::string pathStr(path);
    auto now=std::chrono::steady_clock::now();
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it=cache_.find(pathStr);
        if (it!=cache_.end()) {
            if (now<it->second.expireTime) {
                return it->second.data;
            }
            cache_.erase(it);
        }
        std::string data=GetDataFromZkServer(path);
        if (!data.empty()) {
            CacheItem item;
            item.data=data;
            item.expireTime=now+std::chrono::seconds(CacheTTLSeconds);
            cache_[pathStr]=item;
            LOG_DEBUG("cache added for path: %s, data: %s", path, data.c_str());
        }
        return data;
    }
}

std::string ZkClient::GetDataFromZkServer(const char *path) {
    char buffer[64]={0};
    int bufferSize=sizeof(buffer);
    int flag=zoo_get(zhandle_,path,0,buffer,&bufferSize,nullptr);
    if (flag==ZOK) {
        LOG_DEBUG("get znode success: %s, data:%s", path, buffer);
        return std::string(buffer,bufferSize);
    }else {
        LOG_WARN("get znode error: %s, flag:%d", path, flag);
        return "";
    }
}

void ZkClient::DeleteCache(const std::string &path) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it=cache_.find(path);
    if (it!=cache_.end()) {
        cache_.erase(it);
        LOG_DEBUG("cache delete path:%s",path.c_str());
    }
}

void ZkClient::DeleteAllCache() {
    std::lock_guard<std::mutex> lock(mutex_);
    cache_.clear();
    LOG_DEBUG("delete all cache");
}
