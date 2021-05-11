#include <iostream> 
#include "feature_extract.hpp"
#include "face_pipeline.hpp"
#include "source.hpp"

#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"


int main () {

  facealign::Pipeline pipeline("name");
  facealign::FAModuleConfig source_config = {
    "source",
    {
      {"decoder_type", "opencv"}
    },
    20,
    "Source",
    {"show"}
  };

  facealign::FAModuleConfig show_config = {
    {"show"},
    {},
    20,
    "Show",
    {}
  };

  pipeline.BuildPipeline({source_config, show_config});
  pipeline.Start();
  auto source = pipeline.GetModule("source");
  auto source_ = dynamic_cast<facealign::Source* >(source);
  source_->AddDecoder();
  auto decoder_ = dynamic_cast<facealign::DecoderOpencv* >(source_->GetDecoder());
  decoder_->AddFiles("/home/cambricon/workspace/USTC-cambricon214/face/data/video/");
  decoder_->VideoLoop();
  std::this_thread::sleep_for(std::chrono::seconds(1000));
  pipeline.Stop();

}
