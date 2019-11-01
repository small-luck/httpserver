/*
 *服务启动模块
 *author:liupei
 *date:2019/10/31
 * */

#include "log.h"
#include "net.h"
#include <iostream>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>

#define DEFAULTPORT     (10086)
#define DEFAULTIP       "0.0.0.0"

//后台化
void daemon_run()
{
    int pid;
    int fd;
    signal(SIGCHLD, SIG_IGN);
    
    pid = fork();
    if (pid < 0) {
        std::cout << "fork error" << std::endl;
        exit(1);
    } else if (pid > 0) {
        exit(0);
    }

    setsid();

    fd = open("/dev/null", O_RDWR, 0);
    if (fd != -1) {
        dup2(fd, STDIN_FILENO);
        dup2(fd, STDOUT_FILENO);
        dup2(fd, STDERR_FILENO);
    }

    if (fd > 2)
        close(fd);

}

static void useage()
{
    std::cout << "**********************************" << std::endl;
    std::cout << "-h  print help information" << std::endl;
    std::cout << "-d  use daemon mode" << std::endl;
    std::cout << "-p  port" << std::endl;
    std::cout << "-i  ip" << std::endl;
    std::cout << "**********************************" << std::endl;
}

int main(int argc, char* argv[])
{   
    int ch;
    bool bdaemon = false;
    short port = 0;
    char ip[32] = {0};
    Context ctxt;

    while ((ch = getopt(argc, argv, "hdp:i:")) != -1) {
        switch (ch) {
            case 'h':
                useage();
                return 0;
            case 'd':
                bdaemon = true;
                break;
            case 'p':
                port = atol(optarg);
                break;
            case 'i':
                strcpy(ip, optarg);
                break;
            default:
                useage();
                return 0;
        }
    }

    if (bdaemon) {
        daemon_run();
    }

    if (port == 0)
        port = DEFAULTPORT;
    if (strlen(ip) == 0)
        strcpy(ip, DEFAULTIP);
    
    //启动日志模块
    log_init("httpserver.log", LOG_INFO_MASK | LOG_MSG_MASK | LOG_WARN_MASK | LOG_ERROR_MASK);

    //启动context模块
    ctxt.init();

    //main_loop
    ctxt.main_loop(ip, port);
    
    //销毁context模块
    ctxt.uninit();

    //停止日志模块
    log_uninit();

    return 0;
}

