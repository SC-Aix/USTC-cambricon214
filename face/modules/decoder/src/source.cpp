#include "source.hpp"

namespace facealign {
int DecoderOpencv::count = 0;

 Source::Source(const std::string& name) : Module(name) {
 }

 void Source::Close() {
   std::cout << "this close!" << std::endl;
 }

 bool Source::CheckParamSet(const ModuleParamSet& parameters) const { // cosnt 必须加
   return true;
 }

 bool Source::Open(ModuleParamSet parameters) {
   if (parameters.find("decoder_type") != parameters.end()) {
     if(parameters["decoder_type"] == "opencv")
     param.type = FFOPENCV;
     else if (parameters["decoder_type"] == "rtsp")
     param.type = FFRTSP;
     else {
       std::cout << "unsupport format now!" << std::endl;
       return false;
     }
     return true;
   }
   return false;
 }

 int Source::Process(std::shared_ptr<FAFrameInfo> data) {
  // std::cout << "this is source module, Process be invoke!" << std::endl;
   return 0;
 }


 bool Source::AddDecoder() {
   if (param.type == FFOPENCV) {
     decoder_ = new (std::nothrow) DecoderOpencv(this);
   } else if (param.type == FFRTSP) {
     
   } else {
     return false;
   }
   return true;
 }

void Source::ImgLoop() {
  decoder_->ImgLoop();
}

void Source::VideoLoop() {
  decoder_->VideoLoop();
}

void DecoderOpencv::GetFiles(const std::string& path, std::vector<std::string>& files) {
  struct dirent *ptr = nullptr;
  DIR *dir;
  dir = opendir(path.c_str());
  if (dir == nullptr) std::cout << "data directory is wrong!" << std::endl;

  while ((ptr = readdir(dir)) != nullptr) {
    if (ptr->d_name[0] == '.')
    continue;
    files.push_back(path + ptr->d_name);
  }
}

std::shared_ptr<FAFrame> DecoderOpencv::CreateFrame() {
  return std::shared_ptr<FAFrame>(new (std::nothrow) FrameOpencv(std::move(mat_ptr)));
}

void DecoderOpencv::DecoderImg(const std::string& path) {
  auto frameinfo = FAFrameInfo::Create();
  cv::Mat *img = mat_ptr.get();
  std::cout << path << std::endl;
  *img = cv::imread(path);
  //mat_ptr = data_img;
  auto frame = CreateFrame();
  frameinfo->datas.insert(std::make_pair(count, frame));
  std::cout << "in source DecoderImg" << std::endl;
  source_module->DoProcess(frameinfo);
  //mat_ptr.reset();
}

void DecoderOpencv::ImgLoop() {
  std::cout << "in DecoderOpencv Loop, files is : " << files.size() << std::endl;
  for (auto &s : files) {
    DecoderImg(s);
    //是否需要sleep？？
  }
}

void DecoderOpencv::DecoderVideo(const std::string& path) {
  cv::VideoCapture cap(path);
  if (!cap.isOpened()) {
    std::cerr << "cannot open a camera or file" << std::endl;
    return;
  }
  //cv::Mat *img = mat_ptr.get();  
  bool flag = true;

  while (flag) {
    mat_ptr = std::make_shared<cv::Mat>();
    cv::Mat *img = mat_ptr.get(); 
    cap >> *img;
    
    if (img->rows <= 0 || img->cols <= 0) {
      flag = false;
      continue;
    } 
    auto frameinfo = FAFrameInfo::Create();
    auto frame = CreateFrame();
    if (frame == nullptr) {
      std::cout << "*************create frame failed!****************" << std::endl;
    }
    frameinfo->datas.insert(std::make_pair(count, frame));
    source_module->DoProcess(frameinfo);
  }
  
  cap.release();
}

void DecoderOpencv::VideoLoop() {
  std::cout << "files size : "<<files.size() << std::endl;
  for (auto &s : files) {
    DecoderVideo(s);
  }
}

} //namespace