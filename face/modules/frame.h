#ifndef FRAME_HPP_
#define FRAME_HPP_

#include <iostream>
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"

#include "cnrt.h"

namespace facealign {
class Frame {
public:
  //Frame(cv::Mat image) : image_data(image) {}
  Frame() {}

  ~Frame() {};

  void* GetFrameCpuData();
  void* GetFrameMluData();

protected:
  void* cpu_data = nullptr;
  void* mlu_data = nullptr;
};

class FrameOpencv : public Frame {

public:
  explicit FrameOpencv(cv::Mat image) : image_data(image) {
    size_ = image_data.rows * image_data.cols 
      * image_data.channels * image_data.depth();
  }
  void Cpu2Mlu();
  void Mlu2Cpu();

  void SetCpuPtr();
  void SetMluPtr();

public:
  cv::Mat image_data;
  size_t size_ = 0;
};

} //namespace 
#endif //!FRAME_HPP_
