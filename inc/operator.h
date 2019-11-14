/*
 *具体业务操作模块
 *author:liupei
 *date:2019/11/8
 * */
#ifndef __OPERATOR_H__
#define __OPERATOR_H__

#include "net.h"

class Operation {
public:
    Operation(const char* name) {
        m_opname.append(name);
    }
    virtual ~Operation() {}
        
    //初始化，为m_request赋值
    void init(std::shared_ptr<Request>& req, std::shared_ptr<Response>& res) {
        m_request = std::shared_ptr<Request>(req);
        m_reponse = std::shared_ptr<Response>(res);
    }

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
    
    //PUT执行的入口
    virtual int execute() override;
    
    //获取类名
    std::string& get_name() {
        return m_opname;
    }

private:
};

class GetUser : public Operation {
public:
    GetUser() : Operation("getuser") {
    }
    virtual ~GetUser() {}
    
    //PUT执行的入口
    virtual int execute() override;
    
    //获取类名
    std::string& get_name() {
        return m_opname;
    }

private:

};

class DeleteUser : public Operation {
public:
    DeleteUser() : Operation("deleteuser") {
    }
    virtual ~DeleteUser() {}
    
    //PUT执行的入口
    virtual int execute() override;
    
    //获取类名
    std::string& get_name() {
        return m_opname;
    }


private:

};

#endif /*__OPERATOR_H__*/

