#ifndef PREPROCESS_BASE_HPP_
#define PREPROCESS_BASE_HPP_
#include <memory>
#include <string>
#include <utility>
#include <vector>


#include "easyinfer/model_loader.h"
#include "infer_server.h"
#include "processor.h"
#include "reflex_object.hpp"
#include "face_frame.hpp"
#include "infer.hpp"

namespace facealign{
  class Inference;
  class Postproc : virtual public ReflexObjectEx<Postproc> {
    public:
      Postproc() {}
      virtual ~Postproc() {}
      static Postproc* Create(const std::string proc_name) {
        return ReflexObjectEx<Postproc>::CreateObject(proc_name);
      }
      void SetModelPtr(infer_server::ModelPtr ptr) {model_info_ = ptr;}
      void SetInferPtr(Inference* infer) {infer_ptr = infer;}
      virtual int Execute(infer_server::InferData* data,
                  const infer_server::ModelIO& model_out,
                  const infer_server::ModelInfo& model_info) {
        return 0;
      }
    public:
      infer_server::ModelPtr model_info_ = nullptr;
      Inference* infer_ptr = nullptr;
  };
}


#endif // !PREPROCESS_BASE_HPP_
