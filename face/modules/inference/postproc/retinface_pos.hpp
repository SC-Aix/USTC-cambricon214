#ifndef RETINFACE_POS_HPP_
#define RETINFACE_POS_HPP_

#include "face_module.hpp"
#include "face_pipeline.hpp"
#include "face_config.hpp"

#include "infer.hpp"
#include "anchor_generator.h"
#include "infer_server.h"
#include "video_helper.h"
#include "opencv_frame.h"

#include "posprocess_base.hpp"

namespace facealign {
  class PostprocRetin : public Postproc {
    public:
      PostprocRetin() {}
      int Execute(infer_server::PackagePtr package,
                  const infer_server::ModelPtr& model_info,
                  std::shared_ptr<FAFrameInfo> data_) override;
      bool Postprocess(const std::vector<float*> &net_outputs, 
                            const infer_server::ModelPtr &model,
                            std::shared_ptr<FAFrameInfo> frame_info);
      DECLARE_REFLEX_OBJECT(PostprocRetin);
  };
  
  IMPLEMENT_REFLEX_OBJECT(PostprocRetin);

}


#endif // !RETINFACE_POS_HPP_