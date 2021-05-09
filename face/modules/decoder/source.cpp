#include "source.hpp"

namespace facealign {


 Source::Source(const std::string& name) : Module(name) {
 }

 bool Source::Open(ModuleParamSet& parameters) {
   if (parameters.find("decoder_type")) {
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
 }

 int Source::Process(std::shared_ptr<FAFrameInfo> data) {
   std::cout << "this is source module, Process be invoke!" << std::endl;
   return 0;
 }

 bool Source::CheckParamSet(const ModuleParamSet& parameters) {
   return true;
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

void Source::Loop() {
  decoder_->Loop();
}

void DecoderOpencv::GetFiles(const std::string& path, std::vector<std::string>& files) {
    long hFile = 0;
    struct _finddata_t fileinfo;
    std::string p;
    if ((hFile = _findfirst(p.assign(path).append("\\*").c_str(), &fileinfo) != -1)) {
      do {
        if ((fileinfo.attrib & _A_SUBDIR)) {
          if (strcmp(fileinfo.name, ".") != 0 && strcmp(fileinfo.name, "..") != 0)
            GetFiles(p.assign(path).append("\\").append(fileinfo.name), files);
        } else {
          files.push_back(p.assign(path).append("\\").append(fileinfo.name));
        }
      } while (_findnext(hFile, &fileinfo) == 0);
      _findclose(hFile);
    }
  }

std::shared_ptr<FAFrame> CreateFrame() {
  return new (std::nothrow) FAFrame(*(this->mat_ptr));
}

void DecoderOpencv::DecoderImg(const std::string& path) {
  auto frameinfo = FAFrameInfo::Create();
  cv::Mat data_img;
  cv::imread(data_img, path);
  mat_ptr.reset(&data_img);
  auto frame = CreateFrame();
  frameinfo->datas.insert(std::make_shared(count++, frame));
  source_module->DoProcess(frameinfo);
}

void DecoderOpencv::Loop() {
  for (auto &s : files) {
    DecoderImg(s);
    //是否需要sleep？？
  }
}

} //namespace