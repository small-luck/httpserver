/*
 *底层网络模块头文件
 *author:liupei
 *date:2019/10/31
 * */
#ifndef __NET_H__
#define __NET_H__

#include <map>
#include <list>
#include <time.h>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <thread>
#include <vector>
#include <sys/epoll.h>
#include "callback.h"

#define SEND_THREAD_NUM     (1)
#define RECV_THREAD_NUM     (1)
#define PROCESS_THREAD_NUM  (1)

#define EPOLL_INIT_NUM       (32)

#define TCP_SEND_TIMEO      (10)
#define TCP_RECV_TIMEO      (10)

#define MAX_HEADER_SIZE     (4096)

//connection的状态
enum {
    CONN_STATE_INIT = 0,            //初始化
    CONN_STATE_RECV_HEADER,         //准备接收header
    CONN_STATE_RECV_BODY,           //准备接收body
    CONN_STATE_SEND_HEADER,         //准备发送header
    CONN_STATE_SEND_BODY,           //准备发送body
    CONN_STATE_BAD                  //connecttion error
};

//支持的HTTP Method
enum {
    PUT = 0,
    GET,
    DELETE
};

static std::map<std::string, int> g_methods = {
    {"PUT", PUT},
    {"GET", GET},
    {"DELETE", DELETE}
};

class Request {
public:
    Request() {}
       
    void init(std::string& method, std::string& version, std::string& url, uint64_t len) {
        m_http_method = method;
        m_http_version = version;
        m_url = url;
        m_content_length = len;
    }

    std::string& get_method() {
        return m_http_method;
    }

    uint64_t get_content_length() {
        return m_content_length;
    }

    std::string& get_url() {
        return m_url;
    }

private:
    std::string                 m_http_method;
    std::string                 m_http_version;
    std::string                 m_url;
    uint64_t                    m_content_length = 0;
};

class Response {
public:

private:

};

class Httpserver;
class Connection {
public:
    Connection(int fd, std::shared_ptr<Httpserver> ptr) {
        m_sockfd = fd;
        m_httpserver = std::shared_ptr<Httpserver>(ptr);
        m_request = std::make_shared<Request>();
        m_response = std::make_shared<Response>();
    }
    
    //释放一个connection
    static void free_conn(Connection* conn);

    //判断一个connection是否合法，如果不合法，就free
    static bool assert_conn(Connection* conn);

     //检查是否已经接收了一个完整的包
    bool check_has_complete_header();
    
    //解析http header
    bool parse_http_header();

public:
    //获取fd
    int get_fd() {
        return m_sockfd;
    }

    //设置fd
    void set_fd(int fd) {
        m_sockfd = fd;
    }

    //获取conn状态
    int get_state() {
        return m_state;
    }

    //设置conn状态
    void set_state(int state) {
        m_state = state;
    }

    //获取recvbuff
    std::string& get_recv() {
        return m_recv_buffer;
    }

    //设置recvbuff
    void append_to_recv(std::string& str) {
        m_recv_buffer.append(str);
    }

    //获取request ptr
    std::shared_ptr<Request>& get_request() {
        return m_request;
    }

    //获取response ptr
    std::shared_ptr<Response>& get_response() {
        return m_response;
    }

    //获取request header的末尾index
    int get_header_end() {
        return m_header_end;
    } 

    //设置content未接收的大小
    void set_content_left_len(uint64_t len) {
        m_left_content_len = len;
    }

    //获取content未接收的大小
    uint64_t get_content_left_len() {
        return m_left_content_len;
    }

private:
    //socket fd
    int                         m_sockfd = 0;

    //conn state
    int                         m_state = CONN_STATE_INIT; 

    //请求或者响应
    std::shared_ptr<Request>    m_request = nullptr;
    std::shared_ptr<Response>   m_response = nullptr;

    std::string                 m_send_buffer;
    std::string                 m_recv_buffer;

    //request header的末尾index
    int                         m_header_end = 0;
    
    //如果有body, 表示剩下还有多少字节的Body待接收
    uint64_t                    m_left_content_len = 0;

    //httpserver
    std::shared_ptr<Httpserver> m_httpserver;
};

class Httpserver : public std::enable_shared_from_this<Httpserver> {
public:
friend class Connection;
    //初始化
    void init(const char* ip, short port, const ProcessCallback& cb);

    //回收资源
    void uninit();

    //获取自身shared_ptr
    std::shared_ptr<Httpserver> get_ptr() {
        return shared_from_this();
    }

    //发送线程函数
    void send_reponse();

    //接收线程函数
    void recv_request();

    //业务协议线程函数
    void do_protocol();

    //主线程循环函数
    void main_loop();

    //将fd从epoll中del
    int remove_from_epoll(int epollfd, Connection* conn);

    //将fd添加进epoll
    int add_to_epoll(int epollfd, Connection* conn, bool flag);

    //accept流程
    int do_accept();

    //send notify
    void send_notify(int noyifyfd);

    //接收通知消息
    void recv_notify(Connection* conn);
    
    //创建sockpair，来设置conn
    int  set_sockpair_conn(Connection** conn, int* fd);

    //将conn重新放入epoll中
    int reset_conn_in_epoll(Connection* conn);
    
    //将conn重新放入epoll
    static void notify_modify(Connection* conn);

    //将conn放入wakeup queue
    static void notify_wakeup(Connection* conn);

    //获取是否退出
    bool get_exit() {
        return m_exit;
    }

    //获取request队列中的元素
    Connection* pop_element_from_requestqueue();

    //将conn放入response队列中
    void push_element_to_responsequeue(Connection* conn);

private:
    //连接请求队列, for processThread
    std::list<Connection*>        m_requests_queue;
    //连接请求队列锁
    std::mutex                                      m_requests_lock;
    //连接请求队列条件变量
    std::condition_variable                         m_requests_cond;

    //连接响应队列 for sendThread
    std::list<Connection*>        m_responses_queue;
    //连接响应队列锁
    std::mutex                                      m_responses_lock;
    //连接响应队列条件变量
    std::condition_variable                         m_responses_cond;

    //数据到来的队列 for recvThread
    std::list<Connection*>        m_dataready_queue;
    //dataready_queue的锁
    std::mutex                                      m_dataready_lock;
    //dataready_queue的条件变量
    std::condition_variable                         m_dataready_cond;

    //存储需要重新放入epoll中的connection
    std::list<Connection*>                          m_modify_queue;
    std::mutex                                      m_modify_lock;
    std::condition_variable                         m_modify_cond;

    //数据发送线程
    std::shared_ptr<std::thread>                    m_send_threads[SEND_THREAD_NUM];
    //数据接收线程
    std::shared_ptr<std::thread>                    m_recv_threads[RECV_THREAD_NUM];
    //业务线程
    std::shared_ptr<std::thread>                    m_process_threads[PROCESS_THREAD_NUM];  
    
    //是否退出
    bool                                            m_exit = false;

    //epollfd
    int                                             m_epollfd = -1;

    //epoll_event
    std::vector<struct epoll_event>                 m_epoll_events = std::vector<struct epoll_event>(32);
    
    //监听connection
    Connection*                                     m_listen_conn = nullptr;
    
    //socket pair,负责将epoll_wait返回
    Connection*                                     m_notify_wakeup_conn = nullptr;
    int                                             m_wakeup_fd = 0;

    //sockpair , 负责通知主线程修改epoll_event中的conn
    Connection*                                     m_notify_modify_conn = nullptr;
    int                                             m_modify_fd = 0;

    char                                            m_ip[32] = {0};
    short                                           m_port  = 0;

    //业务回调
    ProcessCallback                                 m_process_cb;
};
    
//错误码
enum {
    NET_SUCCESS = 0,
    NET_ARGUMENT_ERROR,
    NET_INTERVAL_ERROR
};


#endif  //__NET_H__
