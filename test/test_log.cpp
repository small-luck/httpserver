#include <iostream>
#include "../inc/log.h"

int main()
{
    log_init("test.log", LOG_INFO_MASK | LOG_WARN_MASK | LOG_MSG_MASK | LOG_ERROR_MASK);
    LOG_MSG("ret = %d\n", 10);
    LOG_INFO("hello world\n");
 
    while (true) {

    }
    return 0;
}

