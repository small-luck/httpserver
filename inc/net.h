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

#define SEND_THREAD_NUM     (1)
#define RECV_THREAD_NUM     (1)
#define PROCESS_THREAD_NUM  (1)

#define EPOLL_INIT_NUM       (32)

#define TCP_SEND_TIMEO      (10)
#define TCP_RECV_TIMEO      (10)

//connection的状态
enum {
    CONN_STATE_INIT = 0,            //初始化
    CONN_STATE_RECV_HEADER,         //准备接收header
    CONN_STATE_RECV_BODY,           //准备接收body
    CONN_STATE_SEND_HEADER,         //准备发送header
    CONN_STATE_SEND_BODY,           //准备发送body
    CONN_STATE_BAD                  //connecttion error
};

class Request {
public:
    int m_content_length;
};

class Response {
};

class Connection {
public:
    Connection(int fd) : m_sockfd(fd) {
    }
    
    //释放一个connection
    static void free_conn(Connection* conn);

     //检查是否已经接收了一个完整的包
    bool check_has_complete_header();

    //解析http header
    bool parse_http_header();

public:
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
};

class Httpserver {
public:
friend class Connection;
    //初始化
    void init();

    //回收资源
    void uninit();

    //发送函数
    void send_reponse();

    //接收函数
    void recv_request();

    //业务函数
    void process();

    //主线程循环函数
    void main_loop(const char* ip, short port);

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
    int  set_sockpair_conn(Connection* conn, int* fd);

    //将conn重新放入epoll中
    int reset_conn_in_epoll(Connection* conn);
public:
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
};
    

enum {
    NET_SUCCESS = 0,
    NET_ARGUMENT_ERROR,
    NET_INTERVAL_ERROR
};


#endif  //__NET_H__
