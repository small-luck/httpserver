/*
 * 服务启动
 * author: liupei
 * date: 2019/11/08
 * */
#ifndef __SERVER_H__
#define __SERVER_H__

#include "net.h"
#include "log.h"
#include "singleton.h"
#include "callback.h"

class Server : public Singletonptr<Server>{
public:
    //server初始化
    bool init(bool bdaemon);

    //server start
    void start(const char* ip, short port, int log_mask, const ProcessCallback& cb);

    //server stop
    void stop();  

private:    
    //写入pid文件
    bool check_pid_file();
    
    //后台化
    void daemon_run();

private:
    bool m_daemon = false;
    std::string m_pidpath;
    std::shared_ptr<Httpserver> m_httpserver = nullptr;
    std::shared_ptr<Logger> m_logger = nullptr;
    std::string m_server_name = "httpserver";
    std::string m_log_name = "httpserver.log";
};

#endif
