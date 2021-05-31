#include "preprocess_base.hpp"

namespace facealign {
  Preproc* Preproc::Create(const std::string proc_name) {
    return ReflexObjectEx<Preproc>::CreateObject(proc_name);
  }
}