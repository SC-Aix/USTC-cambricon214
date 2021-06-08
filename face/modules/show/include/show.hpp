#ifndef SHOW_HPP_
#define SHWO_HPP_

#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"

#include "face_pipeline.hpp"
#include "face_module.hpp"
#include "time.h"
#include "frame.hpp"

namespace facealign {

class Show : public Module, public ModuleCreator<Show> {
  public:
    Show(const std::string& name) : Module(name){cv::namedWindow( "Display window", cv::WINDOW_AUTOSIZE );}
    ~Show() { cv::destroyWindow("Display window");}
    bool Open(ModuleParamSet parameters) override { return true; };
    void Close() override {};
    int Process(std::shared_ptr<FAFrameInfo> data) override;
    
    // clock_t begin = clock();
    // int count = 0;
};

}
#endif // !SHOW_HPP_
