/**
 * @file AsyncLogging.h
 * @brief 异步日志类，基于双缓冲区实现
 */

#ifndef RPC_ASYNCLOGGING_H
#define RPC_ASYNCLOGGING_H

#include "LogFile.h"

#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <memory>
#include <string>
#include <atomic>

/**
 * @brief 异步日志类，基于双缓冲区
 * @details 设计原理：
 *          前端：多个线程调用append()将日志写入当前缓冲区
 *          后端：一个专用线程，负责将缓冲区中的日志写入文件
 *          当当前缓冲区写满时，与备用缓冲区交换，减少锁持有时间
 */
class AsyncLogging {
public:
    /**
     * @brief 构造函数
     * @param basename 日志文件基础名
     * @param rollSize 滚动大小，默认100MB
     * @param flushInterval 刷新间隔，默认3秒
     */
    AsyncLogging(const std::string& basename,size_t rollSize=1024*1024*100,int flushInterval=3);

    /**
     * @brief 析构函数，停止后台日志线程
     */
    ~AsyncLogging();

    /**
     * @brief 追加日志，前端调用
     * @param logline 日志内容
     * @param len 日志长度
     * @details 将日志写入到当前缓冲区，如果缓冲区满则交换到待写入队列
     *          执行流程：
     *          1.加锁保护
     *          2.检查当前缓冲区是否有足够空间
     *          3.如果空间足够，直接写入
     *          4.如果空间不够，将当前缓冲区移入待写入队列
     *          5.尝试使用备用缓冲区作为新的当前缓冲区
     *          6.如果备用缓冲区也在使用，创建新缓冲区
     *          7.写入日志到新缓冲区
     *          8.唤醒后台线程
     */
    void append(const char* logline,int len);

    /**
     * @brief 启动后台日志线程
     */
    void start();

    /**
     * @brief 停止后台日志线程
     * @details 等待所有缓冲区的日志写入完成后停止
     */
    void stop();
private:
    /**
     * @brief 后台线程函数
     * @details 循环执行：
     *          1.等待条件变量或超时
     *          2.交换缓冲区，获取待写入数据
     *          3.将数据写入日志文件
     *          4.刷新文件缓冲区
     */
    void threadFunc();

    /// 缓冲区类型 字符数组
    using Buffer=std::string;

    /// 缓冲区智能指针
    using BufferPtr=std::unique_ptr<Buffer>;

    /// 缓冲区队列类型
    using BufferVector=std::vector<BufferPtr>;

    /// 缓冲区大小 4MB
    static const int kBufferSize=1024*1024*4;

    /// 缓冲区数量
    static const int kBufferNum=2;

    /// 日志文件基础名
    std::string basename_;

    /// 滚动大小
    size_t rollSize_;

    /// 刷新间隔 秒
    int flushInterval_;

    /// 运行标志
    bool running_;

    /// 后台线程
    std::thread thread_;

    /// 互斥锁
    std::mutex mutex_;

    /// 条件变量
    std::condition_variable cond_;

    /// 当前正在写入的缓冲区
    BufferPtr currentBuffer_;

    /// 备用缓冲区
    BufferPtr nextBuffer_;

    /// 待写入文件的缓冲区队列
    BufferVector buffersToWrite_;
};

#endif //RPC_ASYNCLOGGING_H
