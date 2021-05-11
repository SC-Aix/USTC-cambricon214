#ifndef FRAME_HPP_
#define FRAME_HPP_

#include <iostream>
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"

//#include "cnrt.h"

namespace facealign {
class FAFrame {
public:
  //Frame(cv::Mat image) : image_data(image) {}
  FAFrame() {}

  ~FAFrame() {}

  void* GetFrameCpuData();
  void* GetFrameMluData();

protected:
  void* cpu_data = nullptr;
  void* mlu_data = nullptr;
};

class FrameOpencv : public FAFrame {

public:
  explicit FrameOpencv(std::shared_ptr<cv::Mat> ptr) : image_ptr(ptr) {
    size_ = image_ptr->rows * image_ptr->cols 
      * image_ptr->channels() * image_ptr->depth();
  }
  void Cpu2Mlu();
  void Mlu2Cpu();

  void SetCpuPtr();
  void SetMluPtr();

public:
  std::shared_ptr<cv::Mat> image_ptr;
  std::shared_ptr<cv::Mat> detect_post_ptr;
  size_t size_ = 0;
};

} //namespace 
#endif //!FRAME_HPP_
