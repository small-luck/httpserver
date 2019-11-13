/*
 * 单例模式模板类
 * author:liupei
 * date:2019/11/11
 * */
#ifndef __SINGLETON_H__
#define __SINGLETON_H__

#include <memory>

//返回普通指针
template<typename T>
class Singleton {
public:
    static T* get_instance() {
        static T v;
        return &v;
    }
private:
    Singleton() = default;
};

//返回智能指针
template<typename T>
class Singletonptr {
public:
    static std::shared_ptr<T>& get_instance() {
        static std::shared_ptr<T> v = std::make_shared<T>(); 
        return v;
    }

/* private: */
/*     Singletonptr() = default; */
};


#endif /*__SINGLETON_H__*/
