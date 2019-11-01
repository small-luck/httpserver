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

struct Request {
    std::string m_req_buff;
};

struct Response {
    std::string m_resp_buff;
};

struct Connection {
    //socket fd
    int         m_sockfd = 0;
    
    //请求或者响应
   // union {
        std::shared_ptr<Request> m_request;
        std::shared_ptr<Response> m_response;
   // }u;
};

class Context {
public:
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
    int remove_from_epoll(int epollfd, int fd);

    //将fd添加进epoll
    int add_to_epoll(int epollfd, int fd);

    //accept流程
    int do_accept();

public:
    //连接请求队列, for processThread
    std::list<std::shared_ptr<Connection>>        m_requests_queue;
    //连接请求队列锁
    std::mutex                                      m_requests_lock;
    //连接请求队列条件变量
    std::condition_variable                         m_requests_cond;

    //连接响应队列 for sendThread
    std::list<std::shared_ptr<Connection>>        m_responses_queue;
    //连接响应队列锁
    std::mutex                                      m_responses_lock;
    //连接响应队列条件变量
    std::condition_variable                         m_responses_cond;

    //数据到来的队列 for recvThread
    std::list<std::shared_ptr<Connection>>        m_dataready_queue;
    //dataready_queue的锁
    std::mutex                                      m_dataready_lock;
    //dataready_queue的条件变量
    std::condition_variable                         m_dataready_cond;

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
    //struct epoll_event m_epoll_events[1024];
    //监听fd
    int                                             m_listenfd = -1;
};

enum {
    NET_SUCCESS = 0,
    NET_ARGUMENT_ERROR,
    NET_INTERVAL_ERROR
};


#endif  //__NET_H__
