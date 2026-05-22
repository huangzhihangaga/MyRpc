/**
 * @file LogFile.h
 * @brief 日志文件类，负责将日志写入文件并支持文件滚动
 */

#ifndef RPC_LOGFILE_H
#define RPC_LOGFILE_H

#include <string>
#include <stdio.h>

/**
 * @brief 日志文件类，支持按大小滚动和定时刷新
 * @details 按文件大小自动滚动，超过rollSize_创建新文件
 *          定时刷新缓冲区
 *          文件名中制动添加时间戳
 * @note 非线程安全
 */
class LogFile {
public:
    /**
     * @brief 构造函数
     * @param basename 日志文件基础名称
     * @param rollSize 滚动大小，字节，超过后创建新文件，默认100MB
     * @param flushInterval 自动刷新时间，默认3秒
     */
    LogFile(const std::string& basename,size_t rollSize=1024*1024*100,int flushInterval=3);

    /**
     * @brief 析构函数，关闭文件句柄
     */
    ~LogFile();

    /**
     * @brief 追加文件内容
     * @param logline 日志行内容，不自动添加换行符
     * @param len 内容长度
     * @details 写入后，检查是否需要刷新缓冲区
     */
    void append(const char* logline,int len);

    /**
     * @brief 刷新缓冲区，将stdio缓冲区中的数据写入磁盘
     */
    void flush();

    /**
     * @brief 滚动日志文件
     * @return true表示滚动成功，false表示失败
     * @details 关闭当前文件，创建待时间戳的文件
     */
    bool rollFile();

private:
    /**
     * @brief 将日志写入文件
     * @param logline 日志内容
     * @param len 日志长度
     * @details 执行写入，如果写入失败则尝试重新打开文件
     */
    void write(const char* logline,int len);

    /**
     * @brief 生成新的文件日志名
     * @return 格式为 "basename_.YYYYMMDD-HHMMSS.rollCount_.log"
     */
    std::string getLogFileName();

    /// 日志文件基础名
    std::string basename_;

    /// 滚动大小阈值 (字节)
    size_t rollSize_;

    /// 自动刷新时间间隔(秒)
    int flushInterval_;

    /// 上次滚动的时间戳
    time_t lastRoll_;

    /// 上次刷新的时间戳
    time_t lastFlush_;

    /// 同一秒内的滚动计数
    int rollCount_;

    /// 当前使用的时间
    time_t currentRollTime_;

    /// 文件指针
    FILE* fp_;
};


#endif //RPC_LOGFILE_H
