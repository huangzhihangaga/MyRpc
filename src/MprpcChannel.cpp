/**
 * @file MprpcChannel.cpp
 * @brief MprpcChannel类的实现
 * @details 通信协议格式为：
 *  | header_size(4字节) | rpc_header(protobuf序列化，header_size个字节) | args(protobuf序列化，args_size个字节) |
 *  rpc_header => | service_name method_name args_size |
 */

#include "MprpcChannel.h"
#include "rpcheader.pb.h"
#include "MprpcApplication.h"
#include "MprpcController.h"
#include "ZookeeperUtil.h"
#include "Logger.h"
#include "ConnectionPool.h"

#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>

void MprpcChannel::CallMethod(const google::protobuf::MethodDescriptor *method,
                            google::protobuf::RpcController *controller,
                            const google::protobuf::Message *request,
                            google::protobuf::Message *response,
                            google::protobuf::Closure *done)
{
    // 获取服务名和方法名
    const google::protobuf::ServiceDescriptor* serviceDesc=method->service();

    // serviceName 如 UserServiceRpc
    std::string serviceName=serviceDesc->name();

    // methodName 如Login
    std::string methodName=method->name();

    // 获取参数的序列化字符串长度 argsSize
    uint32_t argsSize=0;
    std::string argsStr;
    if (!request->SerializeToString(&argsStr)) {
        controller->SetFailed("serialize request error!");
        LOG_ERROR("serialize request error!");
        return;
    }
    argsSize=argsStr.size();

    // 构造rpc的请求header
    mprpc::RpcHeader rpcHeader;
    rpcHeader.set_service_name(serviceName);
    rpcHeader.set_method_name(methodName);
    rpcHeader.set_args_size(argsSize);

    // 序列化rpc头部
    uint32_t headerSize=0;
    std::string rpcHeaderStr;
    if (!rpcHeader.SerializeToString(&rpcHeaderStr)) {
        controller->SetFailed("serialize rpc_header error!");
        LOG_ERROR("serialize rpc_header error!");
        return;
    }
    headerSize=rpcHeaderStr.size();

    // 打印调试信息
    LOG_DEBUG("======================================");
    LOG_DEBUG("header_size:%lu",headerSize);
    LOG_DEBUG("service_name:%s", serviceName.c_str());
    LOG_DEBUG("method_name:%s", methodName.c_str());
    LOG_DEBUG("args_size:%lu", argsSize);

    // 组装最终要发送的数据包：4字节头部长度 + 头部数据 + 参数数据
    std::string sendRpcStr;
    headerSize=htonl(headerSize);
    sendRpcStr.insert(0,std::string((char*)&headerSize,4));
    sendRpcStr+=rpcHeaderStr;
    sendRpcStr+=argsStr;

    LOG_DEBUG("send_rpc_size size:%d", sendRpcStr.size());
    LOG_DEBUG("======================================");

    // if (send_rpc_str.size()>MprpcApplication::GetMaxMessageSize()) {
    //     char errorBuffer[128] = {0};
    //     snprintf(errorBuffer,sizeof(errorBuffer),"send_rpc_str is too large:%zu ,limit:%zu",send_rpc_str.size(),MprpcApplication::GetMaxMessageSize());
    //     LOG_ERROR("%s",errorBuffer);
    //     controller->SetFailed(errorBuffer);
    //     return;
    // }

    // 连接zookeeper并从zookeeper上获取服务提供者的地址
    ZkClient& zkclient=ZkClient::GetInstance();

    // 例如 /UserServiceRpc/Login
    std::string methodPath="/"+serviceName+"/"+methodName;

    // 获取存储在zookeeper上的服务提供者地址信息"127.0.0.1:8000"
    std::string hostIpPort=zkclient.GetData(methodPath.c_str());
    if (hostIpPort.empty()) {
        LOG_ERROR("method_path:%s address is invalid!", methodPath.c_str());
        controller->SetFailed(methodPath+" address is invalid!");
        return;
    }

    // // 使用tcp编程发送rpc方法的远程调用
    // int clientfd=socket(AF_INET,SOCK_STREAM,0);
    // if (clientfd==-1) {
    //     char errorBuffer[128]={0};
    //     snprintf(errorBuffer,sizeof(errorBuffer),"create socket error:%d",errno);
    //     LOG_ERROR("%s", errorBuffer);
    //     controller->SetFailed(errorBuffer);
    //     return;
    // }
    size_t idx=hostIpPort.find(':');
    if (idx==std::string::npos) {
        LOG_ERROR("method_path:%s address is invalid!", methodPath.c_str());
        controller->SetFailed(methodPath+" address is invalid!");
        return;
    }

    std::string ip=hostIpPort.substr(0,idx);
    uint16_t port=std::stoi(hostIpPort.substr(idx+1,hostIpPort.size()-idx));

    ConnectionPool& pool=ConnectionPool::GetInstance();
    int clientfd=pool.GetConnection(ip,port,3000);

    if (clientfd==-1) {
        char errorBuffer[128];
        snprintf(errorBuffer, sizeof(errorBuffer),"get connection to %s:%d failed or timeout", ip.c_str(), port);
        controller->SetFailed(errorBuffer);
        return;
    }

    // 发送rpc请求
    if (-1==send(clientfd,sendRpcStr.c_str(),sendRpcStr.size(),0)) {
        char errorBuffer[128] = {0};
        snprintf(errorBuffer, sizeof(errorBuffer), "send error: %d", errno);
        LOG_ERROR("%s", errorBuffer);
        controller->SetFailed(errorBuffer);
        pool.ReturnConnection(ip,port,clientfd,false);
        return;
    }

    uint32_t responseLen=0;
    ssize_t recvlen=recv(clientfd,&responseLen,4,MSG_WAITALL);
    if (recvlen!=4) {
        controller->SetFailed("recv response length error");
        LOG_ERROR("recv response length error");
        pool.ReturnConnection(ip,port,clientfd,false);
        return;
    }
    responseLen=ntohl(responseLen);

    // if (responseLen>MprpcApplication::GetMaxMessageSize()) {
    //     char errorBuffer[128] = {0};
    //     snprintf(errorBuffer,sizeof(errorBuffer),"response is too large:%zu ,limit:%zu",send_rpc_str.size(),MprpcApplication::GetMaxMessageSize());
    //     LOG_ERROR("%s",errorBuffer);
    //     controller->SetFailed(errorBuffer);
    //     close(clientfd);
    //     return;
    // }

    std::vector<char> recvBuffer(responseLen);
    size_t received=0;
    while (received<responseLen) {
        ssize_t n=recv(clientfd,recvBuffer.data()+received,responseLen-received,0);
        if (n <= 0) {
            LOG_ERROR("recv failed: n=%zd, errno=%d", n, errno);
            controller->SetFailed("recv response data error");
            pool.ReturnConnection(ip, port, clientfd, false);
            return;
        }
        received+=n;
    }

    if (!response->ParseFromArray(recvBuffer.data(),static_cast<int>(responseLen))) {
        controller->SetFailed("response parse error");
        LOG_ERROR("response parse error");
        pool.ReturnConnection(ip,port,clientfd,false);
        return;
    }

    pool.ReturnConnection(ip,port,clientfd,true);

    if (done!=nullptr) {
        done->Run();
    }
}
