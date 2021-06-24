#include <iostream>
#include <chrono>
#include "face_pipeline.hpp"
#include "source.hpp"

#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
using namespace std;
using namespace chrono;
int main() {

  facealign::Pipeline pipeline("name");
  facealign::FAModuleConfig source_config = {
    "source",
    {
      {"decoder_type", "opencv"},
      { "path", "/home/suchong/workspace/face/USTC-cambricon214/face/data/video/"}
    },
    200,
    "Source",
    {"infer"}
    //{"opencvdetect"}
  };

  facealign::FAModuleConfig opencvdetect_config = {
    "opencvdetect",
    {
      {"path", "/usr/share/opencv/haarcascades/haarcascade_frontalface_alt2.xml"}
    },
    200,
    "OpencvDetect",
    {"opencvextract"}
  };


  facealign::FAModuleConfig opencvextract_config = {
    "opencvextract",
    {
      {"path", "/home/suchong/workspace/face/USTC-cambricon214/face/data/model/lbfmodel.yaml"}
    },
    200,
    "OpencvExtract",
    {}
  };

  facealign::FAModuleConfig infer_config = {
    {"infer"},
    {
      {"model_path", "/home/suchong/workspace/face/USTC-cambricon214/face/data/model/retinaface_4c4b_bgr_270_v140.cambricon"},
      {"preproc", "PreprocRetin"},
    //  {"preproc", "cncv"},
      {"posproc", "PostprocRetin"}
    },
    200,
    "Inference",
    {"opencvextract"}
  };

  facealign::FAModuleConfig warp_config = {
    {"warp"},
    {
      {"model_path", "/home/suchong/workspace/face/USTC-cambricon214/face/data/model/retinaface_4c4b_bgr_270_v140.cambricon"}
    },
    200,
    "OpencvWarp",
    {"show"}
  };

  facealign::FAModuleConfig show_config = {
    {"show"},
    {},
    400,
    "Show",
    {}
  };
  auto start = system_clock::now();

 // pipeline.BuildPipeline({ source_config, infer_config, warp_config, show_config });
  pipeline.BuildPipeline({ source_config, infer_config, opencvextract_config});
  //pipeline.BuildPipeline({source_config, infer_config});
 // pipeline.BuildPipeline({source_config, show_config});
  pipeline.Start();
  auto source = pipeline.GetModule("source");
  auto source_ = dynamic_cast<facealign::Source*>(source);
  source_->AddDecoder();
  source_->Run();
  //auto decoder_ = dynamic_cast<facealign::DecoderOpencv*>(source_->GetDecoder());
  //decoder_->AddFiles("/home/suchong/workspace/face/USTC-cambricon214/face/data/video/");
  //decoder_->AddFiles("rtsp://192.168.81.41:8554/ae.mkv");
  //decoder_->VideoLoop();
  //std::this_thread::sleep_for(std::chrono::seconds(10));
  while (pipeline.IsRunning()) {}
  auto end = system_clock::now();
  auto duration = duration_cast<microseconds>(end - start);
  cout << "花费了"
    << double(duration.count()) * microseconds::period::num / microseconds::period::den
    << "秒" << endl;
  //std::this_thread::sleep_for(1);
  pipeline.Stop();

}
