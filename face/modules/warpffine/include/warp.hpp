#ifndef WARP_HPP_
#define WARP_HPP_

#include "face_pipeline.hpp"
#include "face_module.hpp"
namespace facealign {
  class OpencvWarp : public Module, public ModuleCreator<OpencvWarp> {
  public:
    OpencvWarp(const std::string& name) : Module(name) {}
    ~OpencvWarp() {}
    bool Open(ModuleParamSet parameters) override { return true; };
    void Close() override {};
    int Process(std::shared_ptr<FAFrameInfo> data) override;
    bool GetTransFormMatrix(std::shared_ptr<FAFrameInfo> data);
  
  private:
    std::vector<cv::Mat> src_points;
  };

}

#endif // !WARP_HPP_