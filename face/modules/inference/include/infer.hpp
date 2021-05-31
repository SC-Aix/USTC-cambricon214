#ifndef INFER_HPP_
#define INFER_HPP_

#include "face_module.hpp"
#include "face_pipeline.hpp"
#include "face_config.hpp"

#include "infer.hpp"
#include "anchor_generator.h"
#include "infer_server.h"
#include "video_helper.h"
#include "opencv_frame.h"

#include "posprocess_base.hpp"
#include "preprocess_base.hpp"
namespace facealign {

  class Inference : public Module, public ModuleCreator<Inference> {
  public:
    Inference(const std::string& name) : Module(name) {}
    bool Init();
    bool Open(ModuleParamSet parameters);
    void Close();
    int Process(std::shared_ptr<FAFrameInfo> data);
    infer_server::Session_t CreateSession();

    bool Dopreproc(infer_server::ModelIO* dst, const infer_server::InferData& src,
      const infer_server::ModelInfo& model_info) {
      return pre_ptr->Execute(dst, src, model_info);
    }
    bool MoveDataPostproc(infer_server::InferData* output_data,
      const infer_server::ModelIO& model_output);

  public:
    std::shared_ptr<Postproc> post_ptr = nullptr;
    std::shared_ptr<Preproc> pre_ptr = nullptr;
    int dev_id = 0;
    std::string model_path = "";
    std::string model_name = "subnet0";
    std::string preproc_name = "";
    std::string postproc_name = "";
    std::unique_ptr<infer_server::video::VideoInferServer> server_ = nullptr;
    infer_server::ModelPtr model_ptr = nullptr;
    infer_server::Session_t session_ptr = nullptr;
  };

}

#endif // !INFER_HPP_