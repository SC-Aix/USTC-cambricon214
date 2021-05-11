#include "infer.hpp"

namespace facealign {

struct InferParam {
};

bool Inference::Open(ModuleParamSet parameters) {
  if (parameters.find("model_path") == parameters.end()) {
    std::cout << "model path is not set" << std::endl;
    return false;
  } else {
    model_path = parameters["model_path"];
  }
}

infer_server::Session_t Inference::CreateSession() {
  infer_server::SessionDesc desc;
  desc.name = "detect"; // (TODO) this can set by json
  desc.strategy = infer_server::BatchStrategy::STATIC;
  desc.engine_num = 1;
  desc.show_perf = false;
  desc.priority = 0;
  desc.model_ = model_ptr;
  desc.host_input_layout = {infer_server::DataType::FLOAT32, infer_server::DimOrder::NHWC};
  desc.host_output_layout = {infer_server::DataType::FLOAT32, infer_server::DimOrder::NCHW};

  
}

bool Inference::Init() {
  server_.reset(new infer_server::video::VideoInferServer(dev_id));
  model_ptr = server_.LoadModel(model_path, model_name);

  //create session
  session_ptr = CreateSession();

}


}