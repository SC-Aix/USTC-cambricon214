#include "show.hpp"

namespace facealign {

int Show::Process(std::shared_ptr<FAFrameInfo> data) {
  //std::cout << "Show" << std::endl;
  auto frame = reinterpret_cast<FrameOpencv*>(data->datas[0].get());
 // cv::imshow("123.jpg", frame->image_data);
  if (!frame->detect_objs.empty()) {
    cv::imshow( "Display window", frame->detect_objs[0]->obj);  
    cv::waitKey(30);
  }
  return 0;
}

}
