/*
 * HTTP协议模块
 * author:liupei
 * date:2019/11/12
 * */
#ifndef __PROTOCOL_H__
#define __PROTOCOL_H__

#include <map>
#include <string>
#include "net.h"
#include "dispatcher.h"

class Protocol {
public:
    //业务处理回调函数
    static bool process_callback(Connection* conn);

    //获取dispatcher具体实例
    static Dispatcher* get_dispatcher(std::string& method);
};
#endif /*__PROTOCOL_H__*/

