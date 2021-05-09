#include "frame.h"
namespace facealign {
void* Frame::GetFrameCpuData() {
  return cpu_data;
}

void* Frame::GetFrameMluData() {
  return mlu_data;
}

void FrameOpencv::Cpu2Mlu() {
  cnrtMemcopy(mlu_data, cpu_data, size, type);
}

void FrameOpencv::Mlu2Cpu() {
  cnrtMemcopy(cpu_data, mlu_data, size, type)
}

void FrameOpencv::SetCpuPtr() {
  this->cpu_data = reinterpret_cast<void*>(image_data.data());
}

void FrameOpencv::SetMluPtr() {
  cnrtMalloc(&Mlu2Cpu, size);
  Cpu2Mlu();
}
}
