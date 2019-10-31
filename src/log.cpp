/*
 *日志模块 log.cpp
 *author: liupei
 *date: 2019/10/30
 * */

#include "../inc/log.h"
#include "../inc/utils.h"
#include <iostream>
#include <string>
#include <time.h>

//获取Logger实例
Logger& Logger::get_instance()
{
    static Logger logger;
    return logger;
}

//启动日志模块
bool Logger::start(const char* filepath, int mask)
{
    if (filepath == NULL) {
        m_filepath.append(LOG_DEFAULT_FILEPATH);
    } else {
        m_filepath.append(filepath);
    }
    
    m_mask = mask;

    m_filepoint = fopen(m_filepath.c_str(), "at+");
    if (m_filepoint == NULL) {
        return false;
    }

    //创建log工作线程
    m_work_thread.reset(new std::thread(std::bind(&Logger::do_log, this)));

    return true;
}

//停止日志模块
void Logger::stop()
{
    if (!m_exit) {
        m_exit = true;
        m_cond.notify_one();
    }

    m_work_thread->join();
        
    if (m_filepoint) {
        fclose(m_filepoint);
        m_filepoint = NULL;
    }    
}

//将日志信息放入队列中
void Logger::add_to_list(int type, const char* level, const char* filename, int line, const char* funcname, const char* fmt, ...)
{
    char content[ONE_LOG_MAXSIZE] = {0};
    char msg[256] = {0};

    va_list varg_list;
    va_start(varg_list, fmt);
    vsnprintf(msg, 256, fmt, varg_list);
    va_end(varg_list);

    time_t now = time(NULL);
    struct tm* tmstr = localtime(&now);
    
    //格式：[]2019-10-30 17:31:50] [INFO] [12345] [main.cpp:12 main(int argc, char* argv[])]: hello world 
    sprintf(content, "[%04d-%02d-%02d %02d:%02d:%02d] [%s] [%d] [%s:%d %s]: %s",
                tmstr->tm_year + 1900,
                tmstr->tm_mon + 1,
                tmstr->tm_mday,
                tmstr->tm_hour,
                tmstr->tm_min,
                tmstr->tm_sec,
                level,
                get_thread_id(),
                filename,
                line,
                funcname,
                msg);

    //如果打印到控制台
    if (type == 0) {
        std::cout << content;
        return;
    }
    
    //将日志信息放入队列中
    std::lock_guard<std::mutex> guard(m_mutex);
    m_log_list.emplace_back(content);
    
    //通知日志工作线程
    m_cond.notify_one();
}

//日志工作线程，用于打印或者写入日志
void Logger::do_log()
{
    if (m_filepoint == NULL)
        return;
    
    while (!m_exit) {
        //写日志
        std::unique_lock<std::mutex> guard(m_mutex);
        while (m_log_list.empty()) {
            if (m_exit)
                return;
            
            m_cond.wait(guard);
        }

        const std::string& logstr = m_log_list.front();

        fwrite(logstr.c_str(), logstr.length(), 1, m_filepoint);
        fflush(m_filepoint);
        m_log_list.pop_front();
    }
}

void log_init(const char* filename, int mask)
{
    Logger::get_instance().start(filename, mask);
}

void log_uninit()
{
    Logger::get_instance().stop();
}
