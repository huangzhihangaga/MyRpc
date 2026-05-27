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

    // service_name 如 UserServiceRpc
    std::string serviceName=serviceDesc->name();

    // method_name 如Login
    std::string methodName=method->name();

    // 获取参数的序列化字符串长度 args_size
    uint32_t args_size=0;
    std::string args_str;
    if (!request->SerializeToString(&args_str)) {
        controller->SetFailed("serialize request error!");
        LOG_ERROR("serialize request error!");
        return;
    }
    args_size=args_str.size();

    // 构造rpc的请求header
    mprpc::RpcHeader rpcHeader;
    rpcHeader.set_service_name(serviceName);
    rpcHeader.set_method_name(methodName);
    rpcHeader.set_args_size(args_size);

    // 序列化rpc头部
    uint32_t header_size=0;
    std::string rpc_header_str;
    if (!rpcHeader.SerializeToString(&rpc_header_str)) {
        controller->SetFailed("serialize rpc_header error!");
        LOG_ERROR("serialize rpc_header error!");
        return;
    }
    header_size=rpc_header_str.size();

    // 组装最终要发送的数据包：4字节头部长度 + 头部数据 + 参数数据
    std::string send_rpc_str;
    header_size=htonl(header_size);
    send_rpc_str.insert(0,std::string((char*)&header_size,4));
    send_rpc_str+=rpc_header_str;
    send_rpc_str+=args_str;

    // 打印调试信息
    LOG_DEBUG("======================================");
    LOG_DEBUG("header_size:%lu",header_size);
    LOG_DEBUG("service_name:%s", serviceName.c_str());
    LOG_DEBUG("method_name:%s", methodName.c_str());
    LOG_DEBUG("args_size:%lu", args_size);
    LOG_DEBUG("send_rpc_size size:%d", send_rpc_str.size());
    LOG_DEBUG("======================================");

    // 使用tcp编程发送rpc方法的远程调用
    int clientfd=socket(AF_INET,SOCK_STREAM,0);
    if (clientfd==-1) {
        char errorBuffer[128]={0};
        snprintf(errorBuffer,sizeof(errorBuffer),"create socket error:%d",errno);
        LOG_ERROR("create socket failed: %s", errorBuffer);
        controller->SetFailed(errorBuffer);
        return;
    }

    // 连接zookeeper并从zookeeper上获取服务提供者的地址
    ZkClient zkclient;
    zkclient.Start();

    // 例如 /UserServiceRpc/Login
    std::string methodPath="/"+serviceName+"/"+methodName;

    // 获取存储在zookeeper上的服务提供者地址信息"127.0.0.1:8000"
    std::string host_data=zkclient.GetData(methodPath.c_str());
    if (host_data.empty()) {
        LOG_ERROR("method_path:%s address is invalid!", methodPath.c_str());
        controller->SetFailed(methodPath+" address is invalid!");
        close(clientfd);
        return;
    }

    size_t idx=host_data.find(':');
    if (idx==std::string::npos) {
        LOG_ERROR("method_path:%s address is invalid!", methodPath.c_str());
        controller->SetFailed(methodPath+" address is invalid!");
        close(clientfd);
        return;
    }

    std::string ip=host_data.substr(0,idx);
    uint16_t port=std::stoi(host_data.substr(idx+1,host_data.size()-idx));
    
    struct sockaddr_in serverAddr;
    serverAddr.sin_family=AF_INET;
    serverAddr.sin_port=htons(port);
    // server_addr.sin_addr.s_addr=inet_addr(ip.c_str());
    if (inet_pton(AF_INET,ip.c_str(),&serverAddr.sin_addr)<=0) {
        char errorBuffer[128] = {0};
        snprintf(errorBuffer, sizeof(errorBuffer), "inet_pton error for ip:%s", ip.c_str());
        LOG_ERROR("%s", errorBuffer);
        controller->SetFailed(errorBuffer);
        close(clientfd);
        return;
    }

    // 连接rpc服务节点
    if (-1==connect(clientfd,(struct sockaddr*)&serverAddr,sizeof(serverAddr))) {
        char errorBuffer[128] = {0};
        snprintf(errorBuffer, sizeof(errorBuffer), "connect to %s:%d error: %d",ip.c_str(), port, errno);
        LOG_ERROR("%s", errorBuffer);
        controller->SetFailed(errorBuffer);
        close(clientfd);
        return;
    }

    // 发送rpc请求
    if (-1==send(clientfd,send_rpc_str.c_str(),send_rpc_str.size(),0)) {
        char errorBuffer[128] = {0};
        snprintf(errorBuffer, sizeof(errorBuffer), "send error: %d", errno);
        LOG_ERROR("%s", errorBuffer);
        controller->SetFailed(errorBuffer);
        close(clientfd);
        return;
    }

    uint32_t responseLen=0;
    ssize_t recvlen=recv(clientfd,&responseLen,4,MSG_WAITALL);
    if (recvlen!=4) {
        controller->SetFailed("recv response length error");
        LOG_ERROR("recv response length error");
        close(clientfd);
        return;
    }
    responseLen=ntohl(responseLen);
    std::vector<char> recvBuffer(responseLen);
    size_t received=0;
    while (received<responseLen) {
        ssize_t n=recv(clientfd,recvBuffer.data()+received,responseLen-received,0);
        if (n<=0) {
            controller->SetFailed("recv response data error");
            LOG_ERROR("recv response data error");
            close(clientfd);
            return;
        }
        received+=n;
    }

    if (!response->ParseFromArray(recvBuffer.data(),responseLen)) {
        controller->SetFailed("response parse error");
        LOG_ERROR("response parse error");
        close(clientfd);
        return;
    }

    close(clientfd);

    if (done!=nullptr) {
        done->Run();
    }
}
