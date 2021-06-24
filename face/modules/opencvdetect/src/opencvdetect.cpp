#include "opencvdetect.hpp"

#include <iostream>

#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/objdetect/objdetect.hpp>
#include "face_module.hpp"
#include "frame.hpp"


namespace facealign {
 bool OpencvDetect::Open(ModuleParamSet parameters) {
    if (parameters.find("path") == parameters.end()) {
      std::cerr << "path is not find!" << std::endl;
      return false;
    } else {
      path_ = parameters["path"];
      cascade->load(path_);
    }
    return true;
  }

 int OpencvDetect::Process(std::shared_ptr<FAFrameInfo> data) {
   if (data->IsEos()) {
     RwLockWriteGuard lk(container_lock_);
     this->container_->RightMove();
     return 0;
   }
    auto frame = data->datas[0];
    auto img = frame->image_ptr;
    std::vector<cv::Rect> faces;
    cv::Mat gray,smallImg( cvRound (img->rows/scale), cvRound(img->cols/scale), CV_8UC1 );
    cv::cvtColor(*img, gray, cv::COLOR_BGR2GRAY);
    cv::resize(gray, smallImg, smallImg.size(),0, 0, cv::INTER_LINEAR);
    cv::equalizeHist(smallImg, smallImg);
    cascade->detectMultiScale(smallImg, faces, 1.1, 3, 2, cv::Size(30,30));
    for( std::vector<cv::Rect>::const_iterator r = faces.begin(); r != faces.end(); r++) {
        // TODO 是否需要clone处理，防止后续对图片的操作影响obj中的数据成员
        auto objs = std::make_shared<DetectObjs>();
        int b_x = std::max(static_cast<int>(r->y * scale), 0);
        int b_x_ = std::min(static_cast<int>(r->height * scale), img->rows);
        int b_y = std::max(static_cast<int>(r->x * scale), 0);
        int b_y_ = std::min(static_cast<int>(r->width * scale), img->cols);
        objs->x = b_y;
        objs->y = b_x;
        objs->w = b_y_;
        objs->h = b_x_;
        objs->obj = (*img)(cv::Range(b_x, b_x + b_x_),cv::Range(b_y, b_y + b_y_));
        objs->rect = std::move(*r);
        frame->detect_objs.push_back(objs);
        //     rectangle(*img, cvPoint(cvRound(r->x*scale), cvRound(r->y*scale)),
        //     cvPoint(cvRound((r->x + r->width-1)*scale), cvRound((r->y + r->height-1)*scale)),
        //     color, 3, 8, 0);
        // cv::Mat obj(*img, cvPoint(cvRound(r->x*scale), cvRound(r->y*scale)),
        //     cvPoint(cvRound((r->x + r->width-1)*scale));
    }
    //frame->faces_rect = std::move(faces);
    return true;

  }

}