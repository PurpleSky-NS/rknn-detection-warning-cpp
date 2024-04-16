#pragma once

#include <thread>
#include <mutex>
#include <shared_mutex>
#include <condition_variable>
#include <optional>
#include <queue>
#include <spdlog/spdlog.h>


/** 父类的东西，子类不using就报错？*/
template<typename... Tps>
class BaseQueue
{
public:
    struct DataID{
        int id = 0;
    };
    // 加上shared_ptr的类型元组
    using ValueTupleType = std::tuple<std::shared_ptr<Tps>...>;
    // 上述元组中的第一个类型
    using ValueType = typename std::tuple_element<0, ValueTupleType>::type;
    // 不加shared_ptr的类型元组
    using DataTupleType = std::tuple<Tps...>;
    // 上述元组中的第一个类型
    using DataType = typename std::tuple_element<0, DataTupleType>::type;

    BaseQueue() = default;
    BaseQueue(const BaseQueue &) = default;
    BaseQueue(BaseQueue &&) = default;
    
    /** 放入数据，使用shared_ptr避免底层数据多次拷贝（比起让底层数据自己实现拷贝/移动操作，这么做更好）*/
    virtual void Put(std::shared_ptr<Tps>... svalues) = 0;
    
    /** 阻塞取出数据，返回一个tuple，可自取需要的数据*/
    virtual ValueTupleType Get(DataID &id) = 0;
    
    /** 非阻塞取出数据，返回一个tuple，可自取需要的数据*/
    virtual std::optional<ValueTupleType> GetNoWait(DataID &id) = 0;

protected:
    DataID _id;
    std::shared_mutex _mutex;
    std::condition_variable_any _condForNew;
};

/** 
 * 保留多个数据并不断更新的队列，和普通队列不同之处在于，如果这玩意被塞满了，会将所有缓存都清空，再接收新的数据
 * 这么做的意义在于，如果接收端处理速度不足，不管数据是恰头还是去尾，都会使接收端收到的数据不连续，这样可以保证接收端尽可能接收到连续的数据
 */
template<size_t MAX_SIZE, typename... Tps>
class TQueue: public BaseQueue<Tps...>
{
public:
    using DataID = typename BaseQueue<Tps...>::DataID;
    using ValueTupleType = typename BaseQueue<Tps...>::ValueTupleType;
    using ValueType = typename BaseQueue<Tps...>::ValueType;
    using DataTupleType = typename BaseQueue<Tps...>::DataTupleType;
    using DataType = typename BaseQueue<Tps...>::DataType;

    TQueue() = default;
    TQueue(const TQueue&) = default;
    TQueue(TQueue&&) = delete;
    
    void Put(std::shared_ptr<Tps>... svalues){
        std::unique_lock<std::shared_mutex> lock(_mutex);
        if(_valueQueue.size() == MAX_SIZE){
            spdlog::debug("数据队列已达到上限[{}]，清空所有缓存的数据", "MAX_SIZE");
            _valueQueue.clear();
            _id.id += MAX_SIZE;
        }
        _valueQueue.emplace_back(std::move(svalues)...);
        _condForNew.notify_all();
    }
    
    ValueTupleType Get(DataID &id){
        std::shared_lock<std::shared_mutex> lock(_mutex);
        if(_valueQueue.empty())
            _condForNew.wait(lock);
        return GetValue(id);
    }
    
    std::optional<ValueTupleType> GetNoWait(DataID &id){
        std::shared_lock<std::shared_mutex> lock(_mutex);
        if(_valueQueue.empty())
            return std::nullopt;
        return GetValue(id);
    }

protected:
    using BaseQueue<Tps...>::_id;  // 这玩意总是存储存队列首部数据的ID
    using BaseQueue<Tps...>::_mutex;
    using BaseQueue<Tps...>::_condForNew;
    std::deque<ValueTupleType> _valueQueue;

    ValueTupleType GetValue(DataID &id){
        auto value = std::move(_valueQueue.front());
        _valueQueue.pop_front();
        ++_id.id;
        id = _id;
        return value;
    }
};

/** 
 * 维护单数据的队列，可以反复取出数据，通过ID辨别取数据的对象是否已经拿到了最新数据，如果拿到了就等待
 * 多个线程可以同时调用Get，且那个线程只会拿到属于那个线程的最新数据
 * 对比上面那个队列的不同在于：1.窗口大小为1；2.数据就算被拿走了也不会被清空，适合多个线程同时取数据的情况
 */
template<typename... Tps>
class SQueue: public BaseQueue<Tps...>
{
public:
    using DataID = typename BaseQueue<Tps...>::DataID;
    using ValueTupleType = typename BaseQueue<Tps...>::ValueTupleType;
    using ValueType = typename BaseQueue<Tps...>::ValueType;
    using DataTupleType = typename BaseQueue<Tps...>::DataTupleType;
    using DataType = typename BaseQueue<Tps...>::DataType;

    SQueue() = default;
    SQueue(const SQueue&) = default;
    SQueue(SQueue&&) = default;
    
    virtual void Put(std::shared_ptr<Tps>... svalues){
        std::unique_lock<std::shared_mutex> lock(_mutex);
        _values = std::make_tuple(svalues...);
        ++_id.id;
        _condForNew.notify_all();
    }
    
    ValueTupleType Get(DataID &id){
        std::shared_lock<std::shared_mutex> lock(_mutex);
        if(id.id == _id.id)
            _condForNew.wait(lock);
        return GetValue(id);
    }
    
    std::optional<ValueTupleType> GetNoWait(DataID &id){
        std::shared_lock<std::shared_mutex> lock(_mutex);
        if(id.id == _id.id)
            return std::nullopt;
        return GetValue(id);
    }

protected:
    using BaseQueue<Tps...>::_id;
    using BaseQueue<Tps...>::_mutex;
    using BaseQueue<Tps...>::_condForNew;
    ValueTupleType _values;

    ValueTupleType GetValue(DataID &id){
        id = _id;
        return _values;
    }
};

/** 一个分发器，可以将一个数据分发到多个/种队列上 */
template<typename... Tps>
class Dispatcher
{
public:
    /** 用可变模板+列表初始化可以让构造函数接受若干个队列...*/
    template <typename... QueueTypes>
    Dispatcher(QueueTypes&... queues): _queues({&queues...}){}
    Dispatcher(const Dispatcher&) = default;
    Dispatcher(Dispatcher&&) = default;
    
    void Put(std::shared_ptr<Tps>... svalues){
        for(auto &queue: _queues)
            queue->Put(svalues...);
    }
private:
    std::vector<BaseQueue<Tps...>*> _queues;
};