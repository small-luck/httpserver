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

//获取线程ID
pid_t get_thread_id();

//设置fd non-block
int set_fd_nonblock(int fd);

//设置socket reuseaddr
int set_reuseaddr(int fd);

//设置socket reuseport
int set_reuseport(int fd);

//设置socket nodelay, forbidden nagel
int set_no_use_nagle(int fd);

//设置socket fd send and recv timeout
int set_fd_send_recv_timeout(int fd);

//ET模式下发送数据
bool send_data(Connection* conn);

//ET模式下接收数据
bool recv_data(Connection* conn);

#endif //__UTILS_H__
