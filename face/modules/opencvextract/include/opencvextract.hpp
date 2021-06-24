#ifndef OPENCVEXTRACT_HPP_
#define OPENCVEXTRACT_HPP_

#include <opencv2/opencv.hpp>
#include <opencv2/face.hpp>

#include "face_pipeline.hpp"
#include "face_module.hpp"
#include "face_config.hpp"


namespace facealign {
  class OpencvExtract: public Module, public ModuleCreator<OpencvExtract> {
    public:
      OpencvExtract(const std::string& name) : Module(name) {
        facemark = cv::face::FacemarkLBF::create();
        //facemark->loadModel(path_);
      }
      void Close() {}
      bool Open(ModuleParamSet parameters);
      int Process(std::shared_ptr<FAFrameInfo> data);
    private:
      std::string path_ = "";
      cv::Ptr<cv::face::Facemark> facemark;
  };
}

#endif // !OPENCVEXTRACT_HPP_