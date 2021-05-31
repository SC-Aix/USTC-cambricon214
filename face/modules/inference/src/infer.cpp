#include "time.h"
#include "infer.hpp"

#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/imgcodecs/imgcodecs.hpp"


#define CLIP(x) ((x) < 0 ? 0 : ((x) > 1 ? 1 : (x)))

namespace facealign {


bool Inference::MoveDataPostproc(infer_server::InferData* output_data, const infer_server::ModelIO& model_output) {
  output_data->Set(model_output);
  return true;
}

bool Inference::Open(ModuleParamSet parameters) {
  if (parameters.find("model_path") == parameters.end()) {
    std::cout << "model path is not set" << std::endl;
    return false;
  } else {
    model_path = parameters["model_path"];
  }

  if (parameters.find("preproc") == parameters.end()) {
    std::cout << "preproc function is not set" << std::endl;
    return false;
  } else {
    preproc_name = parameters["preproc"];
  }

  if (parameters.find("posproc") == parameters.end()) {
    std::cout << "posproc is not set" << std::endl;
    return false;
  } else {
    postproc_name = parameters["posproc"];
  }

  return Init();
}

infer_server::Session_t Inference::CreateSession() {
  infer_server::SessionDesc desc;
  desc.name = "detect"; // (TODO) this can set by json
  desc.strategy = infer_server::BatchStrategy::STATIC;
  desc.engine_num = 2;
  desc.show_perf = false;
  desc.priority = 0;
  desc.model = model_ptr;
  desc.host_input_layout = {infer_server::DataType::FLOAT32, infer_server::DimOrder::NHWC};
  desc.host_output_layout = {infer_server::DataType::FLOAT32, infer_server::DimOrder::NCHW};

  desc.preproc = std::make_shared<infer_server::PreprocessorHost>();
  auto preproc_func = std::bind(&Inference::Dopreproc, this, std::placeholders::_1,
                               std::placeholders::_2, std::placeholders::_3);
  
  desc.preproc->SetParams<infer_server::PreprocessorHost::ProcessFunction>("process_function", preproc_func);

  auto postproc_func = std::bind(&Inference::MoveDataPostproc, this, std::placeholders::_1,
                                 std::placeholders::_2);
  desc.postproc = std::make_shared<infer_server::Postprocessor>();
  desc.postproc->SetParams<infer_server::Postprocessor::ProcessFunction>("process_function", postproc_func);
  return server_->CreateSyncSession(desc);
}

bool Inference::Init() {
  std::cout << __LINE__ << "in infer init function" << std::endl;
  server_.reset(new infer_server::video::VideoInferServer(dev_id));
  if (server_ == nullptr) {
    std::cerr << "server_ init failed !" << std::endl;
  }

  model_ptr = server_->LoadModel(model_path, model_name);
  pre_ptr.reset(Preproc::Create(preproc_name));
  post_ptr.reset(Postproc::Create(postproc_name));
  pre_ptr->SetModelPtr(model_ptr);
  post_ptr->SetModelPtr(model_ptr);

  //create session
  session_ptr = CreateSession();
  if (!session_ptr) {
    std::cerr << "*************create session failed!*************" << std::endl;
    return false;
  }
  
  return true;
}

int Inference::Process(std::shared_ptr<FAFrameInfo> data) {

  infer_server::Status status = infer_server::Status::SUCCESS;
  infer_server::PackagePtr detect_input_package = std::make_shared<infer_server::Package>();
  detect_input_package->data.push_back(std::make_shared<infer_server::InferData>());
  
  auto frame = reinterpret_cast<FrameOpencv*>(data->datas[0].get());
  if (frame->image_ptr.get() == nullptr) std::cout << "---------******8%$#%^^&-----------" << std::endl;
  cv::Mat frame_cv = *(frame->image_ptr.get());
  detect_input_package->data[0]->Set(frame_cv);
  if (frame_cv.cols == 0 || frame_cv.rows == 0) { //TODO 解码 无rows = 0 ， 传递过来rows = 0？？？
    return 0;
  } 
  infer_server::PackagePtr detect_output_package = std::make_shared<infer_server::Package>();
  server_->InferServer::RequestSync(session_ptr, detect_input_package, &status, detect_output_package);
  
  if (infer_server::Status::SUCCESS != status) {
    std::cerr << " Run detector offline model failed.";
    return -1;
  }
  post_ptr->Execute(detect_output_package, model_ptr, data);
  return 0;
}

void Inference::Close() {
 if (server_) {
    if (session_ptr) {
      server_->DestroySession(session_ptr);
      session_ptr = nullptr;
    }
 }
 model_ptr.reset();
}

}

