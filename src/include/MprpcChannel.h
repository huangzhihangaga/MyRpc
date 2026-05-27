/**
 * @file MprpcChannel.h
 * @brief rpc通信通道类，负责rpc请求的序列化、发送和响应接收
 */

#ifndef RPC_MPRPCCHANNEL_H
#define RPC_MPRPCCHANNEL_H

#include <google/protobuf/service.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>

/**
 * @brief rpc通信通道类
 * @details 继承自google::protobuf::RpcChannel，重写其CallMethod方法
 *          所有通过stub代理对象调用的tpc方法都会走到这里
 *          统一处理序列化、网络传输和反序列化
 */
class MprpcChannel : public google::protobuf::RpcChannel {
public:
    /**
     * @brief 重写google::protobuf::RpcChannel的CallMethod方法
     * @param method 被调用的方法描述符
     * @param controller rpc控制器，用于设置错误信息等
     * @param request 请求消息对象
     * @param response 响应消息对象
     * @param done 回调
     * @details 执行流程
     *          1.序列化请求参数和rpc头信息
     *          2.通过zookeeper查询服务地址
     *          3.建立TCP连接
     *          4.发送请求
     *          5.接收响应数据
     *          6.反序列化响应
     * @note 如果过程中发生错误，会通过controller->SetFailed()设置错误信息
     */
    void CallMethod(const google::protobuf::MethodDescriptor *method,
                    google::protobuf::RpcController *controller,
                    const google::protobuf::Message *request,
                    google::protobuf::Message *response,
                    google::protobuf::Closure *done) override;
};

#endif //RPC_MPRPCCHANNEL_H