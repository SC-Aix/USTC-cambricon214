#ifndef INFER_HPP_
#define INFER_HPP_

#include "face_module.hpp"
#include "face_pipline.hpp"

#include "infer_server.h"
namespace facealign {

class Inference : public Module, public ModuleCreator<Inference> {
  public:
    Inference(){}
    bool Init();
    bool Open(ModuleParamset parameters);
    int Process(std::shared_ptr<FAFrameInfo> data);
    static bool Preprocess();
    static bool Postprocess();
    infer_server::Session_t CreateSession();
  
  public:
    int dev_id = 0;
    std::string model_path = "";
    std::string model_name = "subnet0";
    std::unique_ptr<infer_server::video::VideoInferServer> server_ = nullptr;
    infer_server::ModelPtr model_ptr = nullptr;
    infer_server::Session_t session_ptr = nullptr;
};

}

#endif // !INFER_HPP_