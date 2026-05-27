/**
 * @file Logger.cpp
 * @brief Logger类的实现
 */

#include "Logger.h"

#include <cstring>
#include <cstdio>
#include <ctime>
#include <sys/time.h>
#include <cstdlib>
#include <unistd.h>

LogLevel Logger::logLevel_=LogLevel::INFO;

std::shared_ptr<AsyncLogging> Logger::asyncLogging_;

// 日志级别对应的名称，索引与LogLevel枚举值一一对应
static const char* LogLevelName[static_cast<int>(LogLevel::NUM_LOG_LEVELS)]={
    "TRACE",
    "DEBUG",
    "INFO",
    "WARN",
    "ERROR",
    "FATAL"
};

void Logger::setLogLevel(LogLevel level) {
    logLevel_=level;
}

LogLevel Logger::getLogLevel() {
    return logLevel_;
}

void Logger::setAsyncLogging(std::shared_ptr<AsyncLogging> asyncLogging) {
    asyncLogging_=asyncLogging;
}

void Logger::log(LogLevel level, const char *file, int line, const char *func, const char *message) {
    std::string buffer;
    formatHeader(buffer,file,line,level,func);
    buffer.append(message);
    buffer.append("\n");
    output(buffer.c_str(),static_cast<int>(buffer.length()));

    if (level==LogLevel::FATAL) {
        flush();
        exit(-1);
    }
}


void Logger::defaultOutput(const char* msg,int len) {
    fwrite(msg,1,len,stdout);
    fflush(stdout);
}


void Logger::defaultFlush() {
    fflush(stdout);
}

void Logger::output(const char *msg, int len) {
    if (asyncLogging_) {
        asyncLogging_->append(msg,len);
    }else {
        defaultOutput(msg,len);
    }
}

void Logger::flush() {
    if (asyncLogging_) {
        // 此处有asyncLogging_的后台线程处理
    }else {
        defaultFlush();
    }
}

void Logger::formatHeader(std::string &buffer, const char *file, int line, LogLevel level, const char *func) {
    char buf[128]={0};

    struct timeval tv;
    gettimeofday(&tv,nullptr);
    time_t seconds=tv.tv_sec;

    struct tm tm_time;
    localtime_r(&seconds,&tm_time);

    snprintf(buf,sizeof(buf),
        "%04d-%02d-%02d %02d:%02d:%02d.%06ld [%s] %s %d:%s ",
        tm_time.tm_year+1900,
        tm_time.tm_mon+1,
        tm_time.tm_mday,
        tm_time.tm_hour,
        tm_time.tm_min,
        tm_time.tm_sec,
        tv.tv_usec,
        LogLevelName[static_cast<int>(level)],
        file,
        line,
        func);

    buffer.append(buf);
}
