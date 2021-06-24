#include <opencv2/opencv.hpp>
#include <opencv2/face.hpp>
#include "opencvextract.hpp"
#include "frame.hpp"

namespace facealign {
  static void drawLandmarks(cv::Mat& frame, std::vector<cv::Point2f>& landmarks) {
    for (size_t i = 0; i < landmarks.size(); ++i) {
      cv::circle(frame,landmarks[i],3,cv::Scalar(255, 200,0), cv::FILLED);
    }
  }

  bool OpencvExtract::Open(ModuleParamSet parameters) {
    if (parameters.find("path") == parameters.end()) {
      std::cerr << "path is not find! in opencvExtract" << std::endl;
      return false;
    }
    else {
      path_ = parameters["path"];
      facemark->loadModel(path_);
    }
    return true;
  }

  int OpencvExtract::Process(std::shared_ptr<FAFrameInfo> data) {
    static int c = 0;
   if (data->IsEos()) {
     RwLockWriteGuard lk(container_lock_);
     this->container_->RightMove();
     return 0;
   }
    auto frame = data->datas[0];
    if (frame->detect_objs.empty()) {
      std::cerr << "this frame don`t have face!" << std::endl;
      return 1;
    }
   // std::vector<cv::Mat> tmp;
    std::vector<cv::Rect> tmp_rect;
    for (auto& face : frame->detect_objs) {
     // tmp.push_back(face->obj);
      tmp_rect.push_back(face->rect);
    }
    std::vector<std::vector<cv::Point2f>> landmarks;
    bool sucess =  facemark->fit(*frame->image_ptr, tmp_rect, landmarks);
    if (sucess) {
      for (size_t i = 0; i < landmarks.size(); ++i) {
        drawLandmarks(*frame->image_ptr, landmarks[i]);
        std::string name = "res" + std::to_string(c);
        c++;
        name += ".jpg";
        cv::imwrite(name, *frame->image_ptr);
      }
    }
    return 0;
  }
}