#include "detect/yolov7.h"
#include <exception>
#include <fstream>
#include <spdlog/spdlog.h>
#include "timer.hpp"

Yolov7::Yolov7(const std::string &modelPath)
{
    if(rknn_init(&_modelCtx, const_cast<char*>(modelPath.c_str()), 0, 0, nullptr) != 0){
        spdlog::critical("模型加载失败，模型路径为：{}", modelPath);
        throw std::invalid_argument("模型加载失败");
    }
    
    rknn_input_output_num io_num;
    if(rknn_query(_modelCtx, RKNN_QUERY_IN_OUT_NUM, &io_num, sizeof(io_num)) != RKNN_SUCC){
        spdlog::critical("模型输入输出数量查询失败");
        throw std::invalid_argument("模型加载失败");
    }
    if(io_num.n_input != 1){
        spdlog::critical("模型的输入数量应该为1，但该模型为：{}", io_num.n_input);
        throw std::invalid_argument("模型加载失败");
    }

    rknn_tensor_attr tensorInfo;
    tensorInfo.index = 0;
    if(rknn_query(_modelCtx, RKNN_QUERY_INPUT_ATTR, &tensorInfo, sizeof(tensorInfo)) != 0){
        spdlog::critical("模型输入张量查询失败");
        throw std::invalid_argument("模型加载失败");
    }
    if(tensorInfo.n_dims != 4){
        spdlog::critical("模型输入维度应该为4，但指定的模型为: {}", tensorInfo.n_dims);
        throw std::invalid_argument("模型加载失败");
    }
    if(tensorInfo.dims[1] != tensorInfo.dims[2]){
        spdlog::critical("目前只支持宽高相同的模型输入形状，但指定的模型为[{}*{}]", tensorInfo.dims[2], tensorInfo.dims[1]);
        throw std::invalid_argument("模型加载失败");
    }
    _modelSize = tensorInfo.dims[2];
    
    _input.index = 0;
    _input.type = RKNN_TENSOR_UINT8;
    _input.size = 3 * _modelSize * _modelSize;
    _input.pass_through = false;
    _input.fmt = RKNN_TENSOR_NHWC;
    
    _output.resize(io_num.n_output);
    _outputBuf.resize(io_num.n_output);
    _outputInfo.resize(io_num.n_output);
    for(size_t i = 0; i<io_num.n_output; ++i){
        tensorInfo.index = i;
        if(rknn_query(_modelCtx, RKNN_QUERY_OUTPUT_ATTR, &tensorInfo, sizeof(tensorInfo)) != 0){
            spdlog::critical("模型输出张量查询失败");
            throw std::invalid_argument("模型加载失败");
        }
        auto &output = _output[i];
        auto &buf = _outputBuf[i];
        output.index = i;
        output.is_prealloc = true;
        output.size = tensorInfo.size;
        buf.reset(new uint8_t[output.size]);
        output.buf = buf.get();
        _outputInfo[i] = tensorInfo;
    }
    _isQuant = tensorInfo.qnt_type == RKNN_TENSOR_QNT_AFFINE_ASYMMETRIC and tensorInfo.type == RKNN_TENSOR_INT8;

    if(_output.size() == 1){
        if(tensorInfo.n_dims != 3){
            spdlog::critical("模型输出形状检测错误，当输出有1个时每个输出应有3个维度，但当前维度为：{}", tensorInfo.n_dims);
            throw std::invalid_argument("模型加载失败");
        }
        _classes.resize(tensorInfo.dims[2] - 5);
    }
    else{
        if(tensorInfo.n_dims != 4){
            spdlog::critical("模型输出形状检测错误，当输出有3个时每个输出应有4个维度，但当前维度为：{}", tensorInfo.n_dims);
            throw std::invalid_argument("模型加载失败");
        }
        _classes.resize(tensorInfo.dims[1] / 3 - 5);
    }
    if (_isQuant && tensorInfo.n_dims == 1)
        spdlog::warn("当前模型为量化模型，但模型中集成了Grid后处理操作，可能会导致因精度原因而检测不出任何物体");

    for(size_t i=0; i<_classes.size(); ++i)
        _classes[i] = std::to_string(i);
}

bool Yolov7::SetClasses(const std::vector<std::string>& classes)
{
    if(classes.size() != _classes.size())
        return false;
    _classes = classes;
    _classes.shrink_to_fit();
    return true;
}

bool Yolov7::SetClasses(const std::string& labelPath)
{
    std::ifstream labelFile(labelPath);
    std::string label;
    std::vector<std::string> labels;
    labels.reserve(_classes.size());
    while(labelFile >> label)
        if(!label.empty())
            labels.push_back(std::move(label));
    if(labels.size() != _classes.size())
        return false;
    _classes = std::move(labels);
    return true;
}

bool Yolov7::SetAnchors(const std::vector<float> &anchors)
{
    if (anchors.size() != 18){
        spdlog::error("目前只支持设置3个输出头的Anchor，并且每组Anchor只能有3个，共计18个数值，但传入了：{}个", anchors.size());
        return false;
    }
    _anchors = anchors;
    _anchors.shrink_to_fit();
    return true;
}

bool Yolov7::SetAnchors(const std::string &anchorPath)
{
    std::ifstream labelFile(anchorPath);
    float value;
    decltype(_anchors) anchors;
    anchors.reserve(18);
    while(labelFile >> value)
        anchors.push_back(value);
    if (anchors.size() != 18){
        spdlog::error("目前只支持设置3个输出头的Anchor，并且每组Anchor只能有3个，共计18个数值，但文件中有的数值有：{}个", anchors.size());
        return false;
    }
    _anchors = std::move(anchors);
    return true;
}

bool Yolov7::SetObjThresh(float conf)
{
    if(conf > 0 && conf < 1){
        _objThresh = conf;
        return true;
    }
    return false;
}

/**
 * @brief 设置NMS阈值
 *
 * 设置YOLOv7的NMS（非极大值抑制）阈值。
 *
 * @param conf NMS阈值
 *
 * @return 设置成功返回true，否则返回false
 */
bool Yolov7::SetNMSThresh(float conf)
{
    if(conf > 0 && conf < 1){
        _nmsThresh = conf;
        return true;
    }
    return false;
}

/**
 * @brief 在给定图像上进行目标检测
 *
 * 将输入图像进行预处理，并使用YOLOv7模型进行目标检测。
 *
 * @param image 输入图像
 *
 * @return 返回检测结果
 */
ResultType Yolov7::Detect(const cv::Mat &image)
{
    static FPSCounter counter("目标检测");
    counter.Update();

    RecalcScale(image.size());
    auto dstImage = Preprocess(image);
    _input.buf = dstImage.data;
    rknn_inputs_set(_modelCtx, 1, &_input);
    rknn_run(_modelCtx, nullptr);

    rknn_outputs_get(_modelCtx, _output.size(), _output.data(), nullptr);
    if(_output.size() == 1){  // 只有一个输出，说明grid操作在模型里面
        if(_isQuant)
            return Postprocess4Grid<int8_t>();
        return Postprocess4Grid<float16_t>();
    }
    if(_isQuant)
        return Postprocess4Outs<int8_t>();
    return Postprocess4Outs<float16_t>();
}

/**
 * @brief 检测图像中的目标
 *
 * 使用 YOLOv7 模型检测给定图像中的目标，并返回一个包含检测结果的智能指针。
 *
 * @param image 输入的图像
 *
 * @return 包含检测结果的智能指针
 */
std::shared_ptr<ResultType> Yolov7::Detect(std::shared_ptr<const cv::Mat> image)
{
    return std::make_shared<ResultType>(Detect(*image));
}

/**
 * @brief 对图像进行预处理
 *
 * 使用 YOLOv7 模型对给定的图像进行预处理，包括缩放、填充和颜色空间转换。
 *
 * @param image 输入的图像
 *
 * @return 预处理后的图像
 */
cv::Mat Yolov7::Preprocess(const cv::Mat &image)const
{
    cv::Mat dst;
    auto [r, resize, padding] = _cachedScale;
    cv::resize(image, dst, resize, 0, 0);
    cv::copyMakeBorder(dst, dst, padding[0], padding[1], padding[2], padding[3], cv::BORDER_CONSTANT, cv::Scalar(114, 114, 114));
    cv::cvtColor(dst, dst, cv::COLOR_BGR2RGB);
    return dst;
}

/**
 * @brief 非极大值抑制
 *
 * 对检测结果进行非极大值抑制处理，去除重叠度较高的检测框。
 *
 * @param objects 检测结果
 */
void Yolov7::NMS(ResultType &objects)const
{
    ResultType result;
    // 按照得分从高到低排序
    std::sort(objects.begin(), objects.end(), [](auto &obj1, auto obj2){return obj1.score > obj2.score;});
    while (!objects.empty()) {
        auto &base = objects.front();
        for (auto cmpIter = ++objects.begin(); cmpIter != objects.end();) {
            auto &cmp = *cmpIter;

            // 将点按 maxLength * boxes[0].classIndex 平移，以避免与不同类别的两个框发生碰撞
            // translate point by maxLength * boxes[0].classIndex to
            // avoid bumping into two boxes of different classes
            float trans = static_cast<float>(_modelSize * base.classIndex);
            float baseX = base.box.x + trans;
            float baseY = base.box.y + trans;
            float cmpX = cmp.box.x + trans;
            float cmpY = cmp.box.y + trans;

            // 两个框的重叠部分
            // the overlapping part of the two boxes
            float xLeft = std::max(baseX, cmpX);
            float yTop = std::max(baseY, cmpY);
            float xRight = std::min(baseX + base.box.w, cmpX + cmp.box.w);
            float yBottom = std::min(baseY + base.box.h, cmpY + cmp.box.h);
            float area = std::max(0.0f, xRight - xLeft) * std::max(0.0f, yBottom - yTop);
            float iou =  area / (base.box.w * base.box.h + cmp.box.w * cmp.box.h - area);

            // 根据NMS阈值过滤框
            // filter boxes by NMS threshold
            if (iou > _nmsThresh) 
                cmpIter = objects.erase(cmpIter);
            else
                ++cmpIter;
        }
        result.push_back(std::move(base));
        objects.erase(objects.begin());
    }
    objects = std::move(result);
}