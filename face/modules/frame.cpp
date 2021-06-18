#include <memory>

#include "frame.hpp"
#include "cnrt.h"


namespace facealign {

 void FAFrame::Pkt2Mat() {
    size_t y_size = cn_pkt_ptr->strides[0] * cn_pkt_ptr->height;
    size_t uv_size = y_size / 2;
    cpu_data = new uint8_t[y_size + uv_size];
    cnrtMemcpy(reinterpret_cast<void*>(cpu_data), cn_pkt_ptr->ptrs[0], y_size, CNRT_MEM_TRANS_DIR_DEV2HOST);
    cnrtMemcpy(reinterpret_cast<void*>(cpu_data + y_size), cn_pkt_ptr->ptrs[1], uv_size, CNRT_MEM_TRANS_DIR_DEV2HOST);
    image_ptr = std::make_shared<cv::Mat>(cn_pkt_ptr->height * 3 / 2, cn_pkt_ptr->strides[0], CV_8UC1, cpu_data);
    cv::cvtColor(*image_ptr, *image_ptr, cv::COLOR_YUV2BGR_NV21);
  }
  // void* FAFrame::GetFrameMluData() {
  //   return mlu_data;
  // }

  // void FrameOpencv::Cpu2Mlu() {
  //   //cnrtMemcopy(mlu_data, cpu_data, size, type);
  // }

  // void FrameOpencv::Mlu2Cpu() {
  //   //cnrtMemcopy(cpu_data, mlu_data, size, type)
  // }

  // void FrameOpencv::SetCpuPtr() {
  //   this->cpu_data = reinterpret_cast<void*>(image_data.data());
  // }

  // void FrameOpencv::SetMluPtr() {
  //   //cnrtMalloc(&Mlu2Cpu, size);
  //   //Cpu2Mlu();
  // }
}
