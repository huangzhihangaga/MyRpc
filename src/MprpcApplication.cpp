/**
 * @file mprpcapplication.cpp
 * @brief rpc框架基础类实现
 */

#include "MprpcApplication.h"
#include <iostream>
#include <unistd.h>
#include <string>
#include "Logger.h"

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
    LOG_INFO("rpcserverip:%s", m_config.Load("rpcserverip").c_str());
    LOG_INFO("rpcserverport:%s", m_config.Load("rpcserverport").c_str());
    LOG_INFO("zookeeperip:%s", m_config.Load("zookeeperip").c_str());
    LOG_INFO("zookeeperport:%s", m_config.Load("zookeeperport").c_str());
}

MprpcApplication &MprpcApplication::GetInstance() {
    static MprpcApplication app;
    return app;
}

MprpcConfig& MprpcApplication::GetConfig() {
    return m_config;
}