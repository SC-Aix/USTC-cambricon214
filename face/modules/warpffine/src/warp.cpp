#include "warp.hpp"

namespace facealign {
  int OpencvWarp::Process(std::shared_ptr<FAFrameInfo> data) {
    if (data->IsEos()) {
     RwLockWriteGuard lk(container_lock_);
     this->container_->RightMove();
     return 0;
   }
    auto frame = reinterpret_cast<FrameOpencv*>(data->datas[0].get());
    if (frame->detect_objs.empty()) return 0;
    GetTransFormMatrix(data);
    for (size_t i = 0; i < frame->detect_objs.size(); ++i) {
      
      //warpPerspective(frame->detect_objs[0]->obj, 
      //                frame->detect_objs[0]->obj, 
      //                src_points[i], frame->detect_objs[0]->obj.size(), cv::INTER_LINEAR, cv::BORDER_CONSTANT);
      cv::warpAffine(frame->detect_objs[0]->obj, 
                     frame->detect_objs[0]->obj, 
                     src_points[i], frame->detect_objs[0]->obj.size());
    }
    return 1;
  }

  bool OpencvWarp::GetTransFormMatrix(std::shared_ptr<FAFrameInfo> data) {
    auto frame = reinterpret_cast<FrameOpencv*>(data->datas[0].get());
    for (auto obj : frame->detect_objs) {
      int width = obj->obj.cols;
      int height = obj->obj.rows;
      std::vector<cv::Point2f> tmp;
      tmp.push_back(cv::Point2f(width / 3.5, height / 3.5));
      tmp.push_back(cv::Point2f(width * (1 - 1 / 3.5), height / 3.5));
      tmp.push_back(cv::Point2f(width / 2.0, height / 2.0));
      //tmp.push_back(cv::Point2f(width / 2.0, height * 2.5 / 4.0));
      //cv::Mat t = cv::getPerspectiveTransform(obj->fp, tmp);
      cv::Mat t = cv::getAffineTransform(obj->fp, tmp);
      src_points.emplace_back(std::move(t));
    }
    return false;
  }
}