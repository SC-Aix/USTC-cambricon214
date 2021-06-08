#include "time.h"
#include "infer.hpp"
//#include "posprocess_base.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/imgcodecs/imgcodecs.hpp"


#define CLIP(x) ((x) < 0 ? 0 : ((x) > 1 ? 1 : (x)))

namespace facealign {


  // bool Inference::MoveDataPostproc(infer_server::InferData* output_data, const infer_server::ModelIO& model_output) {
  //   output_data->Set(model_output);
  //   return true;
  // }

  bool Inference::Open(ModuleParamSet parameters) {
    if (parameters.find("model_path") == parameters.end()) {
      std::cout << "model path is not set" << std::endl;
      return false;
    }
    else {
      model_path = parameters["model_path"];
    }

    if (parameters.find("preproc") == parameters.end()) {
      std::cout << "preproc function is not set" << std::endl;
      return false;
    }
    else {
      preproc_name = parameters["preproc"];
    }

    if (parameters.find("posproc") == parameters.end()) {
      std::cout << "posproc is not set" << std::endl;
      return false;
    }
    else {
      postproc_name = parameters["posproc"];
    }

    return Init();
  }

  infer_server::Session_t Inference::CreateSession() {
    infer_server::SessionDesc desc;
    desc.name = "detect"; // (TODO) this can set by json
    desc.strategy = infer_server::BatchStrategy::DYNAMIC;
    desc.engine_num = 1;
    desc.show_perf = false;
    desc.priority = 0;
    desc.model = model_ptr;
    desc.host_input_layout = { infer_server::DataType::FLOAT32, infer_server::DimOrder::NHWC };
    desc.host_output_layout = { infer_server::DataType::FLOAT32, infer_server::DimOrder::NCHW };

    desc.preproc = std::make_shared<infer_server::PreprocessorHost>();
    auto preproc_func = std::bind(&Inference::Dopreproc, this, std::placeholders::_1,
      std::placeholders::_2, std::placeholders::_3);

    desc.preproc->SetParams<infer_server::PreprocessorHost::ProcessFunction>("process_function", preproc_func);

    auto postproc_func = std::bind(&Inference::Dopostproc, this, std::placeholders::_1,
      std::placeholders::_2, std::placeholders::_3);
    desc.postproc = std::make_shared<infer_server::Postprocessor>();
    desc.postproc->SetParams<infer_server::Postprocessor::ProcessFunction>("process_function", postproc_func);
    return server_->CreateSession(desc, obs_ptr);
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
    
    pre_ptr->SetInferPtr(this);
    post_ptr->SetInferPtr(this);
    obs_ptr.reset(new PostRetinObs(this));
    //create session
    session_ptr = CreateSession();
    if (!session_ptr) {
      std::cerr << "*************create session failed!*************" << std::endl;
      return false;
    }

    return true;
  }

  int Inference::Process(std::shared_ptr<FAFrameInfo> data) {
   if (data->IsEos()) {
    {
     RwLockWriteGuard lk(container_lock_);
     this->container_->RightMove();
    }
     Transmit(data);
     return 0;
   }

    if (batch_num == 0) {
      input_package = std::make_shared<infer_server::Package>();
    }
    batch_array.push_back(data);
    //infer_server::Status status = infer_server::Status::SUCCESS;
    //infer_server::PackagePtr detect_input_package = std::make_shared<infer_server::Package>();
    input_package->data.push_back(std::make_shared<infer_server::InferData>());
    auto frame = reinterpret_cast<FrameOpencv*>(data->datas[0].get());
    cv::Mat frame_cv = *(frame->image_ptr.get());
    input_package->data[batch_num++]->Set(frame_cv);
    //infer_server::PackagePtr detect_output_package = std::make_shared<infer_server::Package>();
    //server_->InferServer::RequestSync(session_ptr, detect_input_package, &status, detect_output_package);
   if (batch_num == 4) {
    server_->InferServer::Request(session_ptr, std::move(input_package), batch_array);
    input_package.reset();
    batch_array.clear();
    batch_num = 0;
   }
    // if (infer_server::Status::SUCCESS != status) {
    //   std::cerr << " Run detector offline model failed.";
    //   return -1;
    // }
    return 0;
  }

  bool Inference::Transmit(std::shared_ptr<FAFrameInfo> data) {
    RwLockReadGuard lk(this->container_lock_);
    auto pipeline = this->container_;
    if (!pipeline->ProvideData(this, data)) {
      std::cerr << "erro tansmit data in inference" << std::endl;
    }
    return true;
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

