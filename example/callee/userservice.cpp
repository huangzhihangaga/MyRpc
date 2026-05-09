#include <iostream>
#include <string>
#include "user.pb.h"
#include "mprpcapplication.h"
#include "rpcprovider.h"

/*
 * UserService是本地服务类，提供进程内的本地方法 使用在rpc服务的发布端（rpc服务提供者）
 */
class UserService :public fixbug::UserServiceRpc{
public:
    /*
     * @brief Login方法的本地实现
     * @param name 参数一
     * @param pwd 参数二
     * @return 返回值
     */
    bool Login(std::string name,std::string pwd) {
        std::cout<<"doing local service : Login"<<std::endl;
        std::cout<<"name: "<<name<<" pwd: "<<pwd<<std::endl;
        return true;
    }

    /*
     * @brief Register方法的到本地实现
     * @param id 参数一
     * @param name 参数二
     * @param pwd 参数三
     * @return 返回值
     */
    bool Register(uint32_t id,std::string name,std::string pwd) {
        std::cout<<"doing local service: Register"<<std::endl;
        std::cout<<"id:"<<id<<" name:"<<name<<" pwd:"<<pwd<<std::endl;
        return true;
    }

    /*
     * 重写UserServiceRpc的虚函数 下面这些方法都是框架直接调用的
     * 1.caller  ===>  Login(LoginRequest)  =>  muduo  =>  callee
     * 2.callee  ===>  Login(LoginRequest)  => 交到重写的Login方法上
     */
    void Login(google::protobuf::RpcController *controller,
        const fixbug::LoginRequest *request,
        fixbug::LoginResponse *response,
        google::protobuf::Closure *done) override
    {
        // 框架给业务上报了请求参数LoginRequest，业务获取相应数据做本地业务
        std::string name = request->name();
        std::string pwd = request->pwd();

        bool login_result=Login(name,pwd); // 做本地业务

        // 把相应写入
        fixbug::ResultCode* code=response->mutable_result();
        code->set_errcode(0);
        code->set_errmsg("");
        response->set_success(login_result);

        // 执行回调操作 执行响应对象数据的序列化和网络发送 （由框架来完成）
        done->Run();
    }

    // 框架已经从网络上接收到了请求的数据并把它反序列化为request
    void Register(google::protobuf::RpcController* controller,
                       const ::fixbug::RegisterRequest* request,
                       ::fixbug::RegisterResponse* response,
                       ::google::protobuf::Closure* done) override
    {
        uint32_t id=request->id();
        std::string name = request->name();
        std::string pwd = request->pwd();

        bool ret=Register(id,name,pwd);

        response->mutable_result()->set_errcode(0);
        response->mutable_result()->set_errmsg("");
        response->set_success(ret);

        // 把response序列化并通过网络发给rpc client
        done->Run();
    }
};

int main(int argc,char** argv) {

    // 调用框架的初始化操作
    MprpcApplication::Init(argc,argv);

    // provider是一个rpc网路服务对象，把UserService对象发布到rpc节点上
    RpcProvider provider;
    provider.NotifyService(new UserService());

    // 启动与rpc服务发布节点，Run以后，进程进入阻塞状态，等待远端的rpc调用请求
    provider.Run();

    return 0;
}

