#include "show.hpp"

namespace facealign {

int Show::Process(std::shared_ptr<FAFrameInfo> data) {
  std::cout << "Show" << std::endl;
  auto frame = reinterpret_cast<FrameOpencv*>(data->datas[0].get());
  // //cv::imshow("123.jpg", frame->image_data);
  cv::namedWindow( "Display window", cv::WINDOW_AUTOSIZE );
  cv::imshow( "Display window", *(frame->image_ptr));  
  cv::waitKey(1000);
  cv::destroyWindow("Display window");
  return 0;
}

}