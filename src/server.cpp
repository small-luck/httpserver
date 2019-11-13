/*
 *服务启动模块
 *author:liupei
 *date:2019/10/31
 * */

#include "server.h"
#include "protocol.h"
#include <iostream>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include <syslog.h>

#define DEFAULTPORT     (10086)
#define DEFAULTIP       "0.0.0.0"

std::shared_ptr<Server> g_server;

//server初始化
bool Server::init(bool bdaemon)
{
    bool ret;

    m_httpserver = std::make_shared<Httpserver>();
    m_logger = Logger::get_instance();
    m_daemon = bdaemon;
    m_pidpath = "/var/run" + m_server_name + ".pid";

    if (m_daemon) 
        daemon_run();

    ret = check_pid_file();
    if (!ret)
        return false;

    return true;
}

//server start
void Server::start(const char* ip, short port, int log_mask, const ProcessCallback& cb)
{
    m_logger->start(m_log_name.c_str(), log_mask);
    m_httpserver->init(ip, port, cb);
    m_httpserver->main_loop();
}

//server stop
void Server::stop()
{
    m_httpserver->uninit();
    m_logger->stop();
}

//后台化
void Server::daemon_run()
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

bool Server::check_pid_file()
{
    if (m_pidpath.empty())
        return false;

    pid_t pid = getpid();
    int fd;
    char buf[8] = {0};
    struct flock lock;

    memset(&lock, 0, sizeof(lock));
    lock.l_type = F_WRLCK;
    lock.l_whence = SEEK_SET;
    const char* path = m_pidpath.c_str();

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

//信号处理
void prog_exit(int signo)
{
    std::cout << "program recv signal [" << signo << "] to exit." << std::endl;
    g_server->stop();
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
    bool ret;
    bool bdaemon = false;
    short port = 0;
    char ip[32] = {0};

    while ((ch = getopt(argc, argv, "hdp:i:")) != -1) {
        switch (ch) {
            case 'h':
                useage();
                return 0;
            case 'd':
                bdaemon = true;
                break;
            case 'p':
                port = atoi(optarg);
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
    
    g_server = Server::get_instance();
    ret = g_server->init(bdaemon);
    if (!ret) {
        syslog(LOG_ERR, "init server failed\n");
        exit(1);
    }

    g_server->start(ip, port, LOG_INFO_MASK | LOG_MSG_MASK | LOG_WARN_MASK | LOG_ERROR_MASK, Protocol::process_callback);
    
    std::cout << "httpserver exit." << std::endl;
    
    return 0;
}

