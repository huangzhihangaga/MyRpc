/**
 * @file ZookeeperUtil.h
 * @brief ZkClient客户端类接口封装
 */
#ifndef RPC_ZOOKEEPERUTIL_H
#define RPC_ZOOKEEPERUTIL_H

#include <mutex>
#include <condition_variable>
#include <string>
#include <zookeeper/zookeeper.h>

/**
 * @brief 封装zookeeper客户端类
 */
class ZkClient {
public:
    /**
     * @brief ZkClient构造函数
     */
    ZkClient();

    /**
     * @brief ZkClient析构函数
     */
    ~ZkClient();

    /**
     * @brief zookeeper客户端连接服务器端
     */
    void Start();

    /**
     * @brief 在zookeeper上根据指定的path创建znode节点
     * @param path 路径
     * @param data 节点数据
     * @param datalen 数据长度
     * @param state 默认0位永久性节点，EPHEMERAL为临时性节点
     */
    void Create(const char* path,const char* data,int datalen,int state=0);

    /**
     * @brief 根据指定path获取相应的znode节点的值
     * @param path 路径
     * @return znode节点存储的值
     */
    std::string GetData(const char* path);

    /// 全局watcher函数 友元
    friend void globalWatcher(zhandle_t *zh, int type,
                              int state, const char *path,void *watcherCtx);
private:
    /// 互斥锁
    std::mutex mutex_;

    /// 条件变量
    std::condition_variable cond_;

    /// 表示是否已连接
    bool connected_;

    /// zookeeper客户端句柄
    zhandle_t* zhandle_;
};

#endif //RPC_ZOOKEEPERUTIL_H
