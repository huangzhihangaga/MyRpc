/**
 * @file zookeeperutil.cpp
 * @brief zookeeper客户端类接口实现
 */

#include "ZookeeperUtil.h"
#include "MprpcApplication.h"
#include <semaphore>
#include <iostream>
#include <zookeeper/zookeeper.h>
#include "Logger.h"

/**
 * @internal
 * @brief 全局的watcher观察器
 * @details 只处理会话事件，当连接建立成功时，从句柄获取信号量并执行sem_post，使用信号量机制同步等待事件完成，因为zookeeper_init是异步的
 * @param zh zookeeper客户端句柄，代表与ZK服务器的连接
 * @param type 事件类型，如会话事件、数据变化事件等
 * @param state 连接状态，如已连接、断开连接等
 * @param path 节点路径
 * @param watcherCtx 用户上下文
 */
void global_watcher(zhandle_t* zh,int type,int state,
                    const char* path,void* watcherCtx) {
    // 判断事件类型是否是会话类型
    if (type==ZOO_SESSION_EVENT) {
        // 判断连接状态是否为已连接
        if (state==ZOO_CONNECTED_STATE) {
            // 获取关联的信号量指针并执行post
            sem_t* sem=(sem_t*)zoo_get_context(zh);
            sem_post(sem);
        }
    }
}

ZkClient::ZkClient():m_zhandle(nullptr) {

}

ZkClient::~ZkClient() {
    if (m_zhandle != nullptr) {
        // 关闭句柄，释放资源
        zookeeper_close(m_zhandle);
    }
}

void ZkClient::Start() {
    std::string host=MprpcApplication::GetConfig().Load("zookeeperip");
    std::string port=MprpcApplication::GetConfig().Load("zookeeperport");
    std::string connstr=host+":"+port;

    // zookeeper_mt 多线程版本
    // zookeeper的API客户端程序提供了三个线程
    // API调用线程
    // 网络IO线程
    // watcher回调线程
    // 发起zk连接和zk连接响应是异步的
    m_zhandle=zookeeper_init(connstr.c_str(),global_watcher,30000,nullptr,nullptr,0);
    if (nullptr==m_zhandle) {
        LOG_ERROR("zookeeper_init error! connstr:%s", connstr.c_str());
        exit(EXIT_FAILURE);
    }

    sem_t sem;
    sem_init(&sem,0,0);
    zoo_set_context(m_zhandle,&sem);

    // 等待global_watcher中的sem_post(sem)
    sem_wait(&sem);
    LOG_INFO("zookeeper_init success! host:%s", connstr.c_str());
    sem_destroy(&sem);
}

void ZkClient::Create(const char *path, const char *data, int datalen, int state) {
    char path_buffer[128];
    int bufferlen=sizeof(path_buffer);
    int flag;
    // 判断path表示的znode节点是否存在，如果存在，就不再重复重建
    flag=zoo_exists(m_zhandle,path,0,nullptr);
    if (ZNONODE==flag) { // 表示path的节点不存在
        // 创建指定path的znode节点了
        flag=zoo_create(m_zhandle,path,data,datalen,
                        &ZOO_OPEN_ACL_UNSAFE,state,path_buffer,bufferlen);
        if (flag==ZOK) {
            LOG_INFO("znode create success: %s", path);
        }else {
            LOG_ERROR("znode create error: %s, flag:%d", path, flag);
            exit(EXIT_FAILURE);
        }
    }
}

// 根据执行的path获取指定znode节点的值
std::string ZkClient::GetData(const char *path) {
    char buffer[64];
    int bufferlen=sizeof(buffer);
    int flag=zoo_get(m_zhandle,path,0,buffer,&bufferlen,nullptr);
    if (flag!=ZOK) {
        LOG_WARN("get znode error: %s, flag:%d", path, flag);
        return "";
    }else {
        LOG_DEBUG("get znode success: %s, data:%s", path, buffer);
        return buffer;
    }
}
