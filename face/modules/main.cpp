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
    200,
    "Source",
    {"infer"}
  };

  facealign::FAModuleConfig infer_config = {
    {"infer"},
    {
      {"model_path", "/home/suchong/workspace/face/USTC-cambricon214/face/data/model/retinaface_4c4b_bgr_270_v140.cambricon"}
    },
    200,
    "Inference",
    {"show"}
  };

  facealign::FAModuleConfig show_config = {
    {"show"},
    {},
    400,
    "Show",
    {}
  };

  pipeline.BuildPipeline({source_config, infer_config, show_config});
//  pipeline.BuildPipeline({source_config, infer_config});
  //pipeline.BuildPipeline({source_config, show_config});
  pipeline.Start();
  auto source = pipeline.GetModule("source");
  auto source_ = dynamic_cast<facealign::Source* >(source);
  source_->AddDecoder();
  auto decoder_ = dynamic_cast<facealign::DecoderOpencv* >(source_->GetDecoder());
  decoder_->AddFiles("/home/suchong/workspace/face/USTC-cambricon214/face/data/video/");
  decoder_->VideoLoop();
  std::this_thread::sleep_for(std::chrono::seconds(20));
  pipeline.Stop();

}
