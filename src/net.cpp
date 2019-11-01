/*
 *底层网络模块
 *author:liupei
 *date:2019/10/31
 * */

#include "net.h"
#include "log.h"
#include "utils.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <errno.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>

//将fd从epoll中del
int Context::remove_from_epoll(int epollfd, int fd)
{   
    if (fd < 0 || epollfd < 0)
        return -NET_ARGUMENT_ERROR;

    return (::epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, NULL));
}

//将fd添加进epoll
int Context::add_to_epoll(int epollfd, int fd)
{
    if (fd < 0 || epollfd < 0)
        return -NET_ARGUMENT_ERROR;

    struct epoll_event ev;
    ev.data.fd = fd;
    ev.events = EPOLLIN | EPOLLET;

    return (::epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &ev));
}

//初始化context
void Context::init()
{
    int i;

    m_requests_queue.clear();
    m_responses_queue.clear();
    m_dataready_queue.clear(); 

    for (i = 0; i < SEND_THREAD_NUM; i++) {
        m_send_threads[i].reset(new std::thread(std::bind(&Context::send_reponse, this)));
    }

    for (i = 0; i < RECV_THREAD_NUM; i++) {
        m_recv_threads[i].reset(new std::thread(std::bind(&Context::recv_request, this)));
    }

    for (i = 0; i < PROCESS_THREAD_NUM; i++) {
        m_process_threads[i].reset(new std::thread(std::bind(&Context::process, this)));
    }
}

//结束时回收context资源
void Context::uninit()
{
    int i;
    int ret;

    if (!m_exit) {
        m_exit = true;
        
        //退出线程
        m_requests_cond.notify_all();
        m_responses_cond.notify_all();
        m_dataready_cond.notify_all();
        
        for (i = 0; i < SEND_THREAD_NUM; i++)
            m_send_threads[i]->join();

        for (i = 0; i < RECV_THREAD_NUM; i++)
            m_recv_threads[i]->join();
        
        for (i = 0; i < PROCESS_THREAD_NUM; i++)
            m_process_threads[i]->join();

    }

    //释放epollfd和listenfd
    ret = remove_from_epoll(m_epollfd, m_listenfd);
    if (ret < 0) {
        LOG_ERROR("remove_from_epoll error, ret:%d, fd:%d, errno:%d\n", ret, m_listenfd, errno);
    }

    if (m_listenfd > 0) {
        shutdown(m_listenfd, SHUT_RDWR);
        close(m_listenfd);
    }

    if (m_epollfd > 0) 
        close(m_epollfd);
    
    m_epoll_events.clear();

    //释放所有队列中还未执行的请求或者响应
    if (!m_requests_queue.empty()) {
        for (auto& i : m_requests_queue) {
            if (i->m_sockfd > 0) {
                shutdown(i->m_sockfd, SHUT_RDWR);
                close(i->m_sockfd);
                i->m_sockfd = -1;
            }
        }
    }

    if (!m_responses_queue.empty()) {
        for (auto& i : m_responses_queue) {
            if (i->m_sockfd > 0) {
                shutdown(i->m_sockfd, SHUT_RDWR);
                close(i->m_sockfd);
                i->m_sockfd = -1;
            }
        }
    }
    
    if (!m_dataready_queue.empty()) {
        for (auto& i : m_dataready_queue) {
            if (i->m_sockfd > 0) {
                shutdown(i->m_sockfd, SHUT_RDWR);
                close(i->m_sockfd);
                i->m_sockfd = -1;
            }
        }
    }

    m_requests_queue.clear();
    m_responses_queue.clear();
    m_dataready_queue.clear();
}

//发送函数
void Context::send_reponse()
{

}

//接收函数
void Context::recv_request()
{
    std::shared_ptr<Connection> conn = nullptr;
    int sockfd;
    bool ret;
    
    //线程循环
    while (!m_exit) {
        std::unique_lock<std::mutex> lock(m_dataready_lock);
        while (m_dataready_queue.empty()) {
            if (m_exit)
                return;
            m_dataready_cond.wait(lock);
        }

        conn = m_dataready_queue.front();
        if (conn->m_sockfd < 0) {
            LOG_ERROR("sockfd is less than zero, sockfd:%d\n", conn->m_sockfd);
            continue;
        }
        m_dataready_queue.pop_front();
    
        //接收数据
        sockfd = conn->m_sockfd;
        std::shared_ptr<Request> new_request = std::make_shared<Request>();
        ret = recv_data(sockfd, new_request->m_req_buff);
        if (!ret) {
            //连接有问题或对端关闭
            LOG_INFO("fd:%d is close\n", sockfd);
            close(sockfd);
            continue;
        }

        //接收成功, 放入
        LOG_MSG("recvdata: %s\n", new_request->m_req_buff.c_str());



       
    }
}

//业务解析函数，包括从请求队列获取数据，解析，然后组好响应包，放入响应队列中
void Context::process()
{

}

//accept流程
int Context::do_accept()
{
    struct sockaddr_in clientaddr;
    socklen_t addrlen;
    int ret;
    int newfd;
    
    memset(&clientaddr, 0, sizeof(clientaddr));
    newfd = ::accept(m_listenfd, (struct sockaddr*)&clientaddr, &addrlen);
    if (newfd < 0) {
        LOG_ERROR("accept error, ret:%d, errno:%d\n", newfd, errno);
        return newfd;
    }
    
    //将newfd设置为非阻塞
    ret = set_fd_nonblock(newfd);
    if (ret < 0) {
        LOG_ERROR("set_fd_nonblock error, ret:%d, fd:%d, errno:%d\n", ret, newfd, errno);
        close(newfd);
        return ret;
    }

    //set nodelay, forbidden nagle 
    ret = set_no_use_nagle(newfd);
    if (ret < 0) {
        LOG_ERROR("set_no_use_nagle error, ret:%d, fd:%d, errno:%d\n", ret, newfd, errno);
        close(newfd);
        return ret;
    }

    //set send and recv timeout
    ret = set_fd_send_recv_timeout(newfd);
    if (ret < 0) {
        LOG_ERROR("set_fd_send_recv_timeout error, ret:%d, fd:%d, errno:%d\n", ret, newfd, errno);
    }

    LOG_MSG("get a new connection, fd:%d, ip:%s, port:%d\n", newfd, inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));

    //将newfd加入epoll
    ret = add_to_epoll(m_epollfd, newfd);
    if (ret < 0) {
        LOG_ERROR("add_to_epoll error, ret:%d, epollfd:%d, fd:%d, errno:%d\n", ret, m_epollfd, newfd, errno);
        close(newfd);
        return ret;
    }

    return NET_SUCCESS;
}

//主线程循环函数
void Context::main_loop(const char* ip, short port)
{
    int ret;
    struct sockaddr_in servaddr;
    int nevents;
    int i;

    if (ip == nullptr)
        return;
    
    m_listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (m_listenfd < 0) {
        LOG_ERROR("socket error, ret:%d, errno:%d\n", m_listenfd, errno);
        return;
    }
    
    ret = set_fd_nonblock(m_listenfd);
    if (ret < 0) {
        LOG_ERROR("set_fd_nonblock error, ret:%d, fd:%d, errno:%d\n", ret, m_listenfd, errno);
        return;
    }

    //set listenfd reuseaddr reuseport
    ret = set_reuseaddr(m_listenfd);
    if (ret < 0) {
        LOG_ERROR("set_reuseaddr error, ret:%d, fd:%d, errno:%d", ret, m_listenfd, errno);
        return;
    }

    ret = set_reuseport(m_listenfd);
    if (ret < 0) {
        LOG_ERROR("set_reuseport error, ret:%d, fd:%d, errno:%d", ret, m_listenfd, errno);
        return;
    }

    LOG_MSG("ip = %s\n", ip);
    LOG_MSG("port = %d\n", port);

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);
    servaddr.sin_addr.s_addr = inet_addr(ip);
    
    ret = ::bind(m_listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr));
    if (ret == -1) {
        LOG_ERROR("bind error, ret:%d, fd:%d, errno:%d\n", ret, m_listenfd, errno);
        return;
    }

    ret = ::listen(m_listenfd, 50);
    if (ret < 0) {
        LOG_ERROR("listen error, ret:%d, fd:%d, errno:%d\n", ret, m_listenfd, errno);
        return;
    }

    m_epollfd = ::epoll_create(1);
    if (m_epollfd < 0) {
        LOG_ERROR("epoll_create error, ret:%d, errno:%d\n", m_epollfd, errno);
        return;
    }

    //将listen加入epoll
    struct epoll_event ev;
    ev.data.fd = m_listenfd;
    ev.events = EPOLLET | EPOLLIN | EPOLLRDHUP;
    ret = (::epoll_ctl(m_epollfd, EPOLL_CTL_ADD, m_listenfd, &ev));
    if (ret < 0) {
        LOG_ERROR("epoll_ctl error, ret:%d, epollfd:%d, fd:%d, errno:%d\n", ret, m_epollfd, m_listenfd, errno);
        return;
    }

    while (true) {
        nevents = ::epoll_wait(m_epollfd, &*m_epoll_events.begin(), static_cast<int>(m_epoll_events.size()), -1);
        //nevents = ::epoll_wait(m_epollfd, m_epoll_events, 1024, 1000);
        if (nevents == 0) {
            continue;
        } else if (nevents < 0) {
            LOG_ERROR("epoll_wait error, ret:%d, errno:%d\n", nevents, errno);
            continue;
        }
        
        //判断event的num，是否要动态扩增m_epoll_events
        if (static_cast<size_t>(nevents) == m_epoll_events.size()) {
            m_epoll_events.resize(m_epoll_events.size() * 2);
        }

        //当有IO事件触发
        for (i = 0; i < nevents; i++) {
            if (m_epoll_events[i].data.fd == m_listenfd) {
                ret = do_accept();
                if (ret < 0) {
                    LOG_ERROR("do_accept error, ret:%d, fd:%d, errno:%d\n", ret, m_listenfd, errno);
                }
                continue;
            }
            
            std::shared_ptr<Connection> new_conn = std::make_shared<Connection>();
            new_conn->m_sockfd = m_epoll_events[i].data.fd;
            
            m_dataready_lock.lock();
            m_dataready_queue.push_back(new_conn);
            m_dataready_cond.notify_one();
            m_dataready_lock.unlock();
        }
    }
}
