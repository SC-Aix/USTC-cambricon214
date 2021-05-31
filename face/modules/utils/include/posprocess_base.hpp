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

namespace facealign{
  class Postproc : virtual public ReflexObjectEx<Postproc> {
    public:
      virtual ~Postproc() {}
      static Postproc* Create(const std::string proc_name);
      void SetModelPtr(infer_server::ModelPtr ptr) {model_info_ = ptr;}
      virtual int Execute(infer_server::PackagePtr package,
                  const infer_server::ModelPtr& model_info,
                  std::shared_ptr<FAFrameInfo> data_) {
        return 0;
      }
    public:
      infer_server::ModelPtr model_info_ = nullptr;
  };
}


#endif // !PREPROCESS_BASE_HPP_