/**
 * @file mprpcapplication.cpp
 * @brief rpc框架基础类实现
 */

#include "mprpcapplication.h"
#include <iostream>
#include <unistd.h>
#include <string>

// MprpcApplication的静态成员变量m_config初始化
MprpcConfig MprpcApplication::m_config;

// 输出所需要的命令行格式
void ShowArgsHelp() {
    std::cout<<"format: command -i <configfile>"<<std::endl;
}

void MprpcApplication::Init(int argc,char** argv) {
    if (argc<2) {
        ShowArgsHelp();
        exit(EXIT_FAILURE);
    }

    int c=0;
    std::string config_file;
    while ((c=getopt(argc,argv,"i:"))!=-1) {
        switch (c) {
            case 'i' :
                // 将配置文件路径保存到config_file
                config_file=optarg;
                break;
            case '?':
                // 出现未知参数
                ShowArgsHelp();
                exit(EXIT_FAILURE);
                break;
            case ':':
                // -i后没有跟参数
                ShowArgsHelp();
                exit(EXIT_FAILURE);
                break;
            default:
                break;
        }
    }

    // 开始加载配置文件 rpcserver_ip=  rpcserver_port=  zookeeper_ip=  zookeeper_port=
    m_config.LoadConfigFile(config_file.c_str());
    // std::cout<<"rpcserverip:"<<m_config.Load("rpcserverip")<<std::endl;
    // std::cout<<"rpcserverport:"<<m_config.Load("rpcserverport")<<std::endl;
    // std::cout<<"zookeeperip:"<<m_config.Load("zookeeperip")<<std::endl;
    // std::cout<<"zookeeperport:"<<m_config.Load("zookeeperport")<<std::endl;
}

MprpcApplication &MprpcApplication::GetInstance() {
    static MprpcApplication app;
    return app;
}

MprpcConfig& MprpcApplication::GetConfig() {
    return m_config;
}