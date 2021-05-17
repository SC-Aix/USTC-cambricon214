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
namespace facealign {

class Inference : public Module, public ModuleCreator<Inference> {
  public:
    Inference(const std::string &name) : Module(name) {}
    bool Init();
    bool Open(ModuleParamSet parameters);
    void Close();
    int Process(std::shared_ptr<FAFrameInfo> data);
    //static bool Preprocess();
    bool Postprocess(const std::vector<float*> &net_outputs, 
                     const infer_server::ModelPtr &model,
                     std::shared_ptr<FAFrameInfo> frame_info);
    bool DetectCpuPostproc(infer_server::PackagePtr package, const infer_server::ModelPtr& model_info,
                           std::shared_ptr<FAFrameInfo> data_);
    infer_server::Session_t CreateSession();
    bool MoveDataPostproc(infer_server::InferData* output_data, const infer_server::ModelIO& model_output);
    cv::Mat ExpandImage(cv::Mat input_img);
    int Preproc(const cv::Mat &input, cv::Mat *output, const infer_server::ModelPtr &model);
    bool Dopreproc(infer_server::ModelIO* dst, const infer_server::InferData& src,
                                   const infer_server::ModelInfo& model_info);

  public:
    //bool is_init = false; 
    int dev_id = 0;
    std::string model_path = "";
    std::string model_name = "subnet0";
    std::unique_ptr<infer_server::video::VideoInferServer> server_ = nullptr;
    infer_server::ModelPtr model_ptr = nullptr;
    infer_server::Session_t session_ptr = nullptr;
};

}

#endif // !INFER_HPP_