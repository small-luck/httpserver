/*
 *日志模块头文件 log.h
 *author: liupei
 *date:2019/10/30
 * */
#ifndef __LOG_H__
#define __LOG_H__

#include <iostream>
#include <list>
#include <thread>
#include <condition_variable>
#include <time.h>
#include <stdio.h>
#include <memory>
#include <stdarg.h>

class Logger;

//日志掩码
#define LOG_INFO_MASK   0x00000001
#define LOG_WARN_MASK   0x00000010
#define LOG_MSG_MASK    0x00000100
#define LOG_ERROR_MASK  0x00001000

#define DEBUG       //for test

#define LOG_DEBUG(type, mask, level, file, line, funcname, args...) \
    if (Logger::get_instance().get_mask() & mask) { \
        Logger::get_instance().add_to_list(type, level, file, line, funcname, ##args); \
    }

#ifndef DEBUG
    #define LOG_INFO(...)    LOG_DEBUG(1, LOG_INFO_MASK, "INFO", __FILE__, __LINE__, __PRETTY_FUNCTION__, __VA_ARGS__)
    #define LOG_WARN(...)    LOG_DEBUG(1, LOG_WARN_MASK, "WARN", __FILE__, __LINE__, __PRETTY_FUNCTION__, __VA_ARGS__)
    #define LOG_MSG(...)     LOG_DEBUG(1, LOG_MSG_MASK, "MSG", __FILE__, __LINE__, __PRETTY_FUNCTION__, __VA_ARGS__)
    #define LOG_ERROR(...)   LOG_DEBUG(1, LOG_ERROR_MASK, "ERROR", __FILE__, __LINE__, __PRETTY_FUNCTION__, __VA_ARGS__)
#else 
    #define LOG_INFO(...)    LOG_DEBUG(0, LOG_INFO_MASK, "INFO", __FILE__, __LINE__, __PRETTY_FUNCTION__, __VA_ARGS__)
    #define LOG_WARN(...)    LOG_DEBUG(0, LOG_WARN_MASK, "WARN", __FILE__, __LINE__, __PRETTY_FUNCTION__, __VA_ARGS__)
    #define LOG_MSG(...)     LOG_DEBUG(0, LOG_MSG_MASK, "MSG", __FILE__, __LINE__, __PRETTY_FUNCTION__, __VA_ARGS__)
    #define LOG_ERROR(...)   LOG_DEBUG(0, LOG_ERROR_MASK, "ERROR", __FILE__, __LINE__, __PRETTY_FUNCTION__, __VA_ARGS__)
#endif

#define LOG_DEFAULT_FILEPATH    "http_server.log"

#define ONE_LOG_MAXSIZE (1024)

class Logger {
public:
    //获取Logger实例
    static Logger& get_instance();

    //启动日志模块
    bool start(const char* filepath, int mask);

    //停止日志模块
    void stop();

    //将日志信息放入队列中
    void add_to_list(int type, const char* level, const char* filename, int line, const char* funcname, const char* fmt, ...);

    //获取日志掩码
    int get_mask() {
        return m_mask;
    }

private:
    //单例模式
    Logger() = default;

    //nocopyable
    Logger(const Logger& logger) = delete;
    Logger& operator= (const Logger& logger) = delete;

    //日志工作线程，用于打印或者写入日志
    void do_log();

private:
    //文件路径
    std::string                     m_filepath;
    
    //文件指针
    FILE*                           m_filepoint{};

    //日志级别掩码
    int                             m_mask = 0;

    //工作线程指针
    std::shared_ptr<std::thread>    m_work_thread;

    //日志队列锁
    std::mutex                      m_mutex;

    //日志队列条件变量
    std::condition_variable         m_cond;

    //日志线程是否退出
    bool                            m_exit{false};

    std::list<std::string>          m_log_list;

};

void log_init(const char* filename, int mask);
void log_uninit();

#endif //__LOG_H__
