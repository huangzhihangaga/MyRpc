/**
 * @file rpcprovider.h
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
 */
class MprpcProvider {
public:
    /**
     * @brief 框架提供给外部使用的，可以发布rpc方法的函数接口
     * @param service google::protobuf::Service基类，实际传入继承google::protobuf::Service的派生类
     * @details 函数参数不应该设置成某个具体业务中使用到的类，通过基类指针来接收派生类完成框架的功能
     */
    void NotifyService(google::protobuf::Service* service);

    /**
     * @brief 析构函数，退出事件循环
     */
    ~MprpcProvider();

    /**
     * @brief 启动rpc服务节点，开始提供rpc远程网路调用服务
     */
    void Run();

private:
    /// 组合EventLoop
    muduo::net::EventLoop m_eventLoop;

    /// 服务类型信息
    struct ServiceInfo {
        /// 保存服务对象
        google::protobuf::Service* m_service;
        /// 保存字符串和服务方法映射关系
        std::unordered_map<std::string,const google::protobuf::MethodDescriptor*> m_methodMap;
    };

    /// 存储字符串和(注册成功的服务对象和其服务方法的所有信息)的映射关系
    std::unordered_map<std::string,ServiceInfo> m_serviceMap;

    /**
     * @brief 建立连接或断开连接的回调
     * @param conn TcpConnection智能指针的引用
     */
    void onConnection(const muduo::net::TcpConnectionPtr& conn);

    /**
     * @brief 已建立连接用户的读写事件回调
     * @param conn TcpConnection智能指针的引用
     * @param buffer 指向提供的输入缓冲区
     * @param receive_time 接收到最后以一个TCP数据包的时刻
     */
    void onMessage(const muduo::net::TcpConnectionPtr& conn,
                   muduo::net::Buffer* buffer,
                   muduo::Timestamp receive_time);

    /**
     * @brief Closure的回调操作，将rpc方法的执行结果发送会rpc客户端
     * @param conn TcpConnection智能指针的引用
     * @param response 用户传回的数据
     */
    void SendRpcResponse(const muduo::net::TcpConnectionPtr& conn,
                         google::protobuf::Message* response);
};

#endif //RPC_RPCPROVIDER_H
