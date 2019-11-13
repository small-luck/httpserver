/*
 * 回调函数模块
 * author:liupei
 * date:2019/11/12
 * */
#ifndef __CALLBACK_H__
#define __CALLBACK_H__

#include <functional>

class Connection;

//业务逻辑函数回调
typedef std::function<bool(Connection*)> ProcessCallback;

#endif /*__CALLBACK_H__*/
