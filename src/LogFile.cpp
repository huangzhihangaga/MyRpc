/**
 * @file LogFile.cpp
 * @brief LogFile类实现
 */

#include "LogFile.h"
#include <ctime>

LogFile::LogFile(const std::string &basename, size_t rollSize, int flushInterval)
    :basename_(basename)
    ,rollSize_(rollSize)
    ,flushInterval_(flushInterval)
    ,lastRoll_(0)
    ,lastFlush_(0)
    ,currentRollTime_(0)
    ,rollCount_(0)
    ,fp_(nullptr){
    rollFile();
}

LogFile::~LogFile() {
    if (fp_) {
        fclose(fp_);
    }
}

void LogFile::append(const char *logline, int len) {
    write(logline,len);

    time_t now=time(nullptr);
    if (now-lastFlush_>=flushInterval_) {
        flush();
        lastFlush_=now;
    }
}

void LogFile::flush() {
    if (fp_) {
        fflush(fp_);
    }
}

bool LogFile::rollFile() {
    if (fp_) {
        fflush(fp_);
        fclose(fp_);
        fp_=nullptr;
    }

    std::string filename=getLogFileName();
    fp_=fopen(filename.c_str(),"a");
    if (!fp_) {
        return false;
    }

    lastRoll_=time(nullptr);
    lastFlush_=lastRoll_;
    return true;
}

void LogFile::write(const char *logline, int len) {
    if (!fp_) return;

    size_t written=fwrite(logline,1,len,fp_);
    if (written!=static_cast<size_t>(len)) {
        rollFile();
        if (fp_) {
            written=fwrite(logline,1,len,fp_);
        }

        if (written!=static_cast<size_t>(len)) {
            fprintf(stderr,"LogFile failed to write log,len=%d,logmsg=%s",len,logline);
        }
    }

    if (fp_ && ftell(fp_)>static_cast<long>(rollSize_)) {
        rollFile();
    }
}

std::string LogFile::getLogFileName() {
    std::string filename=basename_;

    char timebuf[64];
    time_t now=time(nullptr);
    struct tm tm_time;
    localtime_r(&now,&tm_time);

    if (now!=currentRollTime_) {
        currentRollTime_=now;
        rollCount_=1;
    }else {
        ++rollCount_;
    }

    snprintf(timebuf,sizeof(timebuf),".%04d%02d%02d-%02d%02d%02d.%d",
                                                    tm_time.tm_year+1900,
                                                    tm_time.tm_mon+1,
                                                    tm_time.tm_mday,
                                                    tm_time.tm_hour,
                                                    tm_time.tm_min,
                                                    tm_time.tm_sec,
                                                    rollCount_);
    filename+=timebuf;
    filename+=".log";
    return filename;
}
