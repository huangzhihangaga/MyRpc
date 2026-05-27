#include <iostream>
#include "MprpcApplication.h"
#include "friend.pb.h"
#include "Logger.h"

void InitLogger() {
    Logger::setLogLevel(LogLevel::INFO);

    static std::shared_ptr<AsyncLogging> asyncLogger;
    asyncLogger=std::make_shared<AsyncLogging>("FriendClient");
    asyncLogger->start();
    Logger::setAsyncLogging(asyncLogger);
    LOG_INFO("Friend Client Logger Initialized");
}

int main(int argc,char** argv) {
    InitLogger();
    
    MprpcApplication::Init(argc,argv);
    fixbug::FriendServiceRpc_Stub stub(new MprpcChannel());

    fixbug::GetFriendListsRequest request;
    request.set_userid(1000);
    fixbug::GetFriendListsResponse response;
    MprpcController controller;
    stub.GetFriendList(&controller,&request,&response,nullptr);

    // 一次rpc调用完成，读调用的结果
    if (controller.Failed()) {
        std::cout << controller.ErrorText() << std::endl;
    }else {
        if (0==response.result().errcode()) {
            std::cout << "rpc GetFriendList call success!" << std::endl;
            int size=response.friends_size();
            for (int i=0;i<size;i++) {
                std::cout<<"index:"<<i+1<<" name:"<<response.friends(i) << std::endl;
            }
        }else {
            std::cout << "rpc GetFriendList call error:"<<response.result().errmsg() << std::endl;
        }
    }


    return 0;
}