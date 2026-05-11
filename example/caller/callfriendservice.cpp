#include <iostream>
#include "mprpcapplication.h"
#include "friend.pb.h"

int main(int argc,char** argv) {
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