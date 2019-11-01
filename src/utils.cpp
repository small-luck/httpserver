/*
 *工具模块封装
 *author:liupei
 *date:2019/10/31
 * */
#include "utils.h"
#include "log.h"
#include <sys/syscall.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

//获取线程ID
pid_t get_thread_id()
{
    return ::syscall(SYS_gettid);
}

//设置fd non-block 
int set_fd_nonblock(int fd)
{
    if (fd < 0) 
        return -NET_ARGUMENT_ERROR;

    int oldflag = ::fcntl(fd, F_GETFL, 0);
    int newflag = oldflag | O_NONBLOCK;
    return (::fcntl(fd, F_SETFL, newflag));
}

//设置socket reuseaddr
int set_reuseaddr(int fd)
{
    int on = 1;

    if (fd < 0)
        return -NET_ARGUMENT_ERROR;

    return (::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char*)&on, sizeof(on)));
}

//设置socket reuseport
int set_reuseport(int fd)
{
    int on = 1;

    if (fd < 0)
        return -NET_ARGUMENT_ERROR;

    return (::setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, (char*)&on, sizeof(on)));
}

//设置socket nodelay, forbidden nagel
int set_no_use_nagle(int fd)
{
    int on = 1;

    if (fd < 0)
        return -NET_ARGUMENT_ERROR;

    return (::setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (char*)&on, sizeof(on)));
}

//设置socket fd send and recv timeout
int set_fd_send_recv_timeout(int fd)
{
    struct timeval tv;
    int ret;
    if (fd < 0)
        return -NET_ARGUMENT_ERROR;

    tv.tv_sec = TCP_SEND_TIMEO;
    tv.tv_usec = 0;

    ret = setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, (char*)&tv, sizeof(tv));
    if (ret < 0)
        return ret;

    return (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (char*)&tv, sizeof(tv)));
}

//ET模式下发送数据
bool send_data(int fd, std::string& str)
{
    int nsend = 0;

    if (fd < 0 || str.empty())  
        return false;

    while (true) {
        nsend = ::send(fd, str.c_str(), str.length(), 0);
        if (nsend < 0) {
            //如果是因为tcp发送窗口过小，无法发出去，休眠一段时间后再尝试发送
            if (errno == EWOULDBLOCK) {
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                continue;
            }
            //如果因为中断，则继续尝试发送
            if (errno == EINTR)
                continue;

            return false;
        }
        //如果对端关闭
        if (nsend == 0) {
            LOG_INFO("fd:%d is closed\n", fd);
            return false;
        }
        
        str.erase(nsend);

        if (str.length() == 0)
            break;
    }

    return true;
}

//ET模式下接收数据
bool recv_data(int fd, std::string& str)
{
    int nrecv;
    char msg[4096];

    if (fd < 0)
        return false;

    while (true) {
        memset(&msg, 0, sizeof(msg));
        nrecv = ::recv(fd, msg, 4096, 0);
        if (nrecv < 0) {
            if (errno == EWOULDBLOCK || errno == EINTR)
                return true;
            return false;
        } else if (nrecv == 0) {
            //对端关闭
            return false;
        } else {
            str.append(msg);
        }
    }
}



