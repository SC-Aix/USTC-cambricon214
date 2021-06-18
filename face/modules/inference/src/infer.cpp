#include "time.h"
#include "infer.hpp"
//#include "posprocess_base.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/imgcodecs/imgcodecs.hpp"
#include "cnrt.h"

#define CLIP(x) ((x) < 0 ? 0 : ((x) > 1 ? 1 : (x)))


namespace facealign {

// static void cnrtF(void* memory = nullptr, int dev_id) {
  
// }
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

    if (preproc_name != "cncv") {
      desc.preproc = std::make_shared<infer_server::PreprocessorHost>();
      auto preproc_func = std::bind(&Inference::Dopreproc, this, std::placeholders::_1,
         std::placeholders::_2, std::placeholders::_3);
       desc.preproc->SetParams<infer_server::PreprocessorHost::ProcessFunction>("process_function", preproc_func);
    } else {
       desc.preproc = std::make_shared<infer_server::video::PreprocessorMLU>();
       desc.preproc->SetParams("src_format", infer_server::video::PixelFmt::NV21, "dst_format",
                              infer_server::video::PixelFmt::BGRA, "preprocess_type",
                              infer_server::video::PreprocessType::CNCV_RESIZE_CONVERT);
    }

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
    post_ptr.reset(Postproc::Create(postproc_name));
    post_ptr->SetModelPtr(model_ptr);
    
    post_ptr->SetInferPtr(this);
    obs_ptr.reset(new PostRetinObs(this));
    
    if (preproc_name != "cncv") {
      pre_ptr.reset(Preproc::Create(preproc_name));
      pre_ptr->SetModelPtr(model_ptr);
      pre_ptr->SetInferPtr(this);
    }

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
    input_package->data.emplace_back(new infer_server::InferData);


    auto frame = reinterpret_cast<FAFrame*>(data->datas[0].get());
    if (preproc_name != "cncv") {
      input_package->data[batch_num++]->Set(frame);
    } else {
      infer_server::video::VideoFrame vframe;
      auto cn_pkt_ptr = frame->cn_pkt_ptr;

      infer_server::Buffer bufer_y(cn_pkt_ptr->ptrs[0], cn_pkt_ptr->frame_size * 2 / 3, nullptr, 0);
      infer_server::Buffer bufer_uv(cn_pkt_ptr->ptrs[1], cn_pkt_ptr->frame_size / 3, nullptr, 0);
      vframe.plane[0] = bufer_y;
      vframe.plane[1] = bufer_uv;
      vframe.stride[0] = cn_pkt_ptr->strides[0];
      vframe.stride[1] = cn_pkt_ptr->strides[1];
      vframe.width = cn_pkt_ptr->width;
      vframe.height = cn_pkt_ptr->height;
      vframe.plane_num = 2;
      vframe.format = infer_server::video::PixelFmt::NV21;
      input_package->data[batch_num++]->Set(std::move(vframe));
    }
    
   if (batch_num == 4) {
    server_->InferServer::Request(session_ptr, std::move(input_package), batch_array);
    input_package.reset();
    batch_array.clear();
    batch_num = 0;
   }

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

