#ifndef OPENCVDETECT_HPP_
#define OPENCVDETECT_HPP_
#include <string>

#include "face_pipeline.hpp"
#include "face_module.hpp"
#include "face_config.hpp"

#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/objdetect/objdetect.hpp>

namespace facealign {
  class OpencvDetect : public Module, public ModuleCreator<OpencvDetect> {
    public:
      OpencvDetect(const std::string& name) : Module(name) {
        cascade = std::make_shared<cv::CascadeClassifier>();
      }
      bool Open(ModuleParamSet parameters);
      void Close() { };
      int Process(std::shared_ptr<FAFrameInfo> data);
    private:
      std::string path_ = "";
      float scale = 1.0;
      std::shared_ptr<cv::CascadeClassifier> cascade = nullptr;
  };
}



#endif // !OPENCVDETECT_HPP_
