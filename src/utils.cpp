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
pid_t Util::get_thread_id()
{
    return ::syscall(SYS_gettid);
}

//设置fd non-block 
int Util::set_fd_nonblock(int fd)
{
    if (fd < 0) 
        return -NET_ARGUMENT_ERROR;

    int oldflag = ::fcntl(fd, F_GETFL, 0);
    int newflag = oldflag | O_NONBLOCK;
    return (::fcntl(fd, F_SETFL, newflag));
}

//设置socket reuseaddr
int Util::set_reuseaddr(int fd)
{
    int on = 1;

    if (fd < 0)
        return -NET_ARGUMENT_ERROR;

    return (::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char*)&on, sizeof(on)));
}

//设置socket reuseport
int Util::set_reuseport(int fd)
{
    int on = 1;

    if (fd < 0)
        return -NET_ARGUMENT_ERROR;

    return (::setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, (char*)&on, sizeof(on)));
}

//设置socket nodelay, forbidden nagel
int Util::set_no_use_nagle(int fd)
{
    int on = 1;

    if (fd < 0)
        return -NET_ARGUMENT_ERROR;

    return (::setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (char*)&on, sizeof(on)));
}

//设置socket fd send and recv timeout
int Util::set_fd_send_recv_timeout(int fd)
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
bool Util::send_data(int fd, std::string& sendstr)
{
    int nsend = 0;

    if (fd < 0 ||sendstr.empty())  
        return false;

    while (true) {
        nsend = ::send(fd, sendstr.c_str(), sendstr.length(), 0);
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
        
        sendstr.erase(nsend);

        if (sendstr.length() == 0)
            break;
    }

    return true;
}

//ET模式下接收数据
bool Util::recv_data(int fd, std::string& recvstr)
{
    int nrecv;
    char msg[4096];
    if (fd < 0)
        return false;

    while (true) {
        memset(&msg, 0, sizeof(msg));
        nrecv = ::recv(fd, msg, 4096, 0);
        if (nrecv < 0) {
            if (errno == EWOULDBLOCK || errno == EINTR) {
                break;
            }
            return false;
        } else if (nrecv == 0) {
            //对端关闭
            return false;
        } else {
            recvstr.append(msg);
        }
    }
    
    return true;
}

//根据分隔符将str分成若干个，放入vector
void Util::split(const std::string& str, std::vector<std::string>& v, const char* separator/* = ";"*/)
{
    if (str.empty() || separator == nullptr)
        return;

    std::string buf(str);
    std::size_t pos = std::string::npos;
    int separator_len = strlen(separator);
    std::string substr_str;

    while (true) {
        pos = buf.find(separator);
        if (pos != std::string::npos) {
            substr_str = buf.substr(0, pos);
            if (!substr_str.empty())
                v.push_back(substr_str);
            buf = buf.substr(pos + separator_len);
        } else {
            if (!buf.empty())
                v.push_back(buf);
            break;
        }
    }
}

//根据分隔符将str分成两半，放入map中
void Util::cut(const std::string& str, std::pair<std::string, std::string>& m, const char* separator/* = ":"*/)
{
    if (str.empty() || separator == nullptr)
        return;

    int separator_len = strlen(separator);
    std::size_t pos;

    pos = str.find(separator);
    if (pos != std::string::npos) {
        m.first = str.substr(0, pos);
        m.second = str.substr(pos + separator_len);
    }
}
