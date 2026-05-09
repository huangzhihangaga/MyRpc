#include "test.pb.h"
#include <iostream>
#include <string>

int main1() {
    fixbug::LoginRequest req;
    req.set_name("zhangsan");
    req.set_pwd("123456");

    std::string send_str;
    if (req.SerializeToString(&send_str)) {
        std::cout<<send_str.c_str()<<std::endl;
    }

    fixbug::LoginRequest reqB;
    if (reqB.ParseFromString(send_str)) {
        std::cout<<reqB.name()<<std::endl;
        std::cout<<reqB.pwd()<<std::endl;
    }
    return 0;
}


int main() {
    // fixbug::LoginResponse rsp;
    // fixbug::ResultCode* rc=rsp.mutable_result();
    // rc->set_errcode(0);
    // rc->set_errmsg("fail");

    fixbug::GetFriendListsResponse rsp;
    fixbug::ResultCode* rc=rsp.mutable_result();
    rc->set_errcode(0);

    fixbug::User* user1=rsp.add_friend_list();
    user1->set_name("zhangsan");
    user1->set_age(18);
    user1->set_sex(fixbug::User::MAN);

    fixbug::User* user2=rsp.add_friend_list();
    user2->set_name("zhangsan");
    user2->set_age(18);
    user2->set_sex(fixbug::User::MAN);

    std::cout<<rsp.friend_list_size()<<std::endl;

    

    return 0;
}