#ifndef FRAME_HPP_
#define FRAME_HPP_

#include <iostream>
#include <functional>
#include "easycodec/vformat.h"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"


//#include "cnrt.h"

namespace facealign {

  struct DetectObjs {
    float x = 0.0;
    float y = 0.0;
    float w = 0.0;
    float h = 0.0;
    float score = 0.0;
    std::vector<cv::Point2f> fp;
    cv::Mat obj;
  };
  class FAFrame {
  public:
    //Frame(cv::Mat image) : image_data(image) {}
    explicit FAFrame(std::shared_ptr<cv::Mat> ptr) : image_ptr(ptr) {
      size_ = image_ptr->rows * image_ptr->cols
        * image_ptr->channels() * image_ptr->depth();
    }

    explicit FAFrame(std::shared_ptr<edk::CnFrame> pack) {
      cn_pkt_ptr = pack;
    }

  void Pkt2Mat();
    ~FAFrame() {
      if (back_codec_buff && cn_pkt_ptr)  {
        back_codec_buff(cn_pkt_ptr->buf_id);
        cn_pkt_ptr = nullptr;
      }
    }
    // void Cpu2Mlu();
    // void Mlu2Cpu();
  void setCallBackfunc(std::function<void(uint64_t)> ptr) {
      back_codec_buff = std::move(ptr);
  }
    // void SetCpuPtr();
    // void SetMluPtr();

  public:
    std::function<void(uint64_t)> back_codec_buff = nullptr;
    std::vector<std::shared_ptr<DetectObjs>> detect_objs;
    std::shared_ptr<cv::Mat> image_ptr = nullptr;
    std::shared_ptr<cv::Mat> detect_post_ptr = nullptr;
    std::shared_ptr<edk::CnFrame> cn_pkt_ptr = nullptr;
    size_t size_ = 0;

    // void* GetFrameCpuData();
    // void* GetFrameMluData();

  protected:
     uint8_t* cpu_data = nullptr;
    // uint8_t* mlu_data = nullptr;
  };

  // class FrameOpencv : public FAFrame {
  // public:
  //   explicit FrameOpencv(std::shared_ptr<cv::Mat> ptr) : image_ptr(ptr) {
  //     size_ = image_ptr->rows * image_ptr->cols
  //       * image_ptr->channels() * image_ptr->depth();
  //   }
  //   void Cpu2Mlu();
  //   void Mlu2Cpu();

  //   void SetCpuPtr();
  //   void SetMluPtr();

  // public:
  //   std::vector<std::shared_ptr<DetectObjs>> detect_objs;
  //   std::shared_ptr<cv::Mat> image_ptr = nullptr;
  //   std::shared_ptr<cv::Mat> detect_post_ptr = nullptr;
  //   std::shared_ptr<edk::CnPacket> cn_pkt_ptr = nullptr;
  //   size_t size_ = 0;
  // };

} //namespace 
#endif //!FRAME_HPP_
