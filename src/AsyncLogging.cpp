/**
 * @file AsyncLogging.cpp
 * @brief AsyncLogging类的实现
 */

#include "AsyncLogging.h"

#include <algorithm>
#include <cstring>

AsyncLogging::AsyncLogging(const std::string &basename, size_t rollSize, int flushInterval)
    :basename_(basename)
    ,rollSize_(rollSize)
    ,flushInterval_(flushInterval)
    ,running_(false)
    ,currentBuffer_(new Buffer)
    ,nextBuffer_(new Buffer)
    ,buffersToWrite_()
    ,mutex_()
    ,cond_()
{
    currentBuffer_->reserve(kBufferSize);
    nextBuffer_->reserve(kBufferSize);
}

AsyncLogging::~AsyncLogging() {
    stop();
}

void AsyncLogging::start() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (running_) return;
    running_=true;
    thread_=std::thread(&AsyncLogging::threadFunc,this);
}

void AsyncLogging::stop() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!running_) return;
        running_=false;
    }
    cond_.notify_one();
    if (thread_.joinable()) {
        thread_.join();
    }
}

void AsyncLogging::append(const char *logline, int len) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!running_) return;

    if (currentBuffer_->size() + static_cast<size_t>(len)<=currentBuffer_->capacity()) {
        currentBuffer_->append(logline,len);
    }else {
        buffersToWrite_.push_back(std::move(currentBuffer_));

        if (nextBuffer_) {
            currentBuffer_=std::move(nextBuffer_);
            nextBuffer_.reset(new Buffer);
            nextBuffer_->reserve(kBufferSize);
        }else {
            currentBuffer_.reset(new Buffer);
            currentBuffer_->reserve(kBufferSize);
        }
        currentBuffer_->append(logline,len);

        cond_.notify_one();
    }
}

void AsyncLogging::threadFunc() {
    LogFile logFile(basename_,rollSize_,flushInterval_);

    BufferPtr newBuffer1(new Buffer);
    newBuffer1->reserve(kBufferSize);
    BufferPtr newBuffer2(new Buffer);
    newBuffer2->reserve(kBufferSize);

    BufferVector buffersToWrite;

    while (running_) {
        {
            std::unique_lock<std::mutex> lock(mutex_);

            cond_.wait_for(lock,std::chrono::seconds(flushInterval_),[this]() {
                return !buffersToWrite_.empty() || !running_;
            });

            buffersToWrite_.push_back(std::move(currentBuffer_));
            currentBuffer_=std::move(newBuffer1);
            buffersToWrite.swap(buffersToWrite_);
            if (!nextBuffer_) {
                nextBuffer_=std::move(newBuffer2);
            }
        }

        if (buffersToWrite.empty()) {
            continue;
        }

        for (const auto& buffer:buffersToWrite) {
            if (buffer && !buffer->empty()) {
                logFile.append(buffer->c_str(),static_cast<int>(buffer->size()));
            }
        }

        if (buffersToWrite.size()>kBufferNum) {
            buffersToWrite.resize(kBufferNum);
        }

        if (!newBuffer1 && !buffersToWrite.empty()) {
            newBuffer1=std::move(buffersToWrite.back());
            buffersToWrite.pop_back();
            newBuffer1->clear();
        }

        if (!newBuffer2 && !buffersToWrite.empty()) {
            newBuffer2=std::move(buffersToWrite.back());
            buffersToWrite.pop_back();
            newBuffer2->clear();
        }

        buffersToWrite.clear();
        logFile.flush();
    }

    // 退出时将当前缓冲区中的内容写入
    for (const auto& buffer:buffersToWrite_) {
        if (buffer && !buffer->empty()) {
            logFile.append(buffer->c_str(),static_cast<int>(buffer->size()));
        }
    }
    if (currentBuffer_ && !currentBuffer_->empty()) {
        logFile.append(currentBuffer_->c_str(),static_cast<int>(currentBuffer_->size()));
    }
}
