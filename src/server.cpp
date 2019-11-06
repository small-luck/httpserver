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
#include <syslog.h>

#define DEFAULTPORT     (10086)
#define DEFAULTIP       "0.0.0.0"

std::string project_name = "httpserver";

std::shared_ptr<Httpserver> g_httpserver = nullptr;
//信号处理
void prog_exit(int signo)
{
    std::cout << "program recv signal [" << signo << "] to exit." << std::endl;

    //销毁context模块
    g_httpserver->uninit();

    //停止日志模块
    log_uninit();
}
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

bool check_pid_file(std::string& pid_path)
{
    if (pid_path.empty())
        return false;

    pid_t pid = getpid();
    int fd;
    char buf[8] = {0};
    struct flock lock;

    memset(&lock, 0, sizeof(lock));
    lock.l_type = F_WRLCK;
    lock.l_whence = SEEK_SET;
    const char* path = pid_path.c_str();

    fd = open(path, O_CREAT|O_SYNC|O_WRONLY, 0644);
    if (fd < 0) {
        syslog(LOG_ERR, "open pid file:%s failed, errno:%d\n", path, errno);
        return false;
    }

    if (fcntl(fd, F_SETLK, &lock) < 0) {
        syslog(LOG_ERR, "lock pid file %s failed, errno:%d\n", path, errno);
        close(fd);
        return false;
    }

    if (ftruncate(fd, 0) < 0) {
        syslog(LOG_ERR, "ftruncate failed, fd:%d, length:%d, errno:%d\n", fd, 0, errno);
        close(fd);
        return false;
    }

    sprintf(buf, "%d\n", (int)pid);
    if (write(fd, buf, strlen(buf)) != (int)strlen(buf)) {
        syslog(LOG_ERR, "write buf failed, fd:%d, buf:%s, errno:%d\n", fd, buf, errno);
        close(fd);
        return false;
    }

    return true;
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
    //设置信号处理
    signal(SIGCHLD, SIG_DFL);
    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT, prog_exit);
    signal(SIGTERM, prog_exit);

    int ch;
    int ret;
    bool bdaemon = false;
    short port = 0;
    char ip[32] = {0};
    std::string pid_path;

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

    if (port == 0)
        port = DEFAULTPORT;
    if (strlen(ip) == 0)
        strcpy(ip, DEFAULTIP);

    if (bdaemon) {
        daemon_run();
    }

    //将进程pid写入文件，保证该进程只有一个运行
    pid_path = "/var/run/" + project_name + ".pid";
    if (!check_pid_file(pid_path)) {
        ret = -EACCES;
        return ret;
    }
    
    //启动日志模块
    log_init("httpserver.log", LOG_INFO_MASK | LOG_MSG_MASK | LOG_WARN_MASK | LOG_ERROR_MASK);

    //启动context模块
    g_httpserver = std::make_shared<Httpserver>();
    g_httpserver->init();

    //main_loop
    g_httpserver->main_loop(ip, port);
    
    std::cout << "httpserver exit." << std::endl;

    return 0;
}

