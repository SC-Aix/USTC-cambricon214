#include <iostream> 
#include "feature_extract.hpp"

#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"


int main () {
  cv::Mat image;
  image = cv::imread("./../data/init.jpg");
  if (image.data == nullptr) {
    std::cerr<<"no image found"<<std::endl;
    return 0;
  } else {
   std::cout << image << std::endl;
  }
  imwrite("./../data/123.jpg", image);
  return 0;


}
