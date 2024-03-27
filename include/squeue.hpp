#pragma once

#include <thread>
#include <mutex>
#include <shared_mutex>
#include <condition_variable>
#include <optional>

// 模板类必须包含默认构造函数，复制构造函数（取出时会进行复制构造）、=运算符号（分情况根据需求可实现左值/右值版本）
template<typename T>
class SQueue
{
public:
    using IdType = int;
    using DataType = T;
    static IdType DEFAULT_ID_VALUE;

    SQueue(): _id(DEFAULT_ID_VALUE) {}
    SQueue(const SQueue&)=delete;
    SQueue(SQueue&&)=delete;
    
    // 可传入一个左值或者一个右值
    template<typename TP>
    void Put(TP &&t){
        std::unique_lock<std::shared_mutex> lock(_mutex);
        _value = std::forward<TP>(t);
        ++_id;
        _cond_for_new.notify_all();
    }
    
    T Get(IdType &id)const{
        std::shared_lock<std::shared_mutex> lock(_mutex);
        if(id == _id)
            _cond_for_new.wait(lock);
        id = _id;
        return _value;
    }
    
    std::optional<T> GetNoWait(IdType &id)const{
        std::optional<T> ret;
        std::shared_lock<std::shared_mutex> lock(_mutex);
        if(id != _id){
            id = _id;
            ret = _value;
        }
        return ret;
    }

private:
    IdType _id;
    T _value;
    mutable std::shared_mutex _mutex;
    mutable std::condition_variable_any _cond_for_new;
};
template<typename T>
typename SQueue<T>::IdType SQueue<T>::DEFAULT_ID_VALUE = 0;