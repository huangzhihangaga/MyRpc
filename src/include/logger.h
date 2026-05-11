/**
 * @file logger.h
 * @brief 异步日志模块系统
 */

#ifndef RPC_LOGGER_H
#define RPC_LOGGER_H

#include "lockqueue.h"
#include <string>

/**
 * @brief 日志级别枚举
 */
enum LogLevel {
    INFO, ///< 普通日志
    ERROR, ///< 错误日志
};

/**
 * @brief Mprpc框架提供的异步日志系统
 * @details 采用单例模式，内部使用LockQueue作为日志缓冲区，并采用专门的线程将日志异步写入文件
 */
class Logger {
public:
    /**
     * @brief 获取Logger的单例
     * @return 返回Logger的单例引用
     */
    static Logger& GetInstance();

    /**
     * @brief 设置当前日志级别
     * @param level 要设置的日志级别
     */
    void SetLogLevel(LogLevel level);

    /**
     * @brief 把日志信息写入LockQueue缓冲区中
     * @param msg 日志信息
     */
    void Log(std::string msg);

    /**
     * @brief 删除拷贝构造
     */
    Logger(const Logger&) = delete;

    /**
     * @brief 删除移动构造
     */
    Logger(Logger&&)=delete;

    /**
     * @brief 删除拷贝赋值
     */
    Logger& operator=(const Logger&)=delete;

    /**
     * @brief 删除移动赋值
     */
    Logger& operator=(Logger&&)=delete;

private:
    /// 当前日志级别
    int m_loglevel;

    /// 日志消息队列
    LockQueue<std::string> m_lckQue;

    /**
     * @brief 私有构造函数
     */
    Logger();
};

/**
 * @brief 记录INFO级别日志的宏
 * @param logmsgformat 格式化字符串
 * @param ... 可变参数
 */
#define LOG_INFO(logmsgformat,...) \
    do \
    { \
        Logger& logger=Logger::GetInstance(); \
        logger.SetLogLevel(INFO); \
        char c[1024]={0}; \
        snprintf(c,1024,logmsgformat,##__VA_ARGS__); \
        logger.Log(c); \
    }while (0) \

/**
 * @brief 记录ERROR级别日志的宏
 * @param logmsgformat 格式化字符串
 * @param ... 可变参数
 */
#define LOG_ERR(logmsgformat,...) \
    do \
    { \
        Logger& logger=Logger::GetInstance(); \
        logger.SetLogLevel(ERROR); \
        char c[1024]={0}; \
        snprintf(c,1024,logmsgformat,##__VA_ARGS__); \
        logger.Log(c); \
    }while (0) \

#endif //RPC_LOGGER_H
