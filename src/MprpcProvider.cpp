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

    ZkClient& zkclient=ZkClient::GetInstance();

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
        contextsMap_.erase(conn);
        conn->shutdown();
    }
}

void MprpcProvider::onMessage(const muduo::net::TcpConnectionPtr &conn,
                            muduo::net::Buffer *buffer,
                            muduo::Timestamp receive_time)
{
    auto it=contextsMap_.find(conn);
    if (it==contextsMap_.end()) {
        contextsMap_[conn]=std::make_unique<ParseContext>();
        it=contextsMap_.find(conn);
    }

    std::unique_ptr<ParseContext>& context=it->second;
    while (true) {
        /// 4个字节的头部长度
        if (context->state==ParseState::ExpectHeaderSize) {
            if (buffer->readableBytes()<4) {
                return;
            }

            uint32_t header_size=0;
            memcpy(&header_size,buffer->peek(),4);
            buffer->retrieve(4);
            context->headerSize=ntohl(header_size);

            context->state=ParseState::ExpectHeader;
            context->headerBuffer.clear();
            context->headerBuffer.reserve(header_size);
        }

        if (context->state==ParseState::ExpectHeader) {
            if (buffer->readableBytes()<context->headerSize) {
                return;
            }
            context->headerBuffer.append(buffer->peek(),context->headerSize);
            buffer->retrieve(context->headerSize);
            mprpc::RpcHeader rpcHeader;
            if (!rpcHeader.ParseFromString(context->headerBuffer)) {
                LOG_ERROR("parse rpc_header error");
                context->state=ParseState::ExpectHeaderSize;
                context->headerBuffer.clear();
            }
            context->argsSize=rpcHeader.args_size();

            context->state=ParseState::ExpectArgs;
            context->argsBuffer.clear();
            context->argsBuffer.reserve(context->argsSize);
        }

        if (context->state==ParseState::ExpectArgs) {
            if (buffer->readableBytes()<context->argsSize) {
                return;
            }
            context->argsBuffer.append(buffer->peek(),context->argsSize);
            buffer->retrieve(context->argsSize);
            processRequest(conn,context->headerBuffer,context->argsBuffer,context->headerSize,context->argsSize);

            context->state=ParseState::ExpectHeaderSize;
            context->headerSize=0;
            context->argsSize=0;
            context->headerBuffer.clear();
            context->argsBuffer.clear();
        }
    }
}


void MprpcProvider::processRequest(const muduo::net::TcpConnectionPtr &conn, const std::string &headerStr, const std::string &argsStr, uint32_t headerSize, uint32_t argsSize) {
    mprpc::RpcHeader rpcHeader;
    if (!rpcHeader.ParseFromString(headerStr)) {
        LOG_ERROR("parse rpc_header error");
        return;
    }

    std::string serviceName=rpcHeader.service_name();
    std::string methodName=rpcHeader.method_name();

    LOG_DEBUG("==================");
    LOG_DEBUG("header_size:%u", headerSize);
    LOG_DEBUG("args_size:%u", argsSize);
    LOG_DEBUG("service_name:%s", serviceName.c_str());
    LOG_DEBUG("method_name:%s", methodName.c_str());
    LOG_DEBUG("==================");

    auto it=serviceMap_.find(serviceName);
    if (it==serviceMap_.end()) {
        LOG_ERROR("rpc service %s not exist",serviceName.c_str());
        return;
    }

    auto mit=it->second.methodMap_.find(methodName);
    if (mit==it->second.methodMap_.end()) {
        LOG_ERROR("rpc service %s method %s not exist",serviceName.c_str(),methodName.c_str());
        return;
    }

    google::protobuf::Service* service=it->second.service_;
    const google::protobuf::MethodDescriptor* method=mit->second;

    std::unique_ptr<google::protobuf::Message> request(service->GetRequestPrototype(method).New());
    std::unique_ptr<google::protobuf::Message> response(service->GetResponsePrototype(method).New());

    if (!request->ParseFromString(argsStr)) {
        LOG_ERROR("request parse error, content:%s", argsStr.c_str());
        return;
    }

    google::protobuf::Closure* done=
        google::protobuf::NewCallback<MprpcProvider,const muduo::net::TcpConnectionPtr&,google::protobuf::Message*>
                                    (this,&MprpcProvider::SendRpcResponse,conn,response.get());

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
