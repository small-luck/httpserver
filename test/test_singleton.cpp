#include <iostream>
#include "../inc/singleton.h"

class Test : public Singletonptr<Test>{
    public:
    private:
        std::string m_name = "test";
};
int main()
{
    std::shared_ptr<Test> t1 = Test::get_instance();
    std::cout << "t1 = " << t1 << std::endl;
    std::shared_ptr<Test> t2 = Test::get_instance();
    std::cout << "t2 = " << t2 << std::endl;
    std::shared_ptr<Test> t3 = Test::get_instance();
    std::cout << "t3 = " << t3 << std::endl;
    
    std::shared_ptr<Test> t4 = std::make_shared<Test>();
    std::cout << "t4 = " << t4 << std::endl;
    return 0;
}

