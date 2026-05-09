/**
 * @file mprpcconfig.h
 * @brief 框架读取配置类接口
 */

#ifndef RPC_MPRPCCONFIG_H
#define RPC_MPRPCCONFIG_H

#include <unordered_map>
#include <string>

/**
 * @brief 框架读取配置文件类
 */
class MprpcConfig {
public:
    /**
     * @brief 负责解析加载配置文件
     * @param config_file 配置文件路径
     */
    void LoadConfigFile(const char* config_file);

    /**
     * @brief 查询配置项信息
     * @param key 要查询的配置项
     * @return 该配置项在配置文件中对应的值
     */
    std::string Load(const std::string& key);

private:
    /// 存储配置项及其对应的值
    std::unordered_map<std::string,std::string> m_configMap;

    /**
     * @brief 去除字符串前后的 空格\r\t\n
     * @param src_buf 要进行处理的字符串
     */
    void Trim(std::string& src_buf);
};

#endif //RPC_MPRPCCONFIG_H
