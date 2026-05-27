/**
 * @file MprpcProvider.cpp
 * @brief rpc服务提供类接口实现
 */

#include "MprpcProvider.h"
#include "MprpcApplication.h"
#include "rpcheader.pb.h"
#include "Logger.h"
#include "ZookeeperUtil.h"

#include <google/protobuf/descriptor.h>
#include <functional>
#include <string>
#include <memory>

void MprpcProvider::NotifyService(google::protobuf::Service *service) {
    ServiceInfo serviceInfo;

    // 获取了服务对象的描述信息
    const google::protobuf::ServiceDescriptor* serviceDesc=service->GetDescriptor();

    // 获取服务的名字
    std::string serviceName=serviceDesc->name();

    // 获取服务对象service的方法method数量
    int methodCount=serviceDesc->method_count();

    LOG_INFO("service_name:%s",serviceName.c_str());

    for (int i=0;i<methodCount;++i) {
        // 获取了服务对象指定下标的服务方法描述
        const google::protobuf::MethodDescriptor* methodDesc=serviceDesc->method(i);
        std::string methodName=methodDesc->name();
        serviceInfo.methodMap_.insert({methodName,methodDesc});

        LOG_INFO("method_name:%s",methodName.c_str());
    }
    serviceInfo.service_=service;
    serviceMap_.insert({serviceName,serviceInfo});
}

void MprpcProvider::Run(int threadNum) {
    std::string ip=MprpcApplication::GetConfig().Load("rpcserverip");
    uint16_t port=std::stoi(MprpcApplication::GetConfig().Load("rpcserverport"));
    muduo::net::InetAddress address(ip,port);

    // 创建TcpServer对象
    muduo::net::TcpServer server(&eventLoop_,address,"MprpcProvider");

    // 绑定连接回调和消息回调方法 分离网络代码和业务代码
    server.setConnectionCallback(std::bind(&MprpcProvider::onConnection,this,std::placeholders::_1));
    server.setMessageCallback(std::bind(&MprpcProvider::onMessage,this,
                                                    std::placeholders::_1,
                                                    std::placeholders::_2,
                                                    std::placeholders::_3));
    // 设置muduo库的线程数量
    server.setThreadNum(threadNum);

    // 把当前rpc节点上要发布的服务全部注册到zookeeper上，让rpc客户端可以从zookeeper上面发现服务
    // 从配置文件中读取zookeeper服务器地址和端口并连接
    ZkClient zkclient;
    zkclient.Start();

    // service_name为永久性节点 method_name为临时节点
    for (auto& sp:serviceMap_) {
        // eg. /service_name    /UserServiceRpc
        std::string servicePath="/"+sp.first;
        zkclient.Create(servicePath.c_str(),nullptr,0);
        for (auto& mp:sp.second.methodMap_) {
            // eg. /service_name/method  /UserServiceRpc/Login  存储当前这个rpc服务节点主机的ip和port
            std::string methodPath=servicePath+"/"+mp.first;
            char methodPathData[64]={0};
            int written=snprintf(methodPathData,sizeof(methodPathData),"%s:%u",ip.c_str(),port);

            // ZOO_EPHEMERAL表示znode是临时性节点，客户端断开连接后，zookeeper会自动删除这个节点
            zkclient.Create(methodPath.c_str(),methodPathData,written,ZOO_EPHEMERAL);
        }
    }

    // rpc服务端准备启动，打印信息
    LOG_INFO("MprpcProvider start service at ip:%s port:%d",ip.c_str(),port);

    // 启动网络服务
    server.start();
    eventLoop_.loop();
}

void MprpcProvider::onConnection(const muduo::net::TcpConnectionPtr &conn) {
    if (!conn->connected()) {
        // 如果连接关闭，则断开连接
        conn->shutdown();
    }
}

// 在框架内部 RpcProvider和RpcConsumer要协商之间通信用的protobuf数据类型
// service_name method_name args_size 定义proto的message类型，进行数据头的序列化和反序列化
//                               service_name method_name args_size
//
// header_size(4个字节，表示数据头，及除参数外数据的长度) + header_str + args_str
void MprpcProvider::onMessage(const muduo::net::TcpConnectionPtr &conn,
                            muduo::net::Buffer *buffer,
                            muduo::Timestamp receive_time)
{
    // 网络缓冲区中去取出远程rpc调用请求的字符流
    std::string recvBuffer=buffer->retrieveAllAsString();

    // 从字符流中读取前4个字节的内容
    uint32_t header_size=0;
    recvBuffer.copy((char*)&header_size,4,0);
    header_size=ntohl(header_size);

    // 根据header_size读取数据头的原始字符流,反序列化数据得到rpc请求的详细信息
    std::string rpc_header_str=recvBuffer.substr(4,header_size);
    mprpc::RpcHeader rpcHeader;
    std::string service_name;
    std::string method_name;
    uint32_t args_size;
    if (!rpcHeader.ParseFromString(rpc_header_str)) {
        LOG_DEBUG("rpc_header_str:%s parse error!",rpc_header_str.c_str());
        return;
    }
    service_name=rpcHeader.service_name();
    method_name=rpcHeader.method_name();
    args_size=rpcHeader.args_size();

    // 获取rpc方法参数的字符流数据
    std::string args_str=recvBuffer.substr(4+header_size,args_size);

    // 打印调试信息
    LOG_DEBUG("==================");
    LOG_DEBUG("header_size:%u", header_size);
    LOG_DEBUG("rpc_header_str:%s", rpc_header_str.c_str());
    LOG_DEBUG("service_name:%s", service_name.c_str());
    LOG_DEBUG("method_name:%s", method_name.c_str());
    LOG_DEBUG("args_size:%u", args_size);
    LOG_DEBUG("==================");

    // 获取service对象和method对象
    auto it=serviceMap_.find(service_name);
    if (it==serviceMap_.end()) {
        LOG_ERROR("service %s is not exist!", service_name.c_str());
        return;
    }

    auto mit=it->second.methodMap_.find(method_name);
    if (mit==it->second.methodMap_.end()) {
        LOG_ERROR("service %s method %s is not exist!", service_name.c_str(), method_name.c_str());
        return;
    }

    google::protobuf::Service *service=it->second.service_;
    const google::protobuf::MethodDescriptor* method=mit->second;

    // 生成rpc方法调用的请求request和响应response参数
    std::unique_ptr<google::protobuf::Message> request(service->GetRequestPrototype(method).New());
    std::unique_ptr<google::protobuf::Message> response(service->GetResponsePrototype(method).New());
    if (!request->ParseFromString(args_str)) {
        LOG_ERROR("request parse error, content:%s", args_str.c_str());
        return;
    }

    // 给下面的method方法的调用，绑定一个Closure的回调函数
    google::protobuf::Closure* done=
        google::protobuf::NewCallback<MprpcProvider,
                                        const muduo::net::TcpConnectionPtr&,
                                        google::protobuf::Message*>
                                        (this,
                                            &MprpcProvider::SendRpcResponse,
                                            conn,
                                            response.get());

    // 在框架上根据远端rpc请求，调用当前rpc节点上发布的方法
    // new UserService().Login(controller,request,response,done)
    service->CallMethod(method,nullptr,request.get(),response.release(),done);
}

void MprpcProvider::SendRpcResponse(const muduo::net::TcpConnectionPtr &conn,
                                    google::protobuf::Message *response)
{
    std::string responseStr;
    if (response->SerializeToString(&responseStr)) {
        uint32_t len=htonl(static_cast<uint32_t>(responseStr.size()));
        conn->send(&len,4);
        conn->send(responseStr);
    }else {
        LOG_ERROR("serialize response error");
    }
    conn->shutdown();
    delete response;
}

MprpcProvider::~MprpcProvider() {
    LOG_INFO("MprpcProvider destructor");
    eventLoop_.quit();
}
