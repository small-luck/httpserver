#include "protocol.h"
#include "log.h"
#include "net.h"

//业务解析函数，包括从请求队列获取数据，解析，然后组好响应包，放入响应队列中
bool Protocol::process_callback(Connection* conn)
{
    if (conn == nullptr || conn->get_state() == CONN_STATE_BAD || conn->get_fd() < 0) {
        LOG_ERROR("conn:%p is invalid\n");
        return false;
    }
    
    uint64_t content_length = conn->get_request()->get_content_length();
    std::string content;

    if (content_length > 0) {
        content = conn->get_recv().substr(conn->get_header_end() + 4);
        /* LOG_MSG("body size = %d, content = %s\n", content.size(), content.c_str()); */
    }

    //
    return true;
}

