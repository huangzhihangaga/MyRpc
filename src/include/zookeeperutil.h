/**
 * @file zookeeperutil.h
 * @brief zookeeper客户端类接口封装
 */
#ifndef RPC_ZOOKEEPERUTIL_H
#define RPC_ZOOKEEPERUTIL_H

#include <semaphore.h>
#include <zookeeper/zookeeper.h>
#include <string>

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
     * @brief zkClient连接启动zkServer
     */
    void Start();

    /**
     * @brief 在zkServer上根据指定的path创建znode节点
     * @param path 路径
     * @param data 节点数据
     * @param datalen 数据长度
     * @param state 永久性节点还是临时性节点，默认0为永久性节点
     */
    void Create(const char* path,const char* data,int datalen,int state=0);

    /**
     * @brief 根据参数指定的znode节点路径，获取znode节点的值
     * @param path 路径
     * @return znode节点的值
     */
    std::string GetData(const char* path);

private:
    /// zk的客户端句柄
    zhandle_t* m_zhandle;
};

#endif //RPC_ZOOKEEPERUTIL_H
