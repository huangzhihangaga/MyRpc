/**
 * @file mprpcconfig.cpp
 * @brief 框架读取配置类实现
 */

#include "mprpcconfig.h"

#include <iostream>
#include <string>
#include <memory>

// 自定义FILE的unique_ptr的删除器
auto file_deleter=[](FILE* fp) {
    if (fp) fclose(fp);
};

void MprpcConfig::LoadConfigFile(const char *config_file) {
    std::unique_ptr<FILE,decltype(file_deleter)> fp(fopen(config_file,"r"),file_deleter);
    if (fp==nullptr) {
        // std::cout<<config_file<<" is not exist!"<<std::endl;
        exit(EXIT_FAILURE);
    }

    char buf[1024]={0};
    while (fgets(buf,1024,fp.get())!=nullptr) {
        // 去掉字符串前后的空格\n\r\t
        std::string read_buf(buf);
        Trim(read_buf);

        // 去除无效信息后本行为空或本行为注释#
        if (read_buf.empty() || read_buf[0]=='#') {
            continue;
        }

        // 解析配置项
        size_t idx=read_buf.find('=');
        if (idx==std::string::npos) {
            continue;
        }
        std::string key;
        std::string value;
        key=read_buf.substr(0,idx);
        Trim(key);
        value=read_buf.substr(idx+1);
        Trim(value);
        m_configMap.insert({key,value});
    }
}

std::string MprpcConfig::Load(const std::string& key) {
    auto it=m_configMap.find(key);
    if (it==m_configMap.end()) {
        return "";
    }
    return it->second;
}

void MprpcConfig::Trim(std::string &src_buf) {
    size_t begin = src_buf.find_first_not_of(" \t\r\n");
    if (begin == std::string::npos) {
        src_buf.clear();
        return;
    }

    size_t end = src_buf.find_last_not_of(" \t\r\n");
    src_buf = src_buf.substr(begin, end - begin + 1);
}


