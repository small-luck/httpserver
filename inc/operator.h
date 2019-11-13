/*
 *具体业务操作模块
 *author:liupei
 *date:2019/11/8
 * */
#ifndef __OPERATOR_H__
#define __OPERATOR_H__

#include "log.h"
#include "utils.h"
#include "net.h"

class Operation : public std::enable_shared_from_this<Operation>{
public:
    Operation(const char* name) {
        m_opname.append(name);
    }

    virtual ~Operation() {}

    std::shared_ptr<Operation> getptr() {
        return shared_from_this();
    }
    
    //初始化，为m_request赋值
    void init(std::shared_ptr<Request>& request);

    //业务执行入口
    virtual int execute() = 0;

protected:
    std::string m_opname;
    std::shared_ptr<Request> m_request = nullptr;
    std::shared_ptr<Response> m_reponse = nullptr;
};

class PutUser : public Operation{
public:
    PutUser() : Operation("putuser") {
    }

    virtual ~PutUser() {}
    
    //PUT执行的
    virtual int execute() override;
    
    //获取类名
    std::string& get_name();

private:
    std::string m_content;
};

class GetUser : public Operation {
public:

private:

};

class DeleteUser : public Operation {
public:

private:

};

#endif /*__OPERATOR_H__*/

