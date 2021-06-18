#include "ffmpeg.hpp"

#include <atomic>
#include <chrono>
#include <thread>

#include "cxxutil/log.h"
#include "easycodec/easy_decode.h"

#ifdef __cplusplus
extern "C" {
#endif
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
#ifdef __cplusplus
}
#endif
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

#define FFMPEG_VERSION_3_1 AV_VERSION_INT(57, 40, 100)
#include "source.hpp"

namespace facealign {
int Decoder::count = 0;

 Source::Source(const std::string& name) : Module(name) {
 }

 void Source::Close() {
   std::cout << "source close!" << std::endl;
 }

 bool Source::CheckParamSet(const ModuleParamSet& parameters) const { // cosnt 必须加
   return true;
 }

 bool Source::Open(ModuleParamSet parameters) {
   if (parameters.find("decoder_type") == parameters.end()) { 
     std::cerr << "decoder_type is not set" << std::endl;
     return false;
   }

   if (parameters.find("path") == parameters.end()) {
     std::cerr << "decoder path is not set" << std::endl;
     return false;
   } else {
     param.path = parameters["path"];
   }

   if (parameters["decoder_type"] == "opencv")
     param.DType = FFOPENCV;
   else if (parameters["decoder_type"] == "codec")
     param.DType = FFCODEC;
   else {
     std::cout << "unsupport format now!" << std::endl;
     return false;
   }
  if (!AddDecoder())  {
    std::cerr << "Add Decoder failed!" << std::endl;
    return false;
  }
   return true;
 }

 int Source::Process(std::shared_ptr<FAFrameInfo> data) {
   if (data->IsEos()) {
     RwLockWriteGuard lk(container_lock_);
     this->container_->RightMove();
     return 0;
   }
   return 0;
 }


 bool Source::AddDecoder() {
   if (param.DType == FFOPENCV) {
     decoder_ = new (std::nothrow) DecoderOpencv(this, param.path);
   } else if (param.DType == FFCODEC) {
     decoder_ = new (std::nothrow) DecoderCodec(this, param.path);
   } else {
     std::cerr << "unknow decoder type!" << std::endl;
     return false;
   }
   return true;
 }

void Source::Run() {
  decoder_->Loop();
}

bool Decoder::is_img(const char* path) {
   if (strstr(path, ".jpg") != NULL ||
     strstr(path, ".jPEG") != NULL) {
     img_flag = true;
   }
   return img_flag;
 }

void Decoder::GetFiles(const std::string& path, std::vector<std::string>& files) {
  if (path.substr(0, 7) == "rtsp://") {
      files.push_back(path);
      rtsp_flag = true;
      std::cout << "file input is networkstream!" << std::endl;
      std::cout << path << std::endl;
      return;
  }

  struct dirent *ptr = nullptr;
  DIR *dir;
  dir = opendir(path.c_str());
  if (dir == nullptr)  {
    std::cout << "data directory is wrong!" << std::endl;
    files.push_back(path);
    return;
  }
  while ((ptr = readdir(dir)) != nullptr) {
    if (ptr->d_name[0] == '.')
    continue;
    if (!img_flag && is_img(ptr->d_name)) img_flag = true;
    files.push_back(path + ptr->d_name);
  }
}

std::shared_ptr<FAFrame> DecoderOpencv::CreateFrame() {
  return std::shared_ptr<FAFrame>(new (std::nothrow) FAFrame(std::move(mat_ptr)));
}

void DecoderOpencv::DecoderImg(const std::string& path) {
  auto frameinfo = FAFrameInfo::Create();
  cv::Mat *img = mat_ptr.get();
  *img = cv::imread(path);
  auto frame = CreateFrame();
  frameinfo->datas.insert(std::make_pair(count, frame));
  std::cout << "in source DecoderImg" << std::endl;
  source_module->DoProcess(frameinfo);
}

void DecoderOpencv::Loop() {  
  if (img_flag) {
    for (auto& s : files) {
      DecoderImg(s);
    }
  } else {
    for (auto& s : files) {
      DecoderVideo(s);
    }
  }
  auto frameinfo = FAFrameInfo::Create(true);
  source_module->DoProcess(frameinfo);
}

void DecoderOpencv::DecoderVideo(const std::string& path) {
  cv::VideoCapture cap(path);
  if (!cap.isOpened()) {
    std::cerr << "cannot open a camera or file" << std::endl;
    return;
  }
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



DecoderCodec::DecoderCodec(Source* ptr, const std::string& path) : Decoder(ptr, path), demux_event_handle_(this) {
  parser_.reset(new VideoParser(&demux_event_handle_));
  if (!parser_->Open(files[0].c_str())) {
    THROW_EXCEPTION(edk::Exception::INIT_FAILED, "Open video source failed");
  }

  // set mlu environment
  env_.SetDeviceId(0);
  env_.BindDevice();

  const VideoInfo& info = parser_->GetVideoInfo();
  // create decoder
  edk::EasyDecode::Attr attr;
  attr.frame_geometry.w = info.width;
  attr.frame_geometry.h = info.height;
  attr.codec_type = info.codec_type;
  // attr.interlaced = info.progressive ? false : true;
  attr.pixel_format = edk::PixelFmt::NV21;
  attr.dev_id = 0;
  attr.frame_callback = std::bind(&DecoderCodec::ReceiveFrame, this, std::placeholders::_1);
  attr.eos_callback = std::bind(&DecoderCodec::ReceiveEos, this);
  attr.silent = false;
  attr.input_buffer_num = 8;
  attr.output_buffer_num = 8;
  decode_ = edk::EasyDecode::New(attr);
}

DecoderCodec::~DecoderCodec() {
  Stop();
  WaitForRunLoopExit();
  if (demuxloop->joinable()) demuxloop->join();
  if (runloop->joinable()) runloop->join();
}

void DecoderCodec::Loop() {
  Start();
  uint32_t value = 0;
  runloop = new std::thread(&DecoderCodec::RunLoop, this);
  demuxloop = new std::thread(&DecoderCodec::DemuxLoop, this, value);
}

void DecoderCodec::DemuxLoop(const uint32_t repeat_time) {
  bool is_rtsp = rtsp_flag;
  uint32_t loop_time = 0;

  try {
    while (Running()) {
      // frame rate control, 25 frame per second for local video
      int ret = parser_->ParseLoop(is_rtsp ? 0 : 0);
      if (ret == -1) {
        THROW_EXCEPTION(edk::Exception::UNAVAILABLE, "no video source");
      }

      if (ret == 1) {
        // eos
        if (repeat_time > loop_time++) {
          parser_->Close();
          if (!parser_->Open(path_.c_str())) {
            THROW_EXCEPTION(edk::Exception::INIT_FAILED, "Open video source failed");
          }
          std::cout << "Loop..." << std::endl;
          continue;
        } else {
          demux_event_handle_.SendEos();
          std::cout << "End Of Stream" << std::endl;
          break;
        }
      }
    }
  } catch (edk::Exception& e) {
    std::cout << "---************************######: " << e.what() << std::endl;
    Stop();
  }
  if (Running()) demux_event_handle_.SendEos();
  parser_->Close();
  std::this_thread::sleep_for(std::chrono::duration<float, std::milli>(1000));

  std::unique_lock<std::mutex> lk(mut_);
  if (frames_.empty()) Stop();
}

bool DecoderCodec::RunLoop() {
  in_loop_.store(true);
  try {
    while (running_.load()) {
      // inference
      std::unique_lock<std::mutex> lk(mut_);

      if (!cond_.wait_for(lk, std::chrono::milliseconds(100), [this] { return !frames_.empty(); })) {
        continue;
      }
      edk::CnFrame frame = frames_.front();
      frames_.pop();
      
      lk.unlock();

      Process(frame);

      lk.lock();

      if (frames_.size() == 0 && receive_eos_.load()) {
          // auto frameinfo = FAFrameInfo::Create(true);
          // source_module->DoProcess(frameinfo);
        break;
      }
      lk.unlock();
    }
  } catch (edk::Exception& err) {
    running_.store(false);
    in_loop_.store(false);
    return false;
  }
  // uninitialize
  running_.store(false);
  in_loop_.store(false);
  auto frameinfo = FAFrameInfo::Create(true);
  source_module->DoProcess(frameinfo);
  return true;
}

std::shared_ptr<FAFrame> DecoderCodec::CreateFrame(std::shared_ptr<edk::CnFrame> frame) {
  return std::shared_ptr<FAFrame>(new (std::nothrow) FAFrame(frame));
}
void DecoderCodec::Process(edk::CnFrame& info) {
  auto info_ = std::make_shared<edk::CnFrame>(info);
  auto frame = CreateFrame(info_);
  std::function<void(uint64_t)> func = std::bind(&edk::EasyDecode::ReleaseBuffer, decode_.get(), std::placeholders::_1);
  frame->Pkt2Mat();
  frame->setCallBackfunc(func);
  auto frameinfo = FAFrameInfo::Create();
  frameinfo->datas.insert(std::make_pair(count, frame));
  source_module->DoProcess(frameinfo);
}

} //namespace