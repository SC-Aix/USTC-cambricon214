#ifndef RETINFACE_PRE_HPP_
#define RETINFACE_PRE_HPP_
#include "face_module.hpp"
#include "face_pipeline.hpp"
#include "face_config.hpp"

#include "infer.hpp"
#include "anchor_generator.h"
#include "infer_server.h"
#include "video_helper.h"
#include "opencv_frame.h"

#include "preprocess_base.hpp"
namespace facealign {
  class PreprocRetin : public Preproc {
    public:
      PreprocRetin() {}
      cv::Mat ExpandImage(cv::Mat input_img);
      int preproc(const cv::Mat &input, cv::Mat *output, const infer_server::ModelPtr &model);
      int Execute(infer_server::ModelIO* dst, const infer_server::InferData& src,
                                   const infer_server::ModelInfo& model_info) override;
      DECLARE_REFLEX_OBJECT(PreprocRetin);
  };
  IMPLEMENT_REFLEX_OBJECT(PreprocRetin);
}


#endif // !RETINFACE_PRE_HPP