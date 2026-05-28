#include <iostream>
#include "MprpcApplication.h"
#include "user.pb.h"
#include "MprpcChannel.h"
#include "Logger.h"

// 自定义简单的回调函数
void LoginCallback() {
    std::cout<<"DONE IN CALLBACK"<<std::endl;
}

void InitLogger() {
    Logger::setLogLevel(LogLevel::DEBUG);

    static std::shared_ptr<AsyncLogging> asyncLogger;
    asyncLogger=std::make_shared<AsyncLogging>("UserServiceClient");
    asyncLogger->start();
    Logger::setAsyncLogging(asyncLogger);
    LOG_INFO("User Client Logger Initialized");
}

int main(int argc,char** argv) {
    InitLogger();

    // 整个程序启动后，想使用mprpc框架来享受服务调用，一定需要先调用框架的初始化函数（只初始化一次）
    MprpcApplication::Init(argc,argv);
    fixbug::UserServiceRpc_Stub stub(new MprpcChannel());

    // rpc方法的请求参数
    fixbug::LoginRequest request;
    request.set_name("zhangsan");
    request.set_pwd("123456");

    // rpc方法的响应参数
    fixbug::LoginResponse response;

    // 创建控制器对象，用于处理rpc过程调用中的错误
    MprpcController controller;

    google::protobuf::Closure* done=google::protobuf::NewCallback(&LoginCallback);

    // 发起rpc方法的调用 同步rpc调用过程 MprpcChannel::CallMethod
    // CallMethod(const MethodDescriptor* method,
    //                  RpcController* controller,
    //                  const Message* request,
    //                  Message* response,
    //                  Closure* done) = 0;
    stub.Login(&controller,&request,&response,done);

    if (controller.Failed()) {
        std::cout<<controller.ErrorText()<<std::endl;
    }else {
        // 一次rpc调用完成，读调用的结果
        if (response.result().errcode()==0) {
            std::cout<<"rpc login response success:"<<response.success()<<std::endl;
        }else {
            std::cout<<"rpc login response error:"<<response.result().errmsg()<<std::endl;
        }
    }

    // 演示远程调用发布的rpc方法Register
    fixbug::RegisterRequest req;
    req.set_id(2000);
    req.set_name("mprpc");
    req.set_pwd("666666");
    fixbug::RegisterResponse rsp;
    MprpcController controller2;

    // 以同步的方式发起rpc调用请求，等待返回结果
    stub.Register(&controller2,&req,&rsp,nullptr);
    if (0==rsp.result().errcode()) {
        std::cout<<"rpc register response success:"<<rsp.success()<<std::endl;
    }else {
        std::cout<<"rpc register response error:"<<rsp.result().errmsg()<<std::endl;
    }

    return 0;
}