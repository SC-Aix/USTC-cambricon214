#include "show.hpp"

namespace facealign {

int Show::Process(std::shared_ptr<FAFrameInfo> data) {
  std::cout << "Show" << std::endl;
  auto frame = reinterpret_cast<FrameOpencv*>(data->datas[0].get());
  // //cv::imshow("123.jpg", frame->image_data);
  cv::imshow( "Display window", *(frame->image_ptr));  
  cv::waitKey(60);
  return 0;
}

}