/**
 * @file MprpcApplication.cpp
 * @brief rpc框架基础类实现
 */

#include "MprpcApplication.h"
#include <iostream>
#include <unistd.h>
#include <string>
#include "Logger.h"

// MprpcApplication的静态成员变量config_初始化
MprpcConfig MprpcApplication::config_;
bool MprpcApplication::initialized_=false;

/**
 * @internal
 * @brief 输出所需要的命令行格式
 */
static void ShowArgsHelp() {
    std::cout<<"format: command -i <configfile>"<<std::endl;
}

void MprpcApplication::Init(int argc,char** argv) {
    if (initialized_) {
        LOG_WARN("MprpcApplication already initialized");
        return;
    }
    if (argc<2) {
        ShowArgsHelp();
        exit(EXIT_FAILURE);
    }

    int c=0;
    std::string configFile;
    while ((c=getopt(argc,argv,"i:"))!=-1) {
        switch (c) {
            case 'i' :
                // 将配置文件路径保存到config_file
                configFile=optarg;
                break;
            case '?':
                // 出现未知参数
                ShowArgsHelp();
                LOG_FATAL("invalid param");
                break;
            case ':':
                // -i后没有跟参数
                ShowArgsHelp();
                LOG_FATAL("%c missing param",c);
                break;
            default:
                break;
        }
    }

    // 开始加载配置文件 rpcserver_ip=  rpcserver_port=  zookeeper_ip=  zookeeper_port=
    config_.LoadConfigFile(configFile.c_str());
    LOG_INFO("rpcserverip:%s", config_.Load("rpcserverip").c_str());
    LOG_INFO("rpcserverport:%s", config_.Load("rpcserverport").c_str());
    LOG_INFO("zookeeperip:%s", config_.Load("zookeeperip").c_str());
    LOG_INFO("zookeeperport:%s", config_.Load("zookeeperport").c_str());

    initialized_=true;
}

MprpcApplication &MprpcApplication::GetInstance() {
    static MprpcApplication instance;
    return instance;
}

MprpcConfig& MprpcApplication::GetConfig() {
    return config_;
}