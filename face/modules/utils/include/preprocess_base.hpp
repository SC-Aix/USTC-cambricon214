#ifndef POSTPROCESS_BASE_HPP_
#define POSTPROCESS_BASE_HPP_

#include <memory>
#include <string>
#include <utility>
#include <vector>


#include "easyinfer/model_loader.h"
#include "infer_server.h"
#include "processor.h"
#include "reflex_object.hpp"

namespace facealign {
  class Preproc : virtual public ReflexObjectEx<Preproc> {
    public:
      virtual ~Preproc() {}
      static Preproc* Create(const std::string proc_name);
      void SetModelPtr(infer_server::ModelPtr ptr) {model_info_ = ptr;}
      virtual int Execute(infer_server::ModelIO* dst, const infer_server::InferData& src,
                                   const infer_server::ModelInfo& model_info) {
        return 0;
      }
    public:
      infer_server::ModelPtr model_info_ = nullptr;
  };
}


#endif // !POSTPROCESS_BASE_HPP_
