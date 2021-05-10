#ifndef SHOW_HPP_
#define SHWO_HPP_

#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"

#include "face_pipeline.hpp"
#include "face_module.hpp"
#include "frame.hpp"

namespace facealign {

class Show : public Module, public ModuleCreator<Show> {
  public:
    Show(const std::string& name) : Module(name){}
    bool Open(ModuleParamSet parameters) override { return true; };
    void Close() override {};
    int Process(std::shared_ptr<FAFrameInfo> data) override;
};

}
#endif // !SHOW_HPP_
