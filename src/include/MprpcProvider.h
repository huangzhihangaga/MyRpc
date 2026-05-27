/**
 * @file MprpcProvider.h
 * @brief rpc服务提供类接口
 */

#ifndef RPC_RPCPROVIDER_H
#define RPC_RPCPROVIDER_H

#include <google/protobuf/service.h>
#include <memory>
#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/InetAddress.h>
#include <muduo/net/TcpConnection.h>
#include <string>
#include <unordered_map>

/**
 * @brief 框架提供的专门用于发布rpc服务的网络对象类
 * @details 主要功能：
 *          注册RPC服务对象及其方法
 *          启动TCP服务器监听客户端请求
 *          将服务信息注册到zookeeper
 *          接收并处理RPC调用请求
 */
class MprpcProvider {
public:
    /**
     * @brief 默认构造函数
     */
    MprpcProvider()=default;

    /**
     * @brief 发布RPC服务对象
     * @param service google::protobuf::Service基类，实际传入继承google::protobuf::Service的派生类
     * @details 函数参数不应该设置成某个具体业务中使用到的类
     *          通过基类指针来接收派生类完成框架的功能
     */
    void NotifyService(google::protobuf::Service* service);

    /**
     * @brief 析构函数，退出事件循环
     */
    ~MprpcProvider();

    /**
     * @brief 启动rpc服务节点
     * @param threadNum Tcp服务器的线程数，默认为4
     * @details 执行流程：
     *          1.从配置文件读取ip和端口
     *          2.创建TCP服务器
     *          3.设置连接回调和消息回调
     *          4.连接zookeeper
     *          5.将服务信息注册到zookeeper
     *          6.启动事件循环
     */
    void Run(int threadNum=4);

private:
    /// EventLoop
    muduo::net::EventLoop eventLoop_;

    /**
     * @struct ServiceInfo
     * @brief 服务信息结构体，存储单个服务的数据
     */
    struct ServiceInfo {
        /// 服务对象指针
        google::protobuf::Service* service_;

        /// 方法名:方法描述符 映射表
        std::unordered_map<std::string,const google::protobuf::MethodDescriptor*> methodMap_;
    };

    /// 服务名:服务信息结构体 映射表
    std::unordered_map<std::string,ServiceInfo> serviceMap_;

    /**
     * @brief 建立连接或断开连接的回调
     * @param conn TcpConnection智能指针的引用
     */
    void onConnection(const muduo::net::TcpConnectionPtr& conn);

    /**
     * @brief 消息接收回调
     * @param conn TcpConnection智能指针的引用
     * @param buffer 接收缓冲区
     * @param receive_time 接收时间戳
     * @details 协议解析流程：
     *          1.读取4字节的header_size
     *          2.读取header
     *          3.反序列化header获取service_name、method_name、args_size
     *          4.读取请求参数args
     *          5.查找相应的服务和方法
     *          6.创建请求/响应对象并调用服务方法
     */
    void onMessage(const muduo::net::TcpConnectionPtr& conn,
                   muduo::net::Buffer* buffer,
                   muduo::Timestamp receive_time);

    /**
     * @brief 将rpc方法的执行结果发送回rpc客户端
     * @param conn TcpConnection智能指针的引用
     * @param response 用户传回的数据
     * @details 作为Closure的回调操作，在服务方法执行完成后调用
     */
    void SendRpcResponse(const muduo::net::TcpConnectionPtr& conn,
                         google::protobuf::Message* response);
};

#endif //RPC_RPCPROVIDER_H
