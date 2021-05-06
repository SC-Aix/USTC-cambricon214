#include <memory>
#include <string>
#include <iostream>
#include <unordered_map>

#include "face_frame.hpp"

namespace facealign {
std::shared_ptr<FAFrameInfo>  FAFrameInfo::Create(bool eos, std::shared_ptr<FAFrameInfo> payload) {
   std::shared_ptr<FAFrameInfo> ptr(new (std::nothrow) FAFrameInfo());
   if (!ptr) {
     std::cout << "FAFrameInfo created failed!" << std::endl;
     return nullptr;
   }
   ptr->payload = payload;
   if (eos) {
     ptr->flags |= facealign::FA_FRAME_FLAG_EOS;
     return ptr;
   }
  return ptr;
}

FAFrameInfo::~FAFrameInfo() {
}

}