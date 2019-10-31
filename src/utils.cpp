/*
 *工具模块封装
 *author:liupei
 *date:2019/10/31
 * */
#include "../inc/utils.h"
#include <sys/syscall.h>
#include <unistd.h>

pid_t get_thread_id()
{
    return syscall(SYS_gettid);
}
