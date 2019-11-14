#include "protocol.h"
#include "log.h"

//获取dispatcher具体实例
Dispatcher* Protocol::get_dispatcher(std::string& method)
{
    if (method.empty())
        return nullptr;

    if (method.compare("PUT") == 0) {
        return (new DispatcherPut());
    } else if (method.compare("GET") == 0) {
        return (new DispatcherGet());
    } else if (method.compare("DELETE") == 0) {
        return (new DispatcherDelete());
    } else {
        return nullptr;
    }
}

//业务解析函数，包括从请求队列获取数据，解析，然后组好响应包，放入响应队列中
bool Protocol::process_callback(Connection* conn)
{
    if (conn == nullptr || conn->get_fd() <= 0) {
        LOG_ERROR("get a freed conn\n");
        return true;
    }

    if (conn->get_state() == CONN_STATE_BAD) {
        LOG_ERROR("conn:%p is bad connection, need free\n");
        return false;
    }
    
    uint64_t content_length = conn->get_request()->get_content_length();
    std::string content;
    int ret;

    if (content_length > 0) {
        content = conn->get_recv().substr(conn->get_header_end() + 4);
        /* LOG_MSG("body size = %d, content = %s\n", content.size(), content.c_str()); */
    }

    std::string method = conn->get_request()->get_method();
    std::string url = conn->get_request()->get_url();
    LOG_MSG("method = %s\n", method.c_str());
    LOG_MSG("url = %s\n", url.c_str());
    Dispatcher* disp = Protocol::get_dispatcher(method);
    if (disp == nullptr) {
        //TODO 这里可以返回响应给客户端，表示内部错误
        LOG_ERROR("conn:%p provide unsupport HTTP method:%d\n", conn, method.c_str());
        return false;
    }
    
    std::shared_ptr<Operation> op = disp->get_op(url);
    if (op == nullptr) {
        //TODO 这里可以返回响应给客户端，表示内部错误
        LOG_ERROR("conn:%p provide unsupport HTTP url:%d\n", conn, url.c_str());
        return false;
    }
    
    op->init(conn->get_request(), conn->get_response());
    ret = op->execute();
    if (ret != NET_SUCCESS) {
        //TODO 这里可以返回响应给客户端，表示内部错误
        LOG_ERROR("coon:%p do execute failed, ret:%d\n", conn, ret);
        return false;
    }
    return true;
}

