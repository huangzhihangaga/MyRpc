#include <iostream>
#include <string>
#include "friend.pb.h"
#include <MprpcApplication.h>
#include "MprpcProvider.h"
#include <vector>
#include "Logger.h"
#include "AsyncLogging.h"

class FriendService :public fixbug::FriendServiceRpc{
public:
    std::vector<std::string> GetFriendList(uint32_t userid) {
        LOG_INFO("do GetFriendList service! userid:%u", userid);
        std::vector<std::string> vec;
        vec.push_back("gao yan");
        vec.push_back("liu hong");
        vec.push_back("wang shuo");
        return vec;
    }

    // 重写基类方法
    void GetFriendList(google::protobuf::RpcController *controller,
                        const fixbug::GetFriendListsRequest *request,
                        fixbug::GetFriendListsResponse *response,
                        google::protobuf::Closure *done) override
    {
        uint32_t userid = request->userid();

        std::vector<std::string> friendList=GetFriendList(userid);
        response->mutable_result()->set_errcode(0);
        response->mutable_result()->set_errmsg("");
        for (std::string& name:friendList) {
            std::string* p=response->add_friends();
            *p=name;
        }
        done->Run();
    }
};

void InitLogger() {
    Logger::setLogLevel(LogLevel::INFO);

    static std::shared_ptr<AsyncLogging> asyncLogger;
    asyncLogger=std::make_shared<AsyncLogging>("FriendService");
    asyncLogger->start();
    Logger::setAsyncLogging(asyncLogger);
    LOG_INFO("Friend Service Logger Initialized");
}

int main(int argc,char** argv) {
    InitLogger();

    MprpcApplication::Init(argc,argv);
    LOG_INFO("RPC Framework initialized");

    MprpcProvider provider;
    provider.NotifyService(new FriendService());
    LOG_INFO("Friend Service registered successfully");

    provider.Run();
    return 0;
}