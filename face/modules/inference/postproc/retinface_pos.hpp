#ifndef RETINFACE_POS_HPP_
#define RETINFACE_POS_HPP_

//#include "face_module.hpp"
//#include "face_pipeline.hpp"
//#include "face_config.hpp"

#include "infer.hpp"
#include "anchor_generator.h"
//#include "infer_server.h"
//#include "video_helper.h"
//#include "opencv_frame.h"
#include "face_threadpool.hpp"


namespace facealign {
  class PostprocRetin : public Postproc {
  public:
    // for postproc
    int Execute(infer_server::InferData* data,
      const infer_server::ModelIO& model_out,
      const infer_server::ModelInfo& model_info) override {
      data->Set(model_out);
      return 1;
    }
  private:
    DECLARE_REFLEX_OBJECT(PostprocRetin);

  };



  class PostRetinObs : public infer_server::Observer {
  public:
    PostRetinObs(Inference* ptr) : infer_ptr(ptr) {
      tp = new Threadpool(nullptr, 5);
    }
    ~PostRetinObs() {
      if (tp) free(tp);
    }

    bool DoResponse(infer_server::InferDataPtr data, infer_server::ModelPtr model_info,
      std::shared_ptr<FAFrameInfo> data_);
    bool Postprocess(const std::vector<float*>& net_outputs,
      const infer_server::ModelPtr& model,
      std::shared_ptr<FAFrameInfo> frame_info);

    // for response
    void Response(infer_server::Status status,
      infer_server::PackagePtr data,
      infer_server::any user_data) noexcept override;

  private:
    Threadpool* tp = nullptr;
    Inference* infer_ptr = nullptr;
  };

}


#endif // !RETINFACE_POS_HPP_
