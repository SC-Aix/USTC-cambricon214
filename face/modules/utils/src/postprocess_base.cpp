#include "posprocess_base.hpp"

namespace facealign {
  Postproc* Postproc::Create(const std::string proc_name) {
    return ReflexObjectEx<Postproc>::CreateObject(proc_name);
  }
}