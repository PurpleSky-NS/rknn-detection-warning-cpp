#include "alert/trigger/trigger.h"

#include <json/json.h>
#include "alert/trigger/enter.hpp"
 
Trigger::Trigger(const std::string &event, const std::string &object, const std::string &region):
    _event(event), _object(object), _region(region)
{
    spdlog::debug("事件[{}]已添加，关联对象为[{}]，对应区域为：[{}]", event, object, region.empty() ? "整个画面" : region);
}
    
// 调用这个接口报警
void Trigger::Alert(const std::vector<STracker> &objs, std::shared_ptr<cv::Mat> image)
{
    for(auto &t: objs)
        spdlog::debug("[({}){}] ({}) 区域[{}]发生事件[{}]", _object, t->GetID(), objs.size(), _region, _event);
}

STrigger Trigger::ParseTrigger(const std::string &triggerInfo)
{
    Json::Reader reader;
	Json::Value root;
    
    if(!reader.parse(triggerInfo, root)){
        spdlog::critical("解析事件触发器Json字符串失败，事件配置为：[{}]", triggerInfo);
        throw std::invalid_argument("事件触发器构造失败");
    }
    auto condition = root["condition"].asString();
    auto event = root["event"].asString();
    auto object = root["object"].asString();
    std::string region;

    if(auto regionValue = root["region"];!regionValue.isNull())
        region = regionValue.asString();
        
    auto argsValue = root["args"];

    if(condition == "出现")
        return STrigger(new EnterTrigger(event, object, region));
    
    spdlog::critical("生成事件触发器失败，未知的触发条件[{}]（事件名称为[{}]，对象为[{}]）", condition, event, object);
    throw std::invalid_argument("构造事件触发器失败");
    // event, object, args, region = alert_config['event'], alert_config['object'], alert_config['args'] or [], alert_config.get('region')

    // if condition == '出现':
    //     return EnterTrigger(event, object, region)
    // elif condition == '离开':
    //     return LeaveTrigger(event, object, region)
    // elif condition == '滞留':
    //     return StayTrigger(event, object, region, *args)
    // elif condition.endswith('经过'):
    //     return PassTrigger(event, object, region, condition)
    // elif '等于' in condition \
    //     or '小于' in condition \
    //     or '大于' in condition :
    //     return CountTrigger(event, object, region, condition, *args)
    // raise RuntimeError(f'未知报警条件：{condition}')
}