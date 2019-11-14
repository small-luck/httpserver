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

//释放conn
void Connection::free_conn(Connection* conn)
{
    if (conn == nullptr)
        return;

    if (conn->m_sockfd > 0) {
        shutdown(conn->m_sockfd, SHUT_RDWR);
        close(conn->m_sockfd);
        free(conn);
        conn = nullptr;
    }
}

//判断一个connection是否合法，如果不合法，就free
bool Connection::assert_conn(Connection* conn)
{
    if (conn == nullptr)
        return false;

    int fd = conn->get_fd();
    int state = conn->get_state();

    if (fd <= 0)
        return false;

    if (state == CONN_STATE_BAD) {
        Connection::free_conn(conn);
        return false;
    }

    return true;
}

//检查是否已经接收了一个完整的包
bool Connection::check_has_complete_header()
{
    bool ret = false;
    if (m_recv_buffer.empty() || m_recv_buffer.size() < 4)
        return false;
    
    std::size_t pos = m_recv_buffer.find("\r\n\r\n");
    if(pos != std::string::npos) {
        ret = true;
        m_header_end = pos;
    }
  
    return ret;
}

//解析http header，判断header是否合法
bool Connection::parse_http_header()
{
    std::string header_str = m_recv_buffer.substr(0, m_header_end);

    /* LOG_MSG("header_str = %s\n", header_str.c_str()); */

    //根据\r\n分割为每一行
    std::vector<std::string> header_line;
    Util::split(header_str, header_line, "\r\n");

    /* LOG_MSG("header_line size = %d\n", header_line.size()); */
    /* for (int i = 0; i < (int)header_line.size(); i++) */ 
    /*     LOG_MSG("line[%d] = %s\n", i, header_line[i].c_str()); */
    
    //判断第一行是否符合规范
    std::vector<std::string> first_line_para;
    Util::split(header_line[0].c_str(), first_line_para, " ");
    /* LOG_MSG("first_paras size = %d\n", first_line_para.size()); */
    /* for (int i = 0; i < (int)first_line_para.size(); i++) */ 
    /*     LOG_MSG("para[%d] = %s\n", i, first_line_para[i].c_str()); */
    if (first_line_para.size() < 3) {
        LOG_ERROR("header first line:%s is invalid", header_line[0].c_str());
        return false;
    }

    std::string method = first_line_para[0];
    auto it = g_methods.find(method);
    if (it == g_methods.end()) {
        LOG_ERROR("method:%s is invalid\n", method.c_str());
        return false;
    }
    
    //获取必须的header 字段信息
    std::string url = first_line_para[1];
    std::string version = first_line_para[2];

    std::vector<std::pair<std::string, std::string>> header_set;
    //删除第一行元素
    header_line.erase(header_line.begin()); 
    for (auto& v : header_line) {
        std::pair<std::string, std::string> p;
        Util::cut(v, p, ": ");
        header_set.push_back(p);
    }

    //根据header的key获取value
    /* for (auto& v : header_set) */
    /*     LOG_MSG("key = %s, value = %s\n", v.first.c_str(), v.second.c_str()); */
    
    //获取Content-Length
    uint64_t content_length = 0;
    for (auto& v : header_set) {
        if (v.first.compare("Content-Length") == 0) {
            content_length = atol(v.second.c_str());
        }
    }

    this->m_request->init(method, version, url, content_length);

    return true;
}


//创建sockpair，来设置conn
int  Httpserver::set_sockpair_conn(Connection** conn, int* fd)
{
    int sock_pair[2];
    int ret;
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sock_pair) != 0) {
        LOG_ERROR("socketpair failed, errno:%d\n", errno);
        return -NET_INTERVAL_ERROR;
    }
    *conn = new Connection(sock_pair[1], get_ptr());
    ret = add_to_epoll(m_epollfd, *conn, 0);
    if (ret < 0) {
        LOG_ERROR("epoll_ctl error, ret:%d, epollfd:%d, fd:%d, errno:%d\n", ret, m_epollfd, sock_pair[1], errno);
        ret = -NET_INTERVAL_ERROR;
        goto error;
        
    }
    *fd = sock_pair[0];
    goto exit;
    
error:
    close(sock_pair[0]);
    close(sock_pair[1]);

exit:
    return NET_SUCCESS;
}

//通知主线程退出循环
void Httpserver::send_notify(int notifyfd)
{
    int notify = 1;
    write(notifyfd, (char*)&notify, sizeof(notify));
}
    
//接收通知消息
void Httpserver::recv_notify(Connection* conn)
{
    int notify;
    int fd = conn->get_fd();
    read(fd, (char*)&notify, sizeof(notify));
}

//将fd从epoll中del
int Httpserver::remove_from_epoll(int epollfd, Connection* conn)
{   
    int fd;
    if (conn == nullptr || epollfd < 0)
        return -NET_ARGUMENT_ERROR;
    
    fd = conn->get_fd();
    if (fd < 0)
        return -NET_ARGUMENT_ERROR;

    return (::epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, NULL));
}

//将fd添加进epoll
int Httpserver::add_to_epoll(int epollfd, Connection* conn, bool flag)
{
    int fd;
    if (conn == nullptr || epollfd < 0)
        return -NET_ARGUMENT_ERROR;
    
    fd = conn->get_fd();
    if (fd < 0)
        return -NET_ARGUMENT_ERROR;

    struct epoll_event ev;
    ev.data.ptr = static_cast<void*>(conn);
    ev.events = EPOLLIN | EPOLLET;
    //for listen fd
    if (flag) 
        ev.events |= EPOLLRDHUP;
 
    return (::epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &ev));
}

//将conn重新放入epoll中
int Httpserver::reset_conn_in_epoll(Connection* conn)
{
    if (conn == nullptr)
        return -NET_ARGUMENT_ERROR;

    int fd = conn->get_fd();
    if (fd < 0)
        return -NET_ARGUMENT_ERROR;
    
    struct epoll_event ev;
    ev.data.ptr = static_cast<void*>(conn);
    ev.events = EPOLLIN | EPOLLET;

    return (::epoll_ctl(m_epollfd, EPOLL_CTL_MOD, fd, &ev));
}

//初始化httpserver
void Httpserver::init(const char* ip, short port, const ProcessCallback& cb)
{
    int i;
    
    memcpy(m_ip, ip, strlen(ip));
    m_port = port;
    m_process_cb = cb;

    m_requests_queue.clear();
    m_responses_queue.clear();
    m_dataready_queue.clear(); 

    for (i = 0; i < SEND_THREAD_NUM; i++) {
        m_send_threads[i].reset(new std::thread(std::bind(&Httpserver::send_reponse, this)));
        //m_send_threads[i]->detach();
    }

    for (i = 0; i < RECV_THREAD_NUM; i++) {
        m_recv_threads[i].reset(new std::thread(std::bind(&Httpserver::recv_request, this)));
        //m_recv_threads[i]->detach();
    }

    for (i = 0; i < PROCESS_THREAD_NUM; i++) {
        m_process_threads[i].reset(new std::thread(std::bind(&Httpserver::do_protocol, this)));
        //m_process_threads[i]->detach();
    }
}

//结束时回收httpserver资源
void Httpserver::uninit()
{
    LOG_MSG("enter Httpserver::uninit\n");
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
    int listenfd = m_listen_conn->get_fd();
    ret = remove_from_epoll(m_epollfd, m_listen_conn);
    if (ret < 0) {
        LOG_ERROR("remove_from_epoll error, ret:%d, fd:%d, errno:%d\n", ret, listenfd, errno);
    }

    if (listenfd > 0) {
        shutdown(listenfd, SHUT_RDWR);
        close(listenfd);
    }
    Connection::free_conn(m_listen_conn);
    
    //释放notify 的sockpair
    ret = remove_from_epoll(m_epollfd, m_notify_wakeup_conn);
    if (ret < 0) {
        LOG_ERROR("remove_from_epoll failed, ret:%d, fd:%d, errno:%d\n", ret, m_notify_wakeup_conn->get_fd(), errno);
    }
    if (m_notify_wakeup_conn->get_fd() > 0)
        close(m_notify_wakeup_conn->get_fd());
    if (m_wakeup_fd > 0)
        close(m_wakeup_fd);
    Connection::free_conn(m_notify_wakeup_conn);

    if (m_epollfd > 0) 
        close(m_epollfd);
    
    m_epoll_events.clear();

    //释放所有队列中还未执行的请求或者响应
    if (!m_requests_queue.empty()) {
        for (auto& i : m_requests_queue) {
            Connection::free_conn(i);    
        }
    }

    if (!m_responses_queue.empty()) {
        for (auto& i : m_responses_queue) {
            Connection::free_conn(i);    
        }
    }
    
    if (!m_dataready_queue.empty()) {
        for (auto& i : m_dataready_queue) {
            Connection::free_conn(i);   
        }
    }

    m_requests_queue.clear();
    m_responses_queue.clear();
    m_dataready_queue.clear();

    LOG_MSG("leave Httpserver::uninit\n");
}

//发送函数
void Httpserver::send_reponse()
{
    LOG_MSG("send thread:%0x start...\n", std::this_thread::get_id());

    while (!m_exit) {
    }

    LOG_MSG("send thread:%0x exit...\n", std::this_thread::get_id());
}

//接收函数
void Httpserver::recv_request()
{
    LOG_MSG("recv thread:%0x start...\n", std::this_thread::get_id());
    bool ret;
    
    //线程循环
    while (!m_exit) {
        std::unique_lock<std::mutex> lock(m_dataready_lock);
        while (m_dataready_queue.empty()) {
            if (m_exit)
                goto EXIT;
            m_dataready_cond.wait(lock);
        }

        Connection* conn = m_dataready_queue.front();
        m_dataready_queue.pop_front();

        int fd = conn->get_fd();
        if (!Connection::assert_conn(conn)) {
            LOG_ERROR("get a invalid conn:%p, just ignore\n", conn);
            continue;
        }

        std::string buff;
        ret = Util::recv_data(fd, buff);
        if (!ret) {
            //连接有问题或对端关闭
            LOG_INFO("fd:%d is close\n", fd);
            Connection::free_conn(conn);
            continue;
        }
        //将buff放入conn的recv_buff
        conn->append_to_recv(buff);

        LOG_MSG("recv data = %s\n", conn->get_recv().c_str());
        LOG_MSG("recv data size = %d\n", conn->get_recv().size());

        //接收成功后，将conn放入m_rewuests_queue
        m_requests_lock.lock();
        m_requests_queue.push_back(conn);
        m_requests_cond.notify_one();
        m_requests_lock.unlock();     
    }

EXIT:
    LOG_MSG("recv thread:%0x exit...\n", std::this_thread::get_id());
}

//业务解析函数，包括从请求队列获取数据，解析，然后组好响应包，放入响应队列中
void Httpserver::do_protocol()
{
    LOG_MSG("process thread:%0x start...\n", std::this_thread::get_id());
    uint64_t content_length;
    int state;
    int header_end;
    int left_len;

    while (!m_exit) {
        std::unique_lock<std::mutex> lock(m_requests_lock);
        while (m_requests_queue.empty()) {
            if (m_exit)
                goto EXIT;
            m_requests_cond.wait(lock);
        }
        Connection* conn = m_requests_queue.front();
        m_requests_queue.pop_front();
        
        state = conn->get_state();
        if (!Connection::assert_conn(conn)) {
            LOG_ERROR("get a invalid conn:%p, just ignore\n", conn);
            continue;
        }

        std::string recvbuff = conn->get_recv();
        LOG_MSG("conn->recv = %s\n", recvbuff.c_str());

        if (state == CONN_STATE_INIT || state == CONN_STATE_RECV_HEADER) {
            //解析，是否已经接收了一个完整的HTTP 头部
            if (!conn->check_has_complete_header()) {
                //如果接收的头部大于4096个字节并且还没有结束，认为是不合法的请求，关闭
                if (recvbuff.size() > MAX_HEADER_SIZE) {
                    LOG_ERROR("conn:%p header is too large, not invalid, need close\n", conn);
                    Connection::free_conn(conn);
                    continue;
                }
                //如果没有，把conn重新放入epoll，继续接收
                LOG_MSG("conn:%p don't have a complete header, need recv\n", conn);
                conn->set_state(CONN_STATE_RECV_HEADER);
                goto ADD_EPOLL;
            }

            LOG_MSG("conn:%p get a complete header\n", conn);

            //解析header，判断是否合法
            if (!conn->parse_http_header()) {
                LOG_ERROR("conn %p header is invalid, just free\n", conn);
                Connection::free_conn(conn);
                continue;
            }
        }
        
        content_length = conn->get_request()->get_content_length();
        header_end = conn->get_header_end();
        LOG_MSG("content_length = %d\n", content_length);
        LOG_MSG("recvbuffsize = %d\n", recvbuff.size());
        LOG_MSG("cnn->header_end = %d\n", header_end);
        if (content_length > 0) {
            left_len = content_length - ((uint64_t)recvbuff.size() - header_end - 4);
            conn->set_content_left_len(left_len);
        }
    
        LOG_MSG("m_left_content_len = %d\n", conn->get_content_left_len());

        //判断是否有body有待接收？
        if (conn->get_content_left_len() > 0) {
            LOG_MSG("conn:%p need recv left body\n", conn);
            conn->set_state(CONN_STATE_RECV_BODY);
            goto ADD_EPOLL;
        }

        LOG_MSG("conn:%p get a complete request\n", conn);

        //接收完毕，表明已经接收了一个完整的HTTP请求
        //如果接收了连续的下一个HTTP请求的一部分，怎么办？ TODO
        if (!m_process_cb(conn)) {
            LOG_ERROR("conn:%p do_logic_operation failed, need free\n", conn);
            Connection::free_conn(conn);
            continue;
        }

        //makeup response
        //TODO
        Connection::free_conn(conn);
        continue;

ADD_EPOLL:
        m_modify_lock.lock();
        m_modify_queue.push_back(conn);
        m_modify_lock.unlock();
        send_notify(m_modify_fd);   
    }

EXIT:
    LOG_MSG("process thread:%0x exit...\n", std::this_thread::get_id());
}


//accept流程
int Httpserver::do_accept()
{
    struct sockaddr_in clientaddr;
    socklen_t addrlen;
    int ret;
    int newfd;
    int listenfd = m_listen_conn->get_fd();

    memset(&clientaddr, 0, sizeof(clientaddr));
    newfd = ::accept(listenfd, (struct sockaddr*)&clientaddr, &addrlen);
    if (newfd < 0) {
        LOG_ERROR("accept error, ret:%d, errno:%d\n", newfd, errno);
        return newfd;
    }
    
    //将newfd设置为非阻塞
    ret = Util::set_fd_nonblock(newfd);
    if (ret < 0) {
        LOG_ERROR("set_fd_nonblock error, ret:%d, fd:%d, errno:%d\n", ret, newfd, errno);
        close(newfd);
        return ret;
    }

    //set nodelay, forbidden nagle 
    ret = Util::set_no_use_nagle(newfd);
    if (ret < 0) {
        LOG_ERROR("set_no_use_nagle error, ret:%d, fd:%d, errno:%d\n", ret, newfd, errno);
        close(newfd);
        return ret;
    }

    //set send and recv timeout
    ret = Util::set_fd_send_recv_timeout(newfd);
    if (ret < 0) {
        LOG_ERROR("set_fd_send_recv_timeout error, ret:%d, fd:%d, errno:%d\n", ret, newfd, errno);
    }

    LOG_MSG("get a new connection, fd:%d, ip:%s, port:%d\n", newfd, inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));

    //将newfd加入epoll
    Connection* conn = new Connection(newfd, get_ptr());
    ret = add_to_epoll(m_epollfd, conn, 0);
    if (ret < 0) {
        LOG_ERROR("add_to_epoll error, ret:%d, epollfd:%d, fd:%d, errno:%d\n", ret, m_epollfd, newfd, errno);
        close(newfd);
        return ret;
    }

    return NET_SUCCESS;
}

//主线程循环函数
void Httpserver::main_loop()
{
    int ret;
    struct sockaddr_in servaddr;
    int nevents;
    int i;
    int listenfd;
 
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd < 0) {
        LOG_ERROR("socket error, ret:%d, errno:%d\n", listenfd, errno);
        return;
    }
    
    ret = Util::set_fd_nonblock(listenfd);
    if (ret < 0) {
        LOG_ERROR("set_fd_nonblock error, ret:%d, fd:%d, errno:%d\n", ret, listenfd, errno);
        return;
    }

    //set listenfd reuseaddr reuseport
    ret = Util::set_reuseaddr(listenfd);
    if (ret < 0) {
        LOG_ERROR("set_reuseaddr error, ret:%d, fd:%d, errno:%d", ret, listenfd, errno);
        return;
    }

    ret = Util::set_reuseport(listenfd);
    if (ret < 0) {
        LOG_ERROR("set_reuseport error, ret:%d, fd:%d, errno:%d", ret, listenfd, errno);
        return;
    }

    LOG_MSG("ip = %s\n", m_ip);
    LOG_MSG("port = %d\n", m_port);

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(m_port);
    servaddr.sin_addr.s_addr = inet_addr(m_ip);
    
    ret = ::bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr));
    if (ret == -1) {
        LOG_ERROR("bind error, ret:%d, fd:%d, errno:%d\n", ret, listenfd, errno);
        return;
    }

    ret = ::listen(listenfd, 50);
    if (ret < 0) {
        LOG_ERROR("listen error, ret:%d, fd:%d, errno:%d\n", ret, listenfd, errno);
        return;
    }

    m_epollfd = ::epoll_create(1);
    if (m_epollfd < 0) {
        LOG_ERROR("epoll_create error, ret:%d, errno:%d\n", m_epollfd, errno);
        return;
    }

    //将listen加入epoll
    LOG_INFO("listefd:%d\n", listenfd);
    m_listen_conn = new Connection(listenfd, get_ptr());
    ret = add_to_epoll(m_epollfd, m_listen_conn, 1);
    if (ret < 0) {
        LOG_ERROR("epoll_ctl error, ret:%d, epollfd:%d, fd:%d, errno:%d\n", ret, m_epollfd, listenfd, errno);
        return;
    }

    //创建wakeup_conn的sockpair,并加入epoll
    ret = set_sockpair_conn(&m_notify_wakeup_conn, &m_wakeup_fd);
    if (ret < 0) {
        LOG_ERROR("set_sockpair_conn failed, ret:%d, errno:%d\n", ret, errno);
        return;
    }

    //创建modify_conn的sockpair,并加入epoll
    ret = set_sockpair_conn(&m_notify_modify_conn, &m_modify_fd);
    if (ret < 0) {
        LOG_ERROR("set_sockpair_conn failed, ret:%d, errno:%d\n", ret, errno);
        return;
    }
    
    //loop
    while (!m_exit) {
        nevents = ::epoll_wait(m_epollfd, &*m_epoll_events.begin(), static_cast<int>(m_epoll_events.size()), -1);
        LOG_MSG("epoll_wait return, events:%d\n", nevents);
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
            if (m_epoll_events[i].data.ptr == static_cast<void*>(m_notify_wakeup_conn)) {
                LOG_MSG("get a wakeup conn event, fd:%d\n", m_notify_wakeup_conn->get_fd());
                recv_notify(m_notify_wakeup_conn);    
                if (m_exit)
                    break;
                continue;
            }

            if (static_cast<Connection*>(m_epoll_events[i].data.ptr) == m_notify_modify_conn) {
                LOG_MSG("get a modify conn event, fd:%d\n", m_notify_modify_conn->get_fd());
                recv_notify(m_notify_modify_conn);
                //从modify_queue中获取conn,然后重新设置，放入epoll中
                m_modify_lock.lock();
                if (m_modify_queue.empty()) {
                    m_modify_lock.unlock();
                    continue;
                }

                Connection* modify_conn = m_modify_queue.front();
                m_modify_queue.pop_front();
                m_modify_lock.unlock();
                ret = reset_conn_in_epoll(modify_conn);
                if (ret < 0) {
                    LOG_ERROR("reset_conn_in_epoll: failed, ret:%d, conn:%p, errno:%d\n", ret, modify_conn, errno);
                    Connection::free_conn(modify_conn);
                }
                continue;
            }

            if (static_cast<Connection*>(m_epoll_events[i].data.ptr) == m_listen_conn) {
                ret = do_accept();
                if (ret < 0) {
                    LOG_ERROR("do_accept error, ret:%d, fd:%d, errno:%d\n", ret, listenfd, errno);
                }
                continue;
            }
            
            Connection* conn = static_cast<Connection*>(m_epoll_events[i].data.ptr);
            m_dataready_lock.lock();
            m_dataready_queue.push_back(conn);
            m_dataready_cond.notify_one();
            m_dataready_lock.unlock();
        }
    }

    LOG_MSG("main_loop exit...\n");
}
