/**
 * @file MprpcApplication.h
 * @brief rpc框架基础类接口
 */
#ifndef RPC_MPRPCAPPLICATION_H
#define RPC_MPRPCAPPLICATION_H

#include "MprpcConfig.h"
#include "MprpcChannel.h"
#include "MprpcController.h"

/**
 * @brief rpc框架的基础类，负责框架的一些初始化操作
 * @details 采用单例模式
 */
class MprpcApplication {
public:
    /**
     * @brief 静态成员函数，负责解析命令行参数并加载配置文件，给静态成员变量config_赋值
     * @param argc 来自main的命令行参数数量
     * @param argv 来自main的字符串数组指针
     */
    static void Init(int argc,char** argv);

    /**
     * @brief 静态成员函数，实现单例模式
     * @return 返回唯一单例
     */
    static MprpcApplication& GetInstance();

    /**
     * @brief 获取静态成员变量config_
     * @return 返回静态成员变量MprpcConfig的引用
     */
    static MprpcConfig& GetConfig();

    /**
     * @brief 单例模式删除拷贝构造
     */
    MprpcApplication(const MprpcApplication&)=delete;

    /**
     * @brief 单例模式删除拷贝赋值
     * @return 无
     */
    MprpcApplication& operator=(const MprpcApplication&)=delete;

    /**
     * @brief 单例模式删除移动构造
     */
    MprpcApplication(MprpcApplication&&)=delete;

    /**
     * @brief 单例模式删除移动赋值
     * @return 无
     */
    MprpcApplication& operator=(MprpcApplication&&)=delete;

    /// 默认消息大小限制
    static size_t MaxMessageSize;

    /**
     * @brief 获取消息大小限制
     * @return 消息的最大长度
     * @details 如果配置文件中有消息大小限制选项，则在Init()中赋值一次，否则使用默认值
     */
    static size_t GetMaxMessageSize();

private:
    /// 静态成员变量，负责加载存储配置文件，在MprpcApplication::Init()中赋值
    static MprpcConfig config_;

    /**
     * @brief 默认构造函数
     */
    MprpcApplication()=default;

    /**
     * @brief 默认析构函数
     */
    ~MprpcApplication()=default;

    /**
     * @brief 静态成员变量，表示是否执行过Init()
     */
    static bool initialized_;
};

#endif //RPC_MPRPCAPPLICATION_H
