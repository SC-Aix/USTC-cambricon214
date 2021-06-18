#ifndef DECODER_HPP_
#define DECODER_HPP_ 

#include <iostream>
#include <vector>
#include <string>
#include <dirent.h>
#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>

#include "cxxutil/log.h"
#include "device/mlu_context.h"
#include "easycodec/easy_decode.h"

#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"

#include "face_pipeline.hpp"
#include "ffmpeg.hpp"
#include "face_module.hpp"
#include "frame.hpp"

namespace facealign {
  enum DecoderType {
    FFOPENCV = 0,
    FFCODEC = 1,
    FFDUNKNOWN = 2,
  };

  enum FileType {
    FFRTSP = 0,
    FFVIDEO = 1,
    FFIMAGE = 2,
    FFFUNKNOWN = 3,
  };

  struct SourceParam {
    DecoderType DType = FFDUNKNOWN;
    //FileType  FType = FFFUNKNOWN;
    std::string path = "";
  };

  class Decoder;
  class VideoParser;
  class IDemuxEventHandle;

  class Source : public Module, public ModuleCreator<Source> {
  public:
    Source(const std::string& name);
    bool Open(ModuleParamSet parameters) override;
    void Close() override;
    int Process(std::shared_ptr<FAFrameInfo> data) override;
    bool CheckParamSet(const ModuleParamSet& parameters) const override;
    bool AddDecoder();
    void Run();

  private:
    Decoder* decoder_ = nullptr;
    SourceParam param;
  };

  class Decoder {
  public:
    virtual void DecoderImg(const std::string& path)  {};
    //virtual std::shared_ptr<FAFrame> CreateFrame()  { return nullptr; };
    virtual void Loop() {};
    void GetFiles(const std::string& path, std::vector<std::string>& files);
  protected:
    Decoder(Source* ptr, const std::string& path) : 
    source_module(ptr), path_(path) {
      GetFiles(path_, files);
    }
    bool is_img(const char* path);
    bool rtsp_flag = false;
    bool img_flag = false;
    Source* source_module = nullptr;
    const std::string path_ = "";
    std::vector<std::string> files;
    static int count;
  };

  class DecoderOpencv : public Decoder {
  public:
    DecoderOpencv(Source* ptr, const std::string& path) : Decoder(ptr, path) { mat_ptr = std::make_shared<cv::Mat>(); };
    void DecoderImg(const std::string& path) override;
    std::shared_ptr<FAFrame> CreateFrame();
    void Loop() override;
    //void AddFiles(const std::string& path) { GetFiles(path, files); }
    void DecoderVideo(const std::string& path);
   // void VideoLoop() override;
  private:
    std::shared_ptr<cv::Mat> mat_ptr = nullptr;
  };

  class DecoderCodec : public Decoder {
  public:
    explicit DecoderCodec(Source* ptr, const std::string& path);
    ~DecoderCodec();
    std::shared_ptr<FAFrame> CreateFrame(std::shared_ptr<edk::CnFrame> frame);
    void Start() { running_.store(true); }
    void Stop() {
      running_.store(false);
      cond_.notify_one();
    }
    void Loop() override;
    bool RunLoop();
    void Process(edk::CnFrame& info);
    void DemuxLoop(const uint32_t repeat_time);

    // void Backbuff(uint64_t id) {
    //   decode_->ReleaseBuffer(id);
    // }

    void ReceiveEos() { receive_eos_ = true; }
    void ReceiveFrame(const edk::CnFrame& info) {
      std::unique_lock<std::mutex> lk(mut_);
      frames_.push(info);
      cond_.notify_one();
    }

    bool Running() { return running_.load(); }

  protected:
    void WaitForRunLoopExit() {
      while (in_loop_.load()) {}
    }
    edk::MluContext env_;
    std::unique_ptr<edk::EasyDecode> decode_{ nullptr };

    std::thread* runloop = nullptr;
    std::thread* demuxloop = nullptr;

  private:
    DecoderCodec() = delete;
    class DemuxEventHandle : public IDemuxEventHandle {
    public:
      explicit DemuxEventHandle(DecoderCodec* runner) : runner_(runner) {}
      bool OnPacket(const edk::CnPacket& packet) override { return runner_->decode_->FeedData(packet, true); }
      void OnEos() override { LOGI(SAMPLES) << "capture EOS"; }
      void SendEos() {
        if (!send_eos_) {
          runner_->decode_->FeedEos();
          send_eos_ = true;
        }
      }
      bool Running() override {
        return runner_->Running();
      }
    private:
      DecoderCodec* runner_;
      bool send_eos_{ false };
    } demux_event_handle_;

    std::unique_ptr<VideoParser> parser_;
    std::queue<edk::CnFrame> frames_;
    std::mutex mut_;
    std::condition_variable cond_;
    //std::string data_path_;
    std::atomic<bool> receive_eos_{ false };
    std::atomic<bool> running_{ false };
    std::atomic<bool> in_loop_{ false };

  };


} //facealign
#endif // !DECODER_HPP_
