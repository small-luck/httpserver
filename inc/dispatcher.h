/*
 * 业务分发模块
 * author:liupei
 * date:2019/11/8
 * */
#ifndef __DISPATCHER_H__
#define __DISPATCHER_H__

#include "operator.h"
#include "net.h"

class Dispatcher {
public:
    virtual ~Dispatcher() {}

    virtual std::shared_ptr<Operation> get_op() = 0;
    
protected:
    std::shared_ptr<Operation> m_op = nullptr;
};

class DispatcherPut : public Dispatcher {
public:

private:

};

class DispatcherGet : public Dispatcher {
public:

private:

};

class DispatcherDelete : public Dispatcher {
public:

private:

};

#endif /*__DISPATCHER_H__*/
