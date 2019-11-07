/*
 *工具模块，一些常用的基本函数
 *author:liupei
 *date:2019/10/31
 * */
#ifndef __UTILS_H__
#define __UTILS_H__

#include <sys/types.h>
#include <string>
#include "net.h"

class Util {
public:
    //获取线程ID
    static pid_t get_thread_id();

    //设置fd non-block
    static int set_fd_nonblock(int fd);

    //设置socket reuseaddr
    static int set_reuseaddr(int fd);

    //设置socket reuseport
    static int set_reuseport(int fd);

    //设置socket nodelay, forbidden nagel
    static int set_no_use_nagle(int fd);

    //设置socket fd send and recv timeout
    static int set_fd_send_recv_timeout(int fd);

    //ET模式下发送数据
    static bool send_data(int fd, std::string& sendstr);

    //ET模式下接收数据
    static bool recv_data(int fd, std::string& recvstr);

    //根据分隔符将str分成若干个，放入vector
    static void split(const std::string& str, std::vector<std::string>& v, const char* separator = ":");
    
    //根据分隔符将str分成两半，放入pair中
    static void cut(const std::string& str, std::pair<std::string, std::string>& m, const char* separator = ":");
};

#endif //__UTILS_H__
