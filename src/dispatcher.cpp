#include "dispatcher.h"

std::shared_ptr<Operation> DispatcherPut::get_op(std::string& url)
{
    if (url.empty())
        return nullptr;
    
    std::string target = url.substr(1);
    /* LOG_MSG("target = %s\n", target.c_str()); */
    
    if (target.compare("user") == 0) {
        return (std::make_shared<PutUser>());
    } else {
        return nullptr;
    }
}

std::shared_ptr<Operation> DispatcherGet::get_op(std::string& url)
{
    if (url.empty())
        return nullptr;
    
    std::string target = url.substr(1);
    /* LOG_MSG("target = %s\n", target.c_str()); */
    
    if (target.compare("user") == 0) {
        return (std::make_shared<GetUser>());
    } else {
        return nullptr;
    }
}

std::shared_ptr<Operation> DispatcherDelete::get_op(std::string& url)
{
    if (url.empty())
        return nullptr;
    
    std::string target = url.substr(1);
    /* LOG_MSG("target = %s\n", target.c_str()); */
    
    if (target.compare("user") == 0) {
        return (std::make_shared<DeleteUser>());
    } else {
        return nullptr;
    }
}
