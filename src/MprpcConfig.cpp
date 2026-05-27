/**
 * @file MprpcConfig.cpp
 * @brief 框架读取配置类实现
 */

#include "MprpcConfig.h"
#include "Logger.h"
#include <iostream>
#include <string>
#include <memory>

// 自定义FILE的unique_ptr的删除器
auto fileDeleter=[](FILE* fp) {
    if (fp) fclose(fp);
};

void MprpcConfig::LoadConfigFile(const char *configFile) {
    std::unique_ptr<FILE,decltype(fileDeleter)> fp(fopen(configFile,"r"),fileDeleter);
    if (fp==nullptr) {
        LOG_ERROR("config file %s is not exist!", configFile);
        exit(EXIT_FAILURE);
    }

    char buf[1024]={0};
    while (fgets(buf,1024,fp.get())!=nullptr) {
        // 去掉字符串前后的空格\n\r\t
        std::string readBuf(buf);
        Trim(readBuf);

        // 去除无效信息后本行为空或本行为注释(以#开头)
        if (readBuf.empty() || readBuf[0]=='#') {
            continue;
        }

        // 解析配置项
        size_t idx=readBuf.find('=');
        if (idx==std::string::npos) {
            continue;
        }
        std::string key;
        std::string value;
        key=readBuf.substr(0,idx);
        Trim(key);
        value=readBuf.substr(idx+1);
        Trim(value);
        configMap_.insert({key,value});
    }
}

std::string MprpcConfig::Load(const std::string& key) {
    auto it=configMap_.find(key);
    if (it==configMap_.end()) {
        return "";
    }
    return it->second;
}

void MprpcConfig::Trim(std::string &src) {
    size_t begin = src.find_first_not_of(" \t\r\n");
    if (begin == std::string::npos) {
        src.clear();
        return;
    }

    size_t end = src.find_last_not_of(" \t\r\n");
    src = src.substr(begin, end - begin + 1);
}


