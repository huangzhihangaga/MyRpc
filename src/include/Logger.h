/**
 * @file Logger.h
 * @brief 日志记录器类，支持同步或异步输出
 */

#ifndef RPC_LOGGER_H
#define RPC_LOGGER_H

#include "AsyncLogging.h"

#include <memory>
#include <string>
#include <cstdio>
#include <vector>

/**
 * @enum LogLevel
 * @brief 日志级别枚举
 * @details 只有级别高于或等于当前级别的日志才会输出
 */
enum class LogLevel {
    TRACE=0,            ///< 跟踪级别
    DEBUG=1,            ///< 调试级别
    INFO=2,             ///< 详细级别
    WARN=3,             ///< 警告级别
    ERROR=4,            ///< 错误级别
    FATAL=5,          ///< 致命错误级别
    NUM_LOG_LEVELS      ///< 日志级别总数
};

/**
 * @brief 日志记录器，提供静态日志输出接口
 * @details 支持自动扩容，可以处理超长日志消息
 *          FATAL级别的日志会刷新缓冲区并终止程序
 *          使用可变参数模板实现类型安全的格式化输出
 *          支持同步(默认)和异步输出
 */
class Logger {
public:
    /**
     * @brief 设置全局日志级别
     * @param level 日志级别
     * @details 低于设置的日志级别的日志不会输出
     */
    static void setLogLevel(LogLevel level);

    /**
     * @brief 获取当前日志级别
     * @return 当前日志级别
     */
    static LogLevel getLogLevel();

    /**
     * @brief 设置异步日志后端
     * @param asyncLogging 异步日志对象指针
     * @details 调用此方法后，日志将输出到指定的异步日志系统
     *          asyncLogging_为nullptr时使用同步输出
     */
    static void setAsyncLogging(std::shared_ptr<AsyncLogging> asyncLogging);

    /**
     * @brief 输出日志，无格式化参数版本
     * @param level 日志级别
     * @param file 文件名 通过__FILE__传递
     * @param line 行号 通过__LINE__传递
     * @param func 函数名 通过__FUNCTION__传递
     * @param message 日志消息
     */
    static void log(LogLevel level,const char* file,int line,const char* func,const char* message);

    /**
     * @brief 输出日志
     * @tparam Args 可变参数
     * @param level 日志级别
     * @param file 文件名
     * @param line 行号
     * @param func 函数名
     * @param format 格式化字符串
     * @param args 格式化参数
     * @details 支持任意长度的日志消息，内部会自动分配足够大的缓冲区
     */
    template<typename... Args>
    static void log(LogLevel level,const char* file,int line,const char* func,const char* format,Args&&... args) {
        std::string header;
        formatHeader(header,file,line,level,func);

        std::string message=formatString(format,std::forward<Args>(args)...);
        std::string buffer=header+message+"\n";
        output(buffer.c_str(),static_cast<int>(buffer.length()));

        if (level==LogLevel::FATAL) {
            flush();
            exit(-1);
        }
    }

    /**
     * @brief 刷新日志缓冲区
     */
    static void flush();
private:
    /**
     * @brief 默认输出函数
     * @param msg 日志消息
     * @param len 日志长度
     * @details 输出到标准输出并立即刷新
     */
    static void defaultOutput(const char* msg,int len);

    /**
     * @brief 默认刷新函数
     * @details 刷新标准输出缓冲区
     */
    static void defaultFlush();

    /**
     * @brief 输出日志消息
     * @param msg 日志消息
     * @param len 消息长度
     * @details 根据asyncLogging_是否为空，选择同步或异步输出
     */
    static void output(const char* msg,int len);

    /**
     * @brief 格式化日志头部
     * @param buffer 输出缓冲区
     * @param file 源文件名
     * @param line 行号
     * @param level 日志级别
     * @param func 函数名
     * @details 输出格式："2026-05-21 12:34:56.123456 [INFO] Logger.cpp:42 log() "
     */
    static void formatHeader(std::string& buffer,const char* file,int line,LogLevel level,const char* func);

    /**
     * @brief 格式化字符串
     * @tparam Args 可变参数
     * @param format 格式化字符串
     * @param args 格式化参数
     * @return 格式化后的字符串
     * @details 流程：
     *          1.使用256字节大小的初始缓冲区
     *          2.snprintf尝试格式化
     *          3.如果缓冲区不足，根据written进行扩容，并再次执行snprintf
     */
    template<typename... Args>
    static std::string formatString(const char* format,Args&&... args) {
        size_t bufferSize=256;
        std::vector<char> buffer(bufferSize);

        while (true) {
            int written=snprintf(buffer.data(),bufferSize,format,std::forward<Args>(args)...);
            if (written<0) {
                return "";
            }

            if (static_cast<size_t>(written)<bufferSize) {
                return std::string(buffer.data(),written);
            }

            bufferSize=static_cast<size_t>(written)+1;
            buffer.resize(bufferSize);
        }
    }

    /// 全局日志级别
    static LogLevel logLevel_;

    /// 异步日志后端
    static std::shared_ptr<AsyncLogging> asyncLogging_;
};

/**
 * @def LOG_TRACE
 * @brief 输出TRACE级别日志
 * @param format 格式化字符串
 * @param ... 可变参数
 * @details 仅在当前日志级别小于等于TRACE时输出
 */
#define LOG_TRACE(format,...) \
    do{ \
        if(Logger::getLogLevel()<=LogLevel::TRACE) \
            Logger::log(LogLevel::TRACE,__FILE__,__LINE__,__FUNCTION__,format,##__VA_ARGS__); \
    }while(0)

/**
 * @def LOG_DEBUG
 * @brief 输出DEBUG级别日志
 * @param format 格式化字符串
 * @param ... 可变参数
 * @details 仅在当前日志级别小于等于DEBUG时输出
 */
#define LOG_DEBUG(format,...) \
    do{ \
        if((Logger::getLogLevel() <= LogLevel::DEBUG)) \
            Logger::log(LogLevel::DEBUG,__FILE__,__LINE__,__FUNCTION__,format,##__VA_ARGS__); \
    }while(0)


/**
 * @def LOG_INFO
 * @brief 输出INFO级别日志
 * @param format 格式化字符串
 * @param ... 可变参数
 * @details 仅在当前日志级别小于等于INFO时输出
 */
#define LOG_INFO(format,...) \
    do{ \
        if((Logger::getLogLevel() <= LogLevel::INFO)) \
            Logger::log(LogLevel::INFO,__FILE__,__LINE__,__FUNCTION__,format,##__VA_ARGS__); \
    }while(0)

/**
 * @def LOG_WARN
 * @brief 输出WARN级别日志
 * @param format 格式化字符串
 * @param ... 可变参数
 * @details 仅在当前日志级别小于等于WARN时输出
 */
#define LOG_WARN(format,...) \
    do{ \
        if((Logger::getLogLevel() <= LogLevel::WARN)) \
            Logger::log(LogLevel::WARN,__FILE__,__LINE__,__FUNCTION__,format,##__VA_ARGS__); \
    }while(0)

/**
 * @def LOG_ERROR
 * @brief 输出ERROR级别日志
 * @param format 格式化字符串
 * @param ... 可变参数
 * @details 仅在当前日志级别小于等于ERROR时输出
 */
#define LOG_ERROR(format,...) \
    do{ \
        if((Logger::getLogLevel() <= LogLevel::ERROR)) \
            Logger::log(LogLevel::ERROR,__FILE__,__LINE__,__FUNCTION__,format,##__VA_ARGS__); \
    }while(0)

/**
 * @def LOG_FATAL
 * @brief 输出FATAL级别日志
 * @param format 格式化字符串
 * @param ... 可变参数
 * @details 仅在当前日志级别小于等于FATAL时输出
 */
#define LOG_FATAL(format,...) \
    do{ \
        if((Logger::getLogLevel() <= LogLevel::FATAL)) \
            Logger::log(LogLevel::FATAL,__FILE__,__LINE__,__FUNCTION__,format,##__VA_ARGS__); \
    }while(0)

#endif //RPC_LOGGER_H
