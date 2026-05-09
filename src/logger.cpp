#include "logger.h"
#include <thread>
#include <time.h>
#include <iostream>


Logger &Logger::GetInstance() {
    static Logger logger;
    return logger;
}

Logger::Logger() {
    // 启动专门的写日志线程
    std::thread writeLogTask([&]() {
       for (;;) {
           // 获取当前的日期，然后去日志信息，写入相应的日志文件中 a+
           time_t now=time(nullptr);
           tm* nowtm=localtime(&now);

           char file_name[128];
           snprintf(file_name,sizeof(file_name),"%d-%d-%-dlog.txt",nowtm->tm_year+1900,nowtm->tm_mon+1,nowtm->tm_mday);

           FILE* fp=fopen(file_name,"a+");
           if (fp==nullptr) {
               std::cout<<"logger file:"<<file_name<<" open error!"<<std::endl;
               exit(EXIT_FAILURE);
           }

           std::string msg=m_lckQue.Pop();

           char time_buf[128]={0};
           snprintf(time_buf,sizeof(time_buf),"%d:%d:%d =>[%s] ",
                                                        nowtm->tm_hour,nowtm->tm_min,nowtm->tm_sec,(m_loglevel==INFO? "info":"error"));
           msg.insert(0,time_buf);
           msg.append("\n");

           fputs(msg.c_str(),fp);
           fclose(fp);
       }
    });
    // 设置分离线程，守护线程
    writeLogTask.detach();
}

void Logger::SetLogLevel(LogLevel level) {
    m_loglevel=level;
}

// 写日志，把日志信息写入lockqueue缓冲区当中
void Logger::Log(std::string msg) {
    m_lckQue.Push(msg);
}
