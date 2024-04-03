#pragma once

#include <tuple>
#include <optional>
#include <opencv2/opencv.hpp>
#include <rknn_api.h>
#include "type.h"
#include "timer.h"


// 为节省反复申请内存的开销，这个类的推理接口被设计为不能并发，但不同对象之间无所谓
class Yolov7
{
public:

    Yolov7(const std::string &modelPath);

    /** 设置类别名称 */
    bool SetClasses(const std::vector<std::string>& classes);

    /** 根据标签文件类别名称（每行一个）*/
    bool SetClasses(const std::string &labelPath);

    /** 获取类别信息*/
    const std::vector<std::string> &GetClasses()const;

    /** 设置Anchor的值，支持3*3*2=18个数值 */
    bool SetAnchors(const std::vector<float> &anchors);

    /** 根据Anchor文件设置，支持3*3*2=18个数值 */
    bool SetAnchors(const std::string &anchorPath);

    /** 设置物体置信度阈值 */
    bool SetObjThresh(float conf);

    /** 设置NMS过滤的IOU阈值 */
    bool SetNMSThresh(float conf);

    ResultType Detect(const cv::Mat &image);
    std::shared_ptr<ResultType> Detect(std::shared_ptr<const cv::Mat> image);

private:
    using Scale = std::tuple<double, cv::Size, cv::Vec4w>;

    // 模型相关
    rknn_context _modelCtx;
    bool _isQuant;
    // 输入
    rknn_input _input;
    // 输出
    std::vector<rknn_output> _output;  // rknn输出结构体
    std::vector<std::shared_ptr<void>> _outputBuf;  // 输出的预分配内存
    std::vector<rknn_tensor_attr> _outputInfo;  // 输出的信息（主要是如果有量化则需要scale和zp俩数据）

    std::vector<std::string> _classes;
    std::vector<float> _anchors;  // 当模型中没有grid操作时，就需要anchor来实现grid操作了
    uint _modelSize;
    float _objThresh = 0.25;
    float _nmsThresh = 0.45;
    cv::Size _cachedSize;
    Scale _cachedScale;  //缩放参数，如果提前设置好了检测就不需要重算了

    inline void RecalcScale(const cv::Size &size);

    cv::Mat Preprocess(const cv::Mat &image)const;

    template<typename DataType>
    ResultType Postprocess4Grid()const;

    template<typename DataType>
    ResultType Postprocess4Outs()const;

    void NMS(ResultType &objects)const;
};

inline const std::vector<std::string> &Yolov7::GetClasses()const
{
    return _classes;
}

inline void Yolov7::RecalcScale(const cv::Size &size)
{
    if(size == _cachedSize)
        return;
    if(static_cast<uint>(size.height) == _modelSize && static_cast<uint>(size.width) == _modelSize){
        _cachedScale = {1., {static_cast<int>(_modelSize), static_cast<int>(_modelSize)}, {0, 0, 0, 0}};
        return;
    }
    auto r = static_cast<double>(_modelSize) / std::max(size.width, size.height);
    cv::Size resize(static_cast<int>(r * size.width), static_cast<int>(r * size.height));
    
    auto pt = static_cast<ushort>((_modelSize - resize.height) / 2.);  // 上填充
    auto pl = static_cast<ushort>((_modelSize - resize.width) / 2.);  // 左填充
    // resize的缩放比例（给cv用），padding的上下左右
    _cachedScale = {r, resize, {pt, static_cast<ushort>(_modelSize - pt - resize.height), pl, static_cast<ushort>(_modelSize - pl - resize.width)}};
}

template<typename DataType>
ResultType Yolov7::Postprocess4Grid()const
{
    auto data = static_cast<DataType*>(_output[0].buf);
    auto [r, resize, padding] = _cachedScale;
    std::vector<Object> objects;

    size_t scopeSize = 5 + _classes.size();
    for (size_t i = 0; i < 25200; ++i) {
        size_t lineIndex = i * scopeSize;
        float objConf = data[lineIndex + 4];
        if (objConf < _objThresh)  // 按照物体置信度过滤
            continue;

        // 选最大置信度类别的物体，看是哪个类别
        float maxValue = 0;
        size_t maxIndex;
        for (size_t j = _classes.size(); j < scopeSize; ++j) {
            float value = data[lineIndex + j];
            if (value > maxValue) {
                maxIndex = j - 5;
                maxValue = value;
            }
        }
        if(maxValue < _objThresh)
            continue;
        float ow = data[lineIndex + 2], oh = data[lineIndex + 3];
        objects.emplace_back(Object{
            // 给中心点转换成左上点
            static_cast<int>((data[lineIndex] - ow / 2 - padding[2]) / r + 0.5),  
            static_cast<int>((data[lineIndex + 1] - oh / 2 - padding[0]) / r + 0.5), 
            static_cast<uint>(ow / r + 0.5), 
            static_cast<uint>(oh / r + 0.5),
            objConf * maxValue,  // 最终置信度 = 类别置信度 * 物体置信度
            maxIndex,  // 物体索引
            _classes[maxIndex]  // 物体名称
        });
    }
    NMS(objects);
    return objects;
}

inline static int32_t clip(float val, float min, float max)
{
    return val <= min ? min : (val >= max ? max : val);
}

template<typename T>
inline static float deqnt_affine_to_f32(T qnt, int32_t zp, float scale)
{
    return (static_cast<float>(qnt) - static_cast<float>(zp)) * scale; 
}

template<typename T>
inline static T qnt_f32_to_affine(float f32, int32_t zp, float scale)
{
    return static_cast<T>(f32);
}

template<>
inline int8_t qnt_f32_to_affine(float f32, int32_t zp, float scale)
{
    return static_cast<int8_t>(clip((f32 / scale) + zp, -128, 127));
}

template<typename T>
ResultType Yolov7::Postprocess4Outs()const
{
    ResultType objects;
    const int PROP_BOX_SIZE = 5 + _classes.size();
    auto [r, resize, padding] = _cachedScale;

    for(int oi = 0; oi < _output.size(); ++oi){
        int gridH = _outputInfo[oi].dims[2];
        int gridW = _outputInfo[oi].dims[3];
        int gridLen = gridH * gridW;
        float stride = _modelSize / gridH;
        auto &zp = _outputInfo[oi].zp;
        auto &scale = _outputInfo[oi].scale;
        auto anchor = _anchors.data() + oi * 6;
        T objthresh = qnt_f32_to_affine<T>(_objThresh, zp, scale);
        for (int a = 0; a < 3; ++a){
            for (int i = 0; i < gridH; ++i){
                for (int j = 0; j < gridW; ++j){
                    auto input = static_cast<T*>(_output[oi].buf);
                    int baseOffset = i * gridW + j;
                    T *inPtr = input + (PROP_BOX_SIZE * a) * gridLen + baseOffset;
                    T boxConf = input[(PROP_BOX_SIZE * a + 4) * gridLen + baseOffset];

                    if (boxConf < objthresh)
                        continue;
                        
                    T maxClassProbs = inPtr[5 * gridLen];
                    size_t maxClassId = 0;
                    for (int k = 1; k < _classes.size(); ++k)
                    {
                        T prob = inPtr[(5 + k) * gridLen];
                        if (prob > maxClassProbs)
                        {
                            maxClassId = k;
                            maxClassProbs = prob;
                        }
                    }
                    if (maxClassProbs < objthresh)
                        continue;
                    
                    float boxX = (deqnt_affine_to_f32(*inPtr, zp, scale)) * 2.0 - 0.5;
                    float boxY = (deqnt_affine_to_f32(inPtr[gridLen], zp, scale)) * 2.0 - 0.5;
                    float boxW = (deqnt_affine_to_f32(inPtr[2 * gridLen], zp, scale)) * 2.0;
                    float boxH = (deqnt_affine_to_f32(inPtr[3 * gridLen], zp, scale)) * 2.0;
                    boxX = (boxX + j) * stride;
                    boxY = (boxY + i) * stride;
                    boxW = boxW * boxW * anchor[a * 2];
                    boxH = boxH * boxH * anchor[a * 2 + 1];
                    boxX = boxX - boxW / 2 - padding[2];
                    boxY = boxY - boxH / 2 - padding[0];

                    objects.emplace_back(Object{
                        static_cast<int>(boxX / r + 0.5),  
                        static_cast<int>(boxY / r + 0.5), 
                        static_cast<uint>(boxW / r + 0.5), 
                        static_cast<uint>(boxH / r + 0.5),
                        deqnt_affine_to_f32(boxConf, zp, scale) * deqnt_affine_to_f32(maxClassProbs, zp, scale),
                        maxClassId,  // 物体索引
                        _classes[maxClassId]  // 物体名称
                    });
                }
            }
        }
    }
    NMS(objects);
    return objects;
}