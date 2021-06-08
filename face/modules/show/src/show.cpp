#include "show.hpp"
#include <chrono>
namespace facealign {

int Show::Process(std::shared_ptr<FAFrameInfo> data) {
   if (data->IsEos()) {
     RwLockWriteGuard lk(container_lock_);
     this->container_->RightMove();
     return 0;
   }
  // ++count;
  // if(count % 25 == 0) {
  //   clock_t end = clock();
  //   std::cout << double(end - begin) / CLOCKS_PER_SEC << std::endl;
  //   std::cout << "count : " << count << std::endl;
  // }
  //std::cout << "Show" << std::endl;
  auto frame = reinterpret_cast<FrameOpencv*>(data->datas[0].get());
  //cv::imshow("123.jpg", *(frame->image_ptr.get()));
  //cv::waitKey(30);
 if (!frame->detect_objs.empty()) {
    cv::imshow( "Display window", frame->detect_objs[0]->obj);  
    cv::waitKey(30);
  // } else {
  //   std::cout << "no good!" << std::endl;
  }
  return 0;
}

}
