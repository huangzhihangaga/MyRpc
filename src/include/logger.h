//
// Created by 小晓 on 2026/4/28.
//

#ifndef RPC_LOGGER_H
#define RPC_LOGGER_H

#include "lockqueue.h"
#include <string>

enum LogLevel {
    INFO, // 普通信息
    ERROR, // 错误信息
};
/**
 * @brief Mprpc框架提供的日志系统
 */
class Logger {
public:
    /**
     * @brief 获取日志的单例
     * @return 返回日志的单例引用
     */
    static Logger& GetInstance();

    /**
     * @brief 设置日志级别
     * @param level 要设置的日志级别
     */
    void SetLogLevel(LogLevel level);

    /**
     * @brief 写日志，把日志信息写入lockqueue缓冲区中
     * @param msg 写入的日志信息
     */
    void Log(std::string msg);

    Logger(const Logger&) = delete;
    Logger(Logger&&)=delete;
    Logger& operator=(const Logger&)=delete;
    Logger& operator=(Logger&&)=delete;

private:
    /// 记录日志级别
    int m_loglevel;
    LockQueue<std::string> m_lckQue;

    Logger();

};

// 定义日志宏
#define LOG_INFO(logmsgformat,...) \
    do \
    { \
        Logger& logger=Logger::GetInstance(); \
        logger.SetLogLevel(INFO); \
        char c[1024]={0}; \
        snprintf(c,1024,logmsgformat,##__VA_ARGS__); \
        logger.Log(c); \
    }while (0) \

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
