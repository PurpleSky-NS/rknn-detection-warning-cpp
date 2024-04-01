#pragma once

#include <thread>
#include <mutex>
#include <shared_mutex>
#include <condition_variable>
#include <optional>

template<typename... Tps>
class SQueue
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

    SQueue() = default;
    SQueue(const SQueue&)=delete;
    SQueue(SQueue&&)=delete;
    
    // 可传入一个左值或者一个右值
    template<typename... SPs>
    void Put(SPs&&... values){
        std::unique_lock<std::shared_mutex> lock(_mutex);
        _values = std::make_tuple(values...);
        ++_id.id;
        _condForNew.notify_all();
    }
    
    ValueTupleType Get(DataID &id)const{
        std::shared_lock<std::shared_mutex> lock(_mutex);
        if(id.id == _id.id)
            _condForNew.wait(lock);
        id.id = _id.id;
        return _values;
    }
    
    std::optional<ValueTupleType> GetNoWait(DataID &id)const{
        std::shared_lock<std::shared_mutex> lock(_mutex);
        if(id.id != _id.id){
            id.id = _id.id;
            return _values;
        }
        return std::nullopt;
    }

private:
    DataID _id;
    ValueTupleType _values;
    mutable std::shared_mutex _mutex;
    mutable std::condition_variable_any _condForNew;
};