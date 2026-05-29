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
#include <unordered_map>
#include <chrono>

/**
 * @brief 封装zookeeper客户端类
 * @details 使用单例模式，只需要在MprpcApplication::Init()中连接zookeeper一次
 */
class ZkClient {
public:
    /**
     * @brief 删除拷贝构造
     */
    ZkClient(const ZkClient&)=delete;

    /**
     * @brief 删除拷贝赋值
     * @return 已删除
     */
    ZkClient& operator=(const ZkClient&)=delete;

    /**
     * @brief 获取单例
     * @return 返回ZkClient的引用
     */
    static ZkClient& GetInstance();

    /**
     * @brief zookeeper客户端连接服务器端
     * @details 只调用一次
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
     * @brief 根据指定path获取相应的znode节点的值，带缓存
     * @param path 路径
     * @return znode节点存储的值
     */
    std::string GetData(const char* path);

    /**
     * @brief 删除指定路径的缓存
     * @param path 路径
     */
    void DeleteCache(const std::string& path);

    /**
     * @brief 删除所有缓存
     */
    void DeleteAllCache();
    
    /// 全局watcher函数 友元
    friend void globalWatcher(zhandle_t *zh, int type,
                              int state, const char *path,void *watcherCtx);
private:
    /**
     * @brief ZkClient构造函数
     */
    ZkClient();

    /**
     * @brief ZkClient析构函数
     */
    ~ZkClient();

    /**
     * @brief 缓存结构体
     */
    struct CacheItem {
        /// 缓存的数据
        std::string data;

        /// 过期时间
        std::chrono::steady_clock::time_point expireTime;
    };

    /**
     * @brief 从ZkServer上获取数据
     * @param path 路径
     * @return znode存节点储的值
     */
    std::string GetDataFromZkServer(const char* path);

    /// 互斥锁
    std::mutex mutex_;

    /// 条件变量
    std::condition_variable cond_;

    /// 表示是否已连接
    bool connected_;

    /// zookeeper客户端句柄
    zhandle_t* zhandle_;

    /// 服务地址缓存
    std::unordered_map<std::string,CacheItem> cache_;

    /// 缓存时间，默认60秒
    static constexpr int CacheTTLSeconds=60;
};

#endif //RPC_ZOOKEEPERUTIL_H
